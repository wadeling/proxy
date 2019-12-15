/* Copyright 2017 Istio Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fstream>
#include <iostream>

#include "test/integration/http_integration.h"
#include "test/integration/utility.h"

namespace Envoy {

namespace {
// The HTTP header key for the JWT verification result. Should be the same as
// the one define for forward_payload_header in envoy.conf.jwk
const Http::LowerCaseString kJwtVerificationResultHeaderKey(
    "test-jwt-payload-output");
// {"iss":"https://example.com","sub":"test@example.com","aud":"example_service","exp":2001001001}
const std::string kJwtVerificationResult =
    "eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tIiwic3ViIjoidGVz"
    "dEBleGFtcGxlLmNvbSIsImF1ZCI6ImV4YW1wbGVfc2VydmljZSIs"
    "ImV4cCI6MjAwMTAwMTAwMX0";
}  // namespace

// Base class JWT filter integration tests.
class JwtVerificationFilterIntegrationTest
    : public HttpIntegrationTest,
      public testing::TestWithParam<Network::Address::IpVersion> {
 public:
  JwtVerificationFilterIntegrationTest()
      : HttpIntegrationTest(Http::CodecClient::Type::HTTP1, GetParam(),
                            realTime()) {}
  virtual ~JwtVerificationFilterIntegrationTest() {}
  /**
   * Initializer for an individual integration test.
   */
  void SetUp() override {
    fake_upstreams_.emplace_back(new FakeUpstream(
        0, FakeHttpConnection::Type::HTTP1, version_, timeSystem()));
    registerPort("upstream_0",
                 fake_upstreams_.back()->localAddress()->ip()->port());
    fake_upstreams_.emplace_back(new FakeUpstream(
        0, FakeHttpConnection::Type::HTTP1, version_, timeSystem()));
    registerPort("upstream_1",
                 fake_upstreams_.back()->localAddress()->ip()->port());

    // upstream envoy hardcodes workspace name, so this code is duplicated
    const std::string path = ConfigPath();
    const std::string json_path =
        TestEnvironment::runfilesPath(path, "io_istio_proxy");
    std::string out_json_string =
        TestEnvironment::readFileToStringForTest(json_path);

    // Substitute ports.
    for (const auto& it : port_map_) {
      const std::regex port_regex("\\{\\{ " + it.first + " \\}\\}");
      out_json_string = std::regex_replace(out_json_string, port_regex,
                                           std::to_string(it.second));
    }

    // Substitute paths and other common things.
    out_json_string = TestEnvironment::substitute(out_json_string, version_);

    const std::string extension =
        absl::EndsWith(path, ".yaml") ? ".yaml" : ".json";
    const std::string out_json_path =
        TestEnvironment::temporaryPath(path + ".with.ports" + extension);
    TestEnvironment::createParentPath(out_json_path);
    {
      std::ofstream out_json_file(out_json_path);
      out_json_file << out_json_string;
    }

    test_server_ =
        createIntegrationTestServer(out_json_path, nullptr, timeSystem());
    registerTestServerPorts({"http"});
  }

  /**
   * Destructor for an individual integration test.
   */
  void TearDown() override {
    test_server_.reset();
    fake_upstreams_.clear();
  }

 protected:
  Http::TestHeaderMapImpl BaseRequestHeaders() {
    return Http::TestHeaderMapImpl{
        {":method", "GET"}, {":path", "/"}, {":authority", "host"}};
  }

  Http::TestHeaderMapImpl createHeaders(const std::string& token) {
    auto headers = BaseRequestHeaders();
    headers.addCopy("Authorization", "Bearer " + token);
    return headers;
  }

  Http::TestHeaderMapImpl createIssuerHeaders() {
    return Http::TestHeaderMapImpl{{":status", "200"}};
  }

  std::string InstanceToString(Buffer::Instance& instance) {
    auto len = instance.length();
    return std::string(static_cast<char*>(instance.linearize(len)), len);
  }

  std::map<std::string, std::string> HeadersMapToMap(
      const Http::HeaderMap& headers) {
    std::map<std::string, std::string> ret;
    headers.iterate(
        [](const Http::HeaderEntry& entry,
           void* context) -> Http::HeaderMap::Iterate {
          auto ret = static_cast<std::map<std::string, std::string>*>(context);
          const auto key = std::string(entry.key().getStringView());
          Http::LowerCaseString lower_key{key};
          (*ret)[std::string(lower_key.get())] =
              std::string(entry.value().getStringView());
          return Http::HeaderMap::Iterate::Continue;
        },
        &ret);
    return ret;
  };

  void ExpectHeaderIncluded(const Http::HeaderMap& headers1,
                            const Http::HeaderMap& headers2) {
    auto map1 = HeadersMapToMap(headers1);
    auto map2 = HeadersMapToMap(headers2);
    for (const auto& kv : map1) {
      EXPECT_EQ(map2[kv.first], kv.second);
    }
  }

  void TestVerification(const Http::HeaderMap& request_headers,
                        const std::string& request_body,
                        const Http::HeaderMap& issuer_response_headers,
                        const std::string& issuer_response_body,
                        bool verification_success,
                        const Http::HeaderMap& expected_headers,
                        const std::string& expected_body) {
    IntegrationCodecClientPtr codec_client;
    FakeHttpConnectionPtr fake_upstream_connection_issuer;
    FakeHttpConnectionPtr fake_upstream_connection_backend;
    IntegrationStreamDecoderPtr response;
    FakeStreamPtr request_stream_issuer;
    FakeStreamPtr request_stream_backend;

    codec_client = makeHttpConnection(lookupPort("http"));

    // Send a request to Envoy.
    if (!request_body.empty()) {
      auto encoder_decoder = codec_client->startRequest(request_headers);
      Buffer::OwnedImpl body(request_body);
      codec_client->sendData(encoder_decoder.first, body, true);
      response = std::move(encoder_decoder.second);
    } else {
      response = codec_client->makeHeaderOnlyRequest(request_headers);
    }

    // Empty issuer_response_body indicates issuer will not be called.
    // Mock a response from an issuer server.
    if (!issuer_response_body.empty()) {
      ASSERT_TRUE(fake_upstreams_[1]->waitForHttpConnection(
          *dispatcher_, fake_upstream_connection_issuer));
      ASSERT_TRUE(fake_upstream_connection_issuer->waitForNewStream(
          *dispatcher_, request_stream_issuer));
      ASSERT_TRUE(request_stream_issuer->waitForEndStream(*dispatcher_));

      request_stream_issuer->encodeHeaders(
          Http::HeaderMapImpl(issuer_response_headers), false);
      Buffer::OwnedImpl body(issuer_response_body);
      request_stream_issuer->encodeData(body, true);
    }

    // Valid JWT case.
    // Check if the request sent to the backend includes the expected one.
    if (verification_success) {
      ASSERT_TRUE(fake_upstreams_[0]->waitForHttpConnection(
          *dispatcher_, fake_upstream_connection_backend));
      ASSERT_TRUE(fake_upstream_connection_backend->waitForNewStream(
          *dispatcher_, request_stream_backend));
      ASSERT_TRUE(request_stream_backend->waitForEndStream(*dispatcher_));
      EXPECT_TRUE(request_stream_backend->complete());

      ExpectHeaderIncluded(expected_headers, request_stream_backend->headers());
      if (!expected_body.empty()) {
        EXPECT_EQ(expected_body,
                  InstanceToString(request_stream_backend->body()));
      }
    }

    response->waitForEndStream();

    // Invalid JWT case.
    // Check if the response sent to the client includes the expected one.
    if (!verification_success) {
      EXPECT_TRUE(response->complete());

      ExpectHeaderIncluded(expected_headers, response->headers());
      if (!expected_body.empty()) {
        EXPECT_EQ(expected_body, response->body());
      }
    }

    codec_client->close();
    if (!issuer_response_body.empty()) {
      ASSERT_TRUE(fake_upstream_connection_issuer->close());
      ASSERT_TRUE(fake_upstream_connection_issuer->waitForDisconnect());
    }
    if (verification_success) {
      ASSERT_TRUE(fake_upstream_connection_backend->close());
      ASSERT_TRUE(fake_upstream_connection_backend->waitForDisconnect());
    }
  }

 private:
  virtual std::string ConfigPath() = 0;
};

class JwtVerificationFilterIntegrationTestWithJwks
    : public JwtVerificationFilterIntegrationTest {
  std::string ConfigPath() override {
    return "src/envoy/http/jwt_auth/integration_test/envoy.conf.jwk";
  }

 protected:
  const std::string kPublicKeyRSA =
      "{\"keys\": [{\"kty\": \"RSA\",\"alg\": \"RS256\",\"use\": "
      "\"sig\",\"kid\": \"62a93512c9ee4c7f8067b5a216dade2763d32a47\",\"n\": "
      "\"0YWnm_eplO9BFtXszMRQNL5UtZ8HJdTH2jK7vjs4XdLkPW7YBkkm_"
      "2xNgcaVpkW0VT2l4mU3KftR-6s3Oa5Rnz5BrWEUkCTVVolR7VYksfqIB2I_"
      "x5yZHdOiomMTcm3DheUUCgbJRv5OKRnNqszA4xHn3tA3Ry8VO3X7BgKZYAUh9fyZTFLlkeAh"
      "0-"
      "bLK5zvqCmKW5QgDIXSxUTJxPjZCgfx1vmAfGqaJb-"
      "nvmrORXQ6L284c73DUL7mnt6wj3H6tVqPKA27j56N0TB1Hfx4ja6Slr8S4EB3F1luYhATa1P"
      "KU"
      "SH8mYDW11HolzZmTQpRoLV8ZoHbHEaTfqX_aYahIw\",\"e\": \"AQAB\"},{\"kty\": "
      "\"RSA\",\"alg\": \"RS256\",\"use\": \"sig\",\"kid\": "
      "\"b3319a147514df7ee5e4bcdee51350cc890cc89e\",\"n\": "
      "\"qDi7Tx4DhNvPQsl1ofxxc2ePQFcs-L0mXYo6TGS64CY_"
      "2WmOtvYlcLNZjhuddZVV2X88m0MfwaSA16wE-"
      "RiKM9hqo5EY8BPXj57CMiYAyiHuQPp1yayjMgoE1P2jvp4eqF-"
      "BTillGJt5W5RuXti9uqfMtCQdagB8EC3MNRuU_KdeLgBy3lS3oo4LOYd-"
      "74kRBVZbk2wnmmb7IhP9OoLc1-7-9qU1uhpDxmE6JwBau0mDSwMnYDS4G_ML17dC-"
      "ZDtLd1i24STUw39KH0pcSdfFbL2NtEZdNeam1DDdk0iUtJSPZliUHJBI_pj8M-2Mn_"
      "oA8jBuI8YKwBqYkZCN1I95Q\",\"e\": \"AQAB\"}]}";

  const std::string kPublicKeyEC =
      "{\"keys\": ["
      "{"
      "\"kty\": \"EC\","
      "\"crv\": \"P-256\","
      "\"x\": \"EB54wykhS7YJFD6RYJNnwbWEz3cI7CF5bCDTXlrwI5k\","
      "\"y\": \"92bCBTvMFQ8lKbS2MbgjT3YfmYo6HnPEE2tsAqWUJw8\","
      "\"alg\": \"ES256\","
      "\"kid\": \"abc\""
      "},"
      "{"
      "\"kty\": \"EC\","
      "\"crv\": \"P-256\","
      "\"x\": \"EB54wykhS7YJFD6RYJNnwbWEz3cI7CF5bCDTXlrwI5k\","
      "\"y\": \"92bCBTvMFQ8lKbS2MbgjT3YfmYo6HnPEE2tsAqWUJw8\","
      "\"alg\": \"ES256\","
      "\"kid\": \"xyz\""
      "}"
      "]}";
};

INSTANTIATE_TEST_SUITE_P(
    IpVersions, JwtVerificationFilterIntegrationTestWithJwks,
    testing::ValuesIn(TestEnvironment::getIpVersionsForTest()));

TEST_P(JwtVerificationFilterIntegrationTestWithJwks, RSASuccess1) {
  const std::string kJwtNoKid =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9."
      "eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIs"
      "ImF1ZCI6ImV4YW1wbGVfc2VydmljZSIsImV4cCI6MjAwMTAwMTAwMX0."
      "n45uWZfIBZwCIPiL0K8Ca3tmm-ZlsDrC79_"
      "vXCspPwk5oxdSn983tuC9GfVWKXWUMHe11DsB02b19Ow-"
      "fmoEzooTFn65Ml7G34nW07amyM6lETiMhNzyiunctplOr6xKKJHmzTUhfTirvDeG-q9n24-"
      "8lH7GP8GgHvDlgSM9OY7TGp81bRcnZBmxim_UzHoYO3_"
      "c8OP4ZX3xG5PfihVk5G0g6wcHrO70w0_64JgkKRCrLHMJSrhIgp9NHel_"
      "CNOnL0AjQKe9IGblJrMuouqYYS0zEWwmOVUWUSxQkoLpldQUVefcfjQeGjz8IlvktRa77FYe"
      "xfP590ACPyXrivtsxg";

  auto expected_headers = BaseRequestHeaders();
  // TODO: JWT payload is not longer output to header. Find way to verify that
  // data equivalent to this is added to dynamicMetadata.
  //   expected_headers.addCopy(
  //       kJwtVerificationResultHeaderKey,
  //       "eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tIiwic3ViIjoidGVz"
  //       "dEBleGFtcGxlLmNvbSIsImF1ZCI6ImV4YW1wbGVfc2VydmljZSIs"
  //       "ImV4cCI6MjAwMTAwMTAwMX0");

  TestVerification(createHeaders(kJwtNoKid), "", createIssuerHeaders(),
                   kPublicKeyRSA, true, expected_headers, "");
}

TEST_P(JwtVerificationFilterIntegrationTestWithJwks, ES256Success1) {
  // Payload:
  // {"iss": "https://example.com", "sub": "test@example.com", "aud":
  // "example_service",
  //  "exp": 2001001001}

  const std::string kJwtEC =
      "eyJhbGciOiJFUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY"
      "29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIsImV4cCI6MjAwMTAwMTAwMSwiYXVkIjo"
      "iZXhhbXBsZV9zZXJ2aWNlIn0.1Slk-zwP_78zR8Go5COxmmMunkxdTnBeeC91CgR-p2MWM"
      "T9ubWvRvNGGYOTuJ8T17Db68Qk3T8UNTK5lzfR_mw";

  auto expected_headers = BaseRequestHeaders();
  // TODO: JWT payload is not longer output to header. Find way to verify that
  // data equivalent to this is added to dynamicMetadata.
  //   expected_headers.addCopy(kJwtVerificationResultHeaderKey,
  //                            "eyJpc3MiOiJo"
  //                            "dHRwczovL2V4YW1wbGUuY29tIiwic3ViIjoidGVzdEBleGFtc"
  //                            "GxlLmNvbSIsImV4cCI6MjAwMTAwMTAwMSwiYXVkIjoiZXhhbX"
  //                            "BsZV9zZXJ2aWNlIn0");

  TestVerification(createHeaders(kJwtEC), "", createIssuerHeaders(),
                   kPublicKeyEC, true, expected_headers, "");
}

TEST_P(JwtVerificationFilterIntegrationTestWithJwks, JwtExpired) {
  const std::string kJwtNoKid =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9."
      "eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIs"
      "ImV4cCI6MTUwMTI4MTA1OH0.XYPg6VPrq-H1Kl-kgmAfGFomVpnmdZLIAo0g6dhJb2Be_"
      "koZ2T76xg5_Lr828hsLKxUfzwNxl5-k1cdz_kAst6vei0hdnOYqRQ8EhkZS_"
      "5Y2vWMrzGHw7AUPKCQvSnNqJG5HV8YdeOfpsLhQTd-"
      "tG61q39FWzJ5Ra5lkxWhcrVDQFtVy7KQrbm2dxhNEHAR2v6xXP21p1T5xFBdmGZbHFiH63N9"
      "dwdRgWjkvPVTUqxrZil7PSM2zg_GTBETp_"
      "qS7Wwf8C0V9o2KZu0KDV0j0c9nZPWTv3IMlaGZAtQgJUeyemzRDtf4g2yG3xBZrLm3AzDUj_"
      "EX_pmQAHA5ZjPVCAw";

  // Issuer is not called by passing empty pubkey.
  std::string pubkey = "";
  TestVerification(createHeaders(kJwtNoKid), "", createIssuerHeaders(), pubkey,
                   false, Http::TestHeaderMapImpl{{":status", "401"}},
                   "JWT is expired");
}

TEST_P(JwtVerificationFilterIntegrationTestWithJwks, AudInvalid) {
  // Payload:
  // {"iss":"https://example.com","sub":"test@example.com","aud":"invalid_service","exp":2001001001}
  const std::string jwt =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9."
      "eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIs"
      "ImF1ZCI6ImludmFsaWRfc2VydmljZSIsImV4cCI6MjAwMTAwMTAwMX0."
      "gEWnuqtEdVzC94lVbuClaVLoxs-w-_uKJRbAYwAKRulE-"
      "ZhxG9VtKCd8i90xEuk9txB3tT8VGjdZKs5Hf5LjF4ebobV3M9ya6mZvq1MdcUHYiUtQhJe3M"
      "t_2sxRmogK-QZ7HcuA9hpFO4HHVypnMDr4WHgxx2op1vhKU7NDlL-"
      "38Dpf6uKEevxi0Xpids9pSST4YEQjReTXJDJECT5dhk8ZQ_lcS-pujgn7kiY99bTf6j4U-"
      "ajIcWwtQtogYx4bcmHBUvEjcYOC86TRrnArZSk1mnO7OGq4KrSrqhXnvqDmc14LfldyWqEks"
      "X5FkM94prXPK0iN-pPVhRjNZ4xvR-w";

  // Issuer is not called by passing empty pubkey.
  std::string pubkey = "";
  TestVerification(createHeaders(jwt), "", createIssuerHeaders(), pubkey, false,
                   Http::TestHeaderMapImpl{{":status", "401"}},
                   "Audience doesn't match");
}

TEST_P(JwtVerificationFilterIntegrationTestWithJwks, Fail1) {
  std::string token = "invalidToken";
  // Issuer is not called by passing empty pubkey.
  std::string pubkey = "";
  TestVerification(createHeaders(token), "", createIssuerHeaders(), pubkey,
                   false, Http::TestHeaderMapImpl{{":status", "401"}},
                   "JWT_BAD_FORMAT");
}

class JwtVerificationFilterIntegrationTestWithInjectedJwtResult
    : public JwtVerificationFilterIntegrationTestWithJwks {
  // With allow_missing_or_failed option being true, a request without JWT
  // will reach the backend. This is to test the injected JWT result.
  std::string ConfigPath() override {
    return "src/envoy/http/jwt_auth/integration_test/"
           "envoy_allow_missing_or_failed_jwt.conf.jwk";
  }
};

INSTANTIATE_TEST_SUITE_P(
    IpVersions, JwtVerificationFilterIntegrationTestWithInjectedJwtResult,
    testing::ValuesIn(TestEnvironment::getIpVersionsForTest()));

TEST_P(JwtVerificationFilterIntegrationTestWithInjectedJwtResult,
       InjectedJwtResultSanitized) {
  // Create a request without JWT.
  // With allow_missing_or_failed option being true, a request without JWT
  // will reach the backend. This is to test the injected JWT result.
  auto headers = BaseRequestHeaders();
  // Inject a header of JWT verification result
  headers.addCopy(kJwtVerificationResultHeaderKey, kJwtVerificationResult);

  IntegrationCodecClientPtr codec_client;
  FakeHttpConnectionPtr fake_upstream_connection_backend;
  IntegrationStreamDecoderPtr response(
      new IntegrationStreamDecoder(*dispatcher_));
  FakeStreamPtr request_stream_backend;
  codec_client = makeHttpConnection(lookupPort("http"));
  // Send a request to Envoy.
  response = codec_client->makeHeaderOnlyRequest(headers);
  ASSERT_TRUE(fake_upstreams_[0]->waitForHttpConnection(
      *dispatcher_, fake_upstream_connection_backend));
  ASSERT_TRUE(fake_upstream_connection_backend->waitForNewStream(
      *dispatcher_, request_stream_backend));
  ASSERT_TRUE(request_stream_backend->waitForEndStream(*dispatcher_));
  EXPECT_TRUE(request_stream_backend->complete());

  response->waitForEndStream();
  codec_client->close();
  ASSERT_TRUE(fake_upstream_connection_backend->close());
  ASSERT_TRUE(fake_upstream_connection_backend->waitForDisconnect());
}

}  // namespace Envoy
