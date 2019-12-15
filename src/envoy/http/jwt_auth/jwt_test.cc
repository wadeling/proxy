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

#include "jwt.h"

#include <tuple>

#include "common/common/utility.h"
#include "common/json/json_loader.h"
#include "test/test_common/utility.h"

namespace Envoy {
namespace Http {
namespace JwtAuth {

class DatasetPem {
 public:
  // JWT with
  // Header:  {"alg":"RS256","typ":"JWT"}
  // Payload:
  // {"iss":"https://example.com","sub":"test@example.com","exp":1501281058}
  const std::string kJwt =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9."
      "eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIs"
      "ImV4cCI6MTUwMTI4MTA1OH0.FxT92eaBr9thDpeWaQh0YFhblVggn86DBpnTa_"
      "DVO4mNoGEkdpuhYq3epHPAs9EluuxdSkDJ3fCoI758ggGDw8GbqyJAcOsH10fBOrQbB7EFRB"
      "CI1xz6-6GEUac5PxyDnwy3liwC_"
      "gK6p4yqOD13EuEY5aoYkeM382tDFiz5Jkh8kKbqKT7h0bhIimniXLDz6iABeNBFouczdPf04"
      "N09hdvlCtAF87Fu1qqfwEQ93A-J7m08bZJoyIPcNmTcYGHwfMR4-lcI5cC_93C_"
      "5BGE1FHPLOHpNghLuM6-rhOtgwZc9ywupn_bBK3QzuAoDnYwpqQhgQL_CdUD_bSHcmWFkw";

  // JWT with
  // Header:  {"alg":"RS384","typ":"JWT"}
  // Payload:
  // {"iss":"https://example.com","sub":"test@example.com","exp":1501281058}
  const std::string kJwtRs384 =
      "eyJhbGciOiJSUzM4NCIsInR5cCI6IkpXVCJ9."
      "eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIs"
      "ImV4cCI6MTUwMTI4MTA1OH0.NvinWcCVmBAmbK5FnAPt8gMBSWOU9kjTEIxcDqJBzjB6nKGj"
      "sUYF05RC69F4POrJKLl3ak9LQUFPAwn732xEavbQunl-MreZCtRKrTX2xdwod0_u3gvSakcc"
      "N9kEkbXMqJ5DhFUH0Viv7oVQtbRzwB7hr0ip-Yi8RAbrKfk8qDX0bT2TOlqzbLDnIp3M5btX"
      "vO1GfOirIiz0YDfzEmSbkhZAnz4D062LWwyfIfM1ZhFusSyYBaNjib1vBfjIGsiYW-ot9dRY"
      "X0YZP1YF-XxalyUGalD6pn-5nOkd86KL8ch0OkxBpHc1XqBrrsw0Pjax6Sv-nYYUb9qN6p69"
      "q9YstA";

  // JWT with
  // Header:  {"alg":"RS512","typ":"JWT"}
  // Payload:
  // {"iss":"https://example.com","sub":"test@example.com","exp":1501281058}
  const std::string kJwtRs512 =
      "eyJhbGciOiJSUzUxMiIsInR5cCI6IkpXVCJ9."
      "eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIs"
      "ImV4cCI6MTUwMTI4MTA1OH0.BaBGWBS5ZhOX7o0LlAYqnnS-rME0E_eAjnCzPolSY5oh-Mic"
      "WFN3B1AW-iCeAW3fHf7GhlbshKoybLaj7Cj87m9T-w015WGyIBIwWKQVjfT62RJ1hrKzoyM5"
      "flVbwMPG70vqV9xfOTpZ4iZ9QomAut4yMDSBTINeeQLRVckYUN-IQVLU-bMnnvabsIQeNxhs"
      "sG6S61cOD234mGdgkxoaZhHDprvEtAaYAuuKsIlaNIbp8r5hYFv09SMjAELlneObiMI3m5IG"
      "yx3cF3odgb8PPLRBEOxD6HwJzmvbYmkjmgLuE5vb5lLEacyn2I1ko7e-Hlzvp_ezST0wknz5"
      "wadrCQ";

  // {"iss":"https://example.com","sub":"test@example.com","exp":1501281058,
  // aud: [aud1, aud2] }
  // signature part is invalid.
  const std::string kJwtMultiSub =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6ImFmMDZjMTlmOGU1YjMzMTUyMT"
      "ZkZjAxMGZkMmI5YTkzYmFjMTM1YzgifQ.eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tI"
      "iwiaWF0IjoxNTE3ODc1MDU5LCJhdWQiOlsiYXVkMSIsImF1ZDIiXSwiZXhwIjoxNTE3ODc"
      "4NjU5LCJzdWIiOiJodHRwczovL2V4YW1wbGUuY29tIn0.fzzlfQG2wZpPRRAPa6Yu";

  const std::string kJwtSub = "test@example.com";
  const std::string kJwtHeaderEncoded = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9";
  const std::string kJwtPayloadEncoded =
      "eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIs"
      "ImV4cCI6MTUwMTI4MTA1OH0";
  const std::string kJwtSignatureEncoded =
      "FxT92eaBr9thDpeWaQh0YFhblVggn86DBpnTa_"
      "DVO4mNoGEkdpuhYq3epHPAs9EluuxdSkDJ3fCoI758ggGDw8GbqyJAcOsH10fBOrQbB7EFRB"
      "CI1xz6-6GEUac5PxyDnwy3liwC_"
      "gK6p4yqOD13EuEY5aoYkeM382tDFiz5Jkh8kKbqKT7h0bhIimniXLDz6iABeNBFouczdPf04"
      "N09hdvlCtAF87Fu1qqfwEQ93A-J7m08bZJoyIPcNmTcYGHwfMR4-lcI5cC_93C_"
      "5BGE1FHPLOHpNghLuM6-rhOtgwZc9ywupn_bBK3QzuAoDnYwpqQhgQL_CdUD_bSHcmWFkw";
  const std::string kJwtPayload =
      R"EOF({"iss":"https://example.com","sub":"test@example.com","exp":1501281058})EOF";

  const std::string kPublicKey =
      "MIIBCgKCAQEAtw7MNxUTxmzWROCD5BqJxmzT7xqc9KsnAjbXCoqEEHDx4WBlfcwk"
      "XHt9e/2+Uwi3Arz3FOMNKwGGlbr7clBY3utsjUs8BTF0kO/poAmSTdSuGeh2mSbc"
      "VHvmQ7X/kichWwx5Qj0Xj4REU3Gixu1gQIr3GATPAIULo5lj/ebOGAa+l0wIG80N"
      "zz1pBtTIUx68xs5ZGe7cIJ7E8n4pMX10eeuh36h+aossePeuHulYmjr4N0/1jG7a"
      "+hHYL6nqwOR3ej0VqCTLS0OloC0LuCpLV7CnSpwbp2Qg/c+MDzQ0TH8g8drIzR5h"
      "Fe9a3NlNRMXgUU5RqbLnR9zfXr7b9oEszQIDAQAB";

  //  private key:
  //      "-----BEGIN RSA PRIVATE KEY-----"
  //      "MIIEowIBAAKCAQEAtw7MNxUTxmzWROCD5BqJxmzT7xqc9KsnAjbXCoqEEHDx4WBl"
  //      "fcwkXHt9e/2+Uwi3Arz3FOMNKwGGlbr7clBY3utsjUs8BTF0kO/poAmSTdSuGeh2"
  //      "mSbcVHvmQ7X/kichWwx5Qj0Xj4REU3Gixu1gQIr3GATPAIULo5lj/ebOGAa+l0wI"
  //      "G80Nzz1pBtTIUx68xs5ZGe7cIJ7E8n4pMX10eeuh36h+aossePeuHulYmjr4N0/1"
  //      "jG7a+hHYL6nqwOR3ej0VqCTLS0OloC0LuCpLV7CnSpwbp2Qg/c+MDzQ0TH8g8drI"
  //      "zR5hFe9a3NlNRMXgUU5RqbLnR9zfXr7b9oEszQIDAQABAoIBAQCgQQ8cRZJrSkqG"
  //      "P7qWzXjBwfIDR1wSgWcD9DhrXPniXs4RzM7swvMuF1myW1/r1xxIBF+V5HNZq9tD"
  //      "Z07LM3WpqZX9V9iyfyoZ3D29QcPX6RGFUtHIn5GRUGoz6rdTHnh/+bqJ92uR02vx"
  //      "VPD4j0SNHFrWpxcE0HRxA07bLtxLgNbzXRNmzAB1eKMcrTu/W9Q1zI1opbsQbHbA"
  //      "CjbPEdt8INi9ij7d+XRO6xsnM20KgeuKx1lFebYN9TKGEEx8BCGINOEyWx1lLhsm"
  //      "V6S0XGVwWYdo2ulMWO9M0lNYPzX3AnluDVb3e1Yq2aZ1r7t/GrnGDILA1N2KrAEb"
  //      "AAKHmYNNAoGBAPAv9qJqf4CP3tVDdto9273DA4Mp4Kjd6lio5CaF8jd/4552T3UK"
  //      "N0Q7N6xaWbRYi6xsCZymC4/6DhmLG/vzZOOhHkTsvLshP81IYpWwjm4rF6BfCSl7"
  //      "ip+1z8qonrElxes68+vc1mNhor6GGsxyGe0C18+KzpQ0fEB5J4p0OHGnAoGBAMMb"
  //      "/fpr6FxXcjUgZzRlxHx1HriN6r8Jkzc+wAcQXWyPUOD8OFLcRuvikQ16sa+SlN4E"
  //      "HfhbFn17ABsikUAIVh0pPkHqMsrGFxDn9JrORXUpNhLdBHa6ZH+we8yUe4G0X4Mc"
  //      "R7c8OT26p2zMg5uqz7bQ1nJ/YWlP4nLqIytehnRrAoGAT6Rn0JUlsBiEmAylxVoL"
  //      "mhGnAYAKWZQ0F6/w7wEtPs/uRuYOFM4NY1eLb2AKLK3LqqGsUkAQx23v7PJelh2v"
  //      "z3bmVY52SkqNIGGnJuGDaO5rCCdbH2EypyCfRSDCdhUDWquSpBv3Dr8aOri2/CG9"
  //      "jQSLUOtC8ouww6Qow1UkPjMCgYB8kTicU5ysqCAAj0mVCIxkMZqFlgYUJhbZpLSR"
  //      "Tf93uiCXJDEJph2ZqLOXeYhMYjetb896qx02y/sLWAyIZ0ojoBthlhcLo2FCp/Vh"
  //      "iOSLot4lOPsKmoJji9fei8Y2z2RTnxCiik65fJw8OG6mSm4HeFoSDAWzaQ9Y8ue1"
  //      "XspVNQKBgAiHh4QfiFbgyFOlKdfcq7Scq98MA3mlmFeTx4Epe0A9xxhjbLrn362+"
  //      "ZSCUhkdYkVkly4QVYHJ6Idzk47uUfEC6WlLEAnjKf9LD8vMmZ14yWR2CingYTIY1"
  //      "LL2jMkSYEJx102t2088meCuJzEsF3BzEWOP8RfbFlciT7FFVeiM4"
  //      "-----END RSA PRIVATE KEY-----";

  /*
   * jwt with header replaced by
   * "{"alg":"RS256","typ":"JWT", this is a invalid json}"
   */
  const std::string kJwtWithBadJsonHeader =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsIHRoaXMgaXMgYSBpbnZhbGlkIGpzb259."
      "eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIs"
      "ImV4cCI6MTUwMTI4MTA1OH0."
      "ERgdOJdVCrUAaAIMaAG6rgAR7M6ZJUjvKxIMgb9jrfsEVzsetb4UlPsrO-FBA4LUT_"
      "xIshL4Bzd0_3w63v7xol2-iAQgW_7PQeeEEqqMcyfkuXEhHu_lXawAlpqKhCmFuyIeYBiSs-"
      "RRIqHCutIJSBfcIGLMRcVzpMODfwMMlzjw6SlfMuy68h54TpBt1glvwEg71lVVO7IE3Fxwgl"
      "EDR_2MrVwjmyes9TmDgsj_zBHHn_d09kVqV_adYXtVec9fyo7meODQXB_"
      "eWm065WsSRFksQn8fidWtrAfdcSzYe2wN0doP-QYzJeWKll15XVRKS67NeENz40Wd_Tr_"
      "tyHBHw";

  /*
   * jwt with payload replaced by
   * "this is not a json"
   */
  const std::string kJwtWithBadJsonPayload =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.dGhpcyBpcyBub3QgYSBqc29u."
      "NhQjffMuBkYA9MXq3Fi3h2RRR6vNsYHOjF22GRHRcAEsTHJGYpWsU0MpkWnSJ04Ktx6PFp8f"
      "jRUI0bLtLC2R2Nv3VQNfvcZy0cJmlEWGZbRjEA2AwOaMpiKX-6z5BtMic9hG5Aw1IDxf_"
      "ZvqiE5nRxPBnMXxsINgJ1veTd0zBhOsr0Y3Onl2O3UJSqrQn4kSqjpTENODjSJcJcfiy15sU"
      "MX7wCiP_FSjLAW-"
      "mcaa8RdV49LegwB185eK9UmTJ98yTqEN7w9wcKkZFe8vpojkJX8an0EjGOTJ_5IsU1A_"
      "Xv1Z1ZQYGTOEsMH8j9zWslYTRq15bDIyALHRD46UHqjDSQ";

  /*
   * jwt with header replaced by
   * "{"typ":"JWT"}"
   */
  const std::string kJwtWithAlgAbsent =
      "eyJ0eXAiOiJKV1QifQ."
      "eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIs"
      "ImV4cCI6MTUwMTI4MTA1OH0"
      "."
      "MGJmMDU2YjViZmJhMzE5MGI3MTRiMmE4NDhiMmIzNzI2Mzk3MGUwOGVmZTAwMzc0YzY4MWFj"
      "NzgzMDZjZWRlYgoyZWY3Mzk2NWE2MjYxZWI2M2FhMGFjM2E1NDQ1MjNmMjZmNjU2Y2MxYWIz"
      "YTczNGFlYTg4ZDIyN2YyZmM4YTI5CjM5OTQwNjI2ZjI3ZDlmZTM4M2JjY2NhZjIxMmJlY2U5"
      "Y2Q3NGY5YmY2YWY2ZDI2ZTEyNDllMjU4NGVhZTllMGQKMzg0YzVlZmUzY2ZkMWE1NzM4YTIw"
      "MzBmYTQ0OWY0NDQ1MTNhOTQ4NTRkMzgxMzdkMTljMWQ3ZmYxYjNlMzJkMQoxMGMyN2VjZDQ5"
      "MTMzNjZiZmE4Zjg3ZTEyNWQzMGEwYjhhYjUyYWE5YzZmZTcwM2FmZDliMjkzODY3OWYxNWQy"
      "CjZiNWIzZjgzYTk0Zjg1MjFkMDhiNmYyNzY1MTM1N2MyYWI0MzBkM2FlYjg5MTFmNjM5ZGNj"
      "MGM2NTcxNThmOWUKOWQ1ZDM2NWFkNGVjOTgwYmNkY2RiMDUzM2MzYjY2MjJmYWJiMDViNjNk"
      "NjIxMDJiZDkyZDE3ZjZkZDhiMTBkOQo1YjBlMDRiZWU2MDBjNjRhNzM0ZGE1ZGY2YjljMGY5"
      "ZDM1Mzk3MjcyNDcyN2RjMTViYjk1MjMwZjdmYmU5MzYx";

  /*
   * jwt with header replaced by
   * "{"alg":256,"typ":"JWT"}"
   */
  const std::string kJwtWithAlgIsNotString =
      "eyJhbGciOjI1NiwidHlwIjoiSldUIn0."
      "eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIs"
      "ImV4cCI6MTUwMTI4MTA1OH0."
      "ODYxMDhhZjY5MjEyMGQ4ZjE5YzMzYmQzZDY3MmE1NjFjNDM1NzdhYmNhNDM0Njg2MWMwNGI4"
      "ZDNhZDExNjUxZgphZTU0ZjMzZWVmMWMzYmQyOTEwNGIxNTA3ZDllZTI0ZmY0OWFkZTYwNGUz"
      "MGUzMWIxN2MwMTIzNTY0NDYzNjBlCjEyZDk3ZGRiMDAwZDgwMDFmYjcwOTIzZDYzY2VhMzE1"
      "MjcyNzdlY2RhYzZkMWU5MThmOThjOTFkNTZiM2NhYWIKNjA0ZThiNWI4N2MwNWM4M2RkODE4"
      "NWYwNDBiYjY4Yjk3MmY5MDc2YmYzYTk3ZjM0OWVhYjA1ZTdjYzdhOGEzZApjMGU4Y2Y0MzJl"
      "NzY2MDAwYTQ0ZDg1ZmE5MjgzM2ExNjNjOGM3OTllMTEyNTM5OWMzYzY3MThiMzY2ZjU5YTVl"
      "CjVjYTdjZTBmNDdlMjZhYjU3M2Y2NDI4ZmRmYzQ2N2NjZjQ4OWFjNTA1OTRhM2NlYTlhNTE1"
      "ODJhMDE1ODA2YzkKZmRhNTFmODliNTk3NjA4Njg2NzNiMDUwMzdiY2IzOTQzMzViYzU2YmFk"
      "ODUyOWIwZWJmMjc1OTkxMTkzZjdjMwo0MDFjYWRlZDI4NjA2MmNlZTFhOGU3YWFiMDJiNjcy"
      "NGVhYmVmMjA3MGQyYzFlMmY3NDRiM2IyNjU0MGQzZmUx";

  /*
   * jwt with header replaced by
   * "{"alg":"InvalidAlg","typ":"JWT"}"
   */
  const std::string kJwtWithInvalidAlg =
      "eyJhbGciOiJJbnZhbGlkQWxnIiwidHlwIjoiSldUIn0."
      "eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIs"
      "ImV4cCI6MTUwMTI4MTA1OH0."
      "MjQ3ZThmMTQ1YWYwZDIyZjVlZDlhZTJhOWIzYWI2OGY5ZTcyZWU1ZmJlNzA1ODE2NjkxZDU3"
      "OGY0MmU0OTlhNgpiMmY0NmM2OTI3Yjc1Mjk3NDdhYTQyZTY3Mjk2NGY0MzkzMzgwMjhlNjE2"
      "ZDk2YWM4NDQwZTQ1MGRiYTM5NjJmCjNlODU0YjljOTNjOTg4YTZmNjVkNDhiYmViNTBkZTg5"
      "NWZjOGNmM2NmY2I0MGY1MmU0YjQwMWFjYWZlMjU0M2EKMzc3MjU2YzgyNmZlODIxYTgxNDZm"
      "ZDZkODhkZjg3Yzg1MjJkYTM4MWI4MmZiNTMwOGYxODAzMGZjZGNjMjAxYgpmYmM2NzRiZGQ5"
      "YWMyNzYwZDliYzBlMTMwMDA2OTE3MTBmM2U5YmZlN2Y4OGYwM2JjMWFhNTAwZTY2ZmVhMDk5"
      "CjlhYjVlOTFiZGVkNGMxZTBmMzBiNTdiOGM0MDY0MGViNjMyNTE2Zjc5YTczNzM0YTViM2M2"
      "YjAxMGQ4MjYyYmUKM2U1MjMyMTE4MzUxY2U5M2VkNmY1NWJhYTFmNmU5M2NmMzVlZjJiNjRi"
      "MDYxNzU4YWJmYzdkNzUzYzAxMWVhNgo3NTg1N2MwMGY3YTE3Y2E3YWI2NGJlMWIyYjdkNzZl"
      "NWJlMThhZWFmZWY5NDU5MjAxY2RkY2NkZGZiZjczMjQ2";

  /*
   * jwt with header replaced by
   * "{"alg":"ES256","typ":"JWT"}"
   */
  const std::string kJwtWithES256Alg =
      "eyJhbGciOiJFUzI1NiIsImtpZCI6IjYyYTkzNTEyYzllZTRjN2Y4MDY3YjVhMjE2ZGFkZTI3"
      "NjNkMzJhNDciLCJ0eXAiOiJKV1QifQ.eyJleHAiOjE1NzE0MTkyNTIsImZvbyI6ImJsYWJsY"
      "SIsImlhdCI6MTU2MTQxOTI1MiwiaXNzIjoidGVzdGluZ0BzZWN1cmUuaXN0aW8uaW8iLCJzd"
      "WIiOiJ0ZXN0aW5nQHNlY3VyZS5pc3Rpby5pbyJ9.JJnYan0ItEmTSPC9sETO5j46Ve0yQkC0"
      "_4uEyfShbhDzejhVavlUdrL5sE2JEq9W-SYUhwGt2eIPMxKl1E1sQn0a_4f6iU6ZxhXnXU91"
      "g2SB8-JF6wrc_I3iybrUrj39kxUZQNr-w8MRp1YBDMmKg1har98AeL0xHzdyF_gf3K57u-9_"
      "yyBoymCjQraMQPWX-MuOI18i7w9MmwfIplxD3sGpnivAma1hSAJWfRFuz_rHst08cZOl_6ZK"
      "8ineqqYL19lHLLJns3dzYIvVxdOdRs87Z5UwCyYjLlxupiLo6MHFBWNMFNgZ"
      "is7wsUauWH47D-ga0JjcmVL4MRgyoP43mA";
};

class DatasetJwk {
 public:
  // The following public key jwk and token are taken from
  // https://github.com/cloudendpoints/esp/blob/master/src/api_manager/auth/lib/auth_jwt_validator_test.cc
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

  //  private key:
  //      "-----BEGIN PRIVATE KEY-----\n"
  //      "MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCoOLtPHgOE289C\n"
  //      "yXWh/HFzZ49AVyz4vSZdijpMZLrgJj/ZaY629iVws1mOG511lVXZfzybQx/BpIDX\n"
  //      "rAT5GIoz2GqjkRjwE9ePnsIyJgDKIe5A+nXJrKMyCgTU/aO+nh6oX4FOKWUYm3lb\n"
  //      "lG5e2L26p8y0JB1qAHwQLcw1G5T8p14uAHLeVLeijgs5h37viREFVluTbCeaZvsi\n"
  //      "E/06gtzX7v72pTW6GkPGYTonAFq7SYNLAydgNLgb8wvXt0L5kO0t3WLbhJNTDf0o\n"
  //      "fSlxJ18VsvY20Rl015qbUMN2TSJS0lI9mWJQckEj+mPwz7Yyf+gDyMG4jxgrAGpi\n"
  //      "RkI3Uj3lAgMBAAECggEAOuaaVyp4KvXYDVeC07QTeUgCdZHQkkuQemIi5YrDkCZ0\n"
  //      "Zsi6CsAG/f4eVk6/BGPEioItk2OeY+wYnOuDVkDMazjUpe7xH2ajLIt3DZ4W2q+k\n"
  //      "v6WyxmmnPqcZaAZjZiPxMh02pkqCNmqBxJolRxp23DtSxqR6lBoVVojinpnIwem6\n"
  //      "xyUl65u0mvlluMLCbKeGW/K9bGxT+qd3qWtYFLo5C3qQscXH4L0m96AjGgHUYW6M\n"
  //      "Ffs94ETNfHjqICbyvXOklabSVYenXVRL24TOKIHWkywhi1wW+Q6zHDADSdDVYw5l\n"
  //      "DaXz7nMzJ2X7cuRP9zrPpxByCYUZeJDqej0Pi7h7ZQKBgQDdI7Yb3xFXpbuPd1VS\n"
  //      "tNMltMKzEp5uQ7FXyDNI6C8+9TrjNMduTQ3REGqEcfdWA79FTJq95IM7RjXX9Aae\n"
  //      "p6cLekyH8MDH/SI744vCedkD2bjpA6MNQrzNkaubzGJgzNiZhjIAqnDAD3ljHI61\n"
  //      "NbADc32SQMejb6zlEh8hssSsXwKBgQDCvXhTIO/EuE/y5Kyb/4RGMtVaQ2cpPCoB\n"
  //      "GPASbEAHcsRk+4E7RtaoDQC1cBRy+zmiHUA9iI9XZyqD2xwwM89fzqMj5Yhgukvo\n"
  //      "XMxvMh8NrTneK9q3/M3mV1AVg71FJQ2oBr8KOXSEbnF25V6/ara2+EpH2C2GDMAo\n"
  //      "pgEnZ0/8OwKBgFB58IoQEdWdwLYjLW/d0oGEWN6mRfXGuMFDYDaGGLuGrxmEWZdw\n"
  //      "fzi4CquMdgBdeLwVdrLoeEGX+XxPmCEgzg/FQBiwqtec7VpyIqhxg2J9V2elJS9s\n"
  //      "PB1rh9I4/QxRP/oO9h9753BdsUU6XUzg7t8ypl4VKRH3UCpFAANZdW1tAoGAK4ad\n"
  //      "tjbOYHGxrOBflB5wOiByf1JBZH4GBWjFf9iiFwgXzVpJcC5NHBKL7gG3EFwGba2M\n"
  //      "BjTXlPmCDyaSDlQGLavJ2uQar0P0Y2MabmANgMkO/hFfOXBPtQQe6jAfxayaeMvJ\n"
  //      "N0fQOylUQvbRTodTf2HPeG9g/W0sJem0qFH3FrECgYEAnwixjpd1Zm/diJuP0+Lb\n"
  //      "YUzDP+Afy78IP3mXlbaQ/RVd7fJzMx6HOc8s4rQo1m0Y84Ztot0vwm9+S54mxVSo\n"
  //      "6tvh9q0D7VLDgf+2NpnrDW7eMB3n0SrLJ83Mjc5rZ+wv7m033EPaWSr/TFtc/MaF\n"
  //      "aOI20MEe3be96HHuWD3lTK0=\n"
  //      "-----END PRIVATE KEY-----";

  // JWT payload JSON
  const std::string kJwtPayload =
      R"EOF({"iss":"https://example.com","sub":"test@example.com","exp":1501281058})EOF";

  // JWT without kid
  // Header:  {"alg":"RS256","typ":"JWT"}
  // Payload:
  // {"iss":"https://example.com","sub":"test@example.com","exp":1501281058}
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

  // JWT payload JSON with long exp
  const std::string kJwtPayloadLongExp =
      R"EOF({"iss":"https://example.com","sub":"test@example.com","aud":"example_service","exp":2001001001})EOF";

  // JWT without kid with long exp
  // Header:  {"alg":"RS256","typ":"JWT"}
  // Payload:
  // {"iss":"https://example.com","sub":"test@example.com","aud":"example_service","exp":2001001001}
  const std::string kJwtNoKidLongExp =
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
  // JWT with correct kid
  // Header:
  // {"alg":"RS256","typ":"JWT","kid":"b3319a147514df7ee5e4bcdee51350cc890cc89e"}
  // Payload:
  // {"iss":"https://example.com","sub":"test@example.com","exp":1501281058}
  const std::string kJwtWithCorrectKid =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6ImIzMzE5YTE0NzUxNGRmN2VlNWU0"
      "YmNkZWU1MTM1MGNjODkwY2M4OWUifQ."
      "eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIs"
      "ImV4cCI6MTUwMTI4MTA1OH0.QYWtQR2JNhLBJXtpJfFisF0WSyzLbD-9dynqwZt_"
      "KlQZAIoZpr65BRNEyRzpt0jYrk7RA7hUR2cS9kB3AIKuWA8kVZubrVhSv_fiX6phjf_"
      "bZYj92kDtMiPJf7RCuGyMgKXwwf4b1Sr67zamcTmQXf26DT415rnrUHVqTlOIW50TjNa1bbO"
      "fNyKZC3LFnKGEzkfaIeXYdGiSERVOTtOFF5cUtZA2OVyeAT3mE1NuBWxz0v7xJ4zdIwHwxFU"
      "wd_5tB57j_"
      "zCEC9NwnwTiZ8wcaSyMWc4GJUn4bJs22BTNlRt5ElWl6RuBohxZA7nXwWig5CoLZmCpYpb8L"
      "fBxyCpqJQ";

  // JWT with existing but incorrect kid
  // Header:
  // {"alg":"RS256","typ":"JWT","kid":"62a93512c9ee4c7f8067b5a216dade2763d32a47"}
  // Payload:
  // {"iss":"https://example.com","sub":"test@example.com","exp":1501281058}
  const std::string kJwtWithIncorrectKid =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6IjYyYTkzNTEyYzllZTRjN2Y4MDY3"
      "YjVhMjE2ZGFkZTI3NjNkMzJhNDcifQ."
      "eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIs"
      "ImV4cCI6MTUwMTI4MTA1OH0."
      "adrKqsjKh4zdOuw9rMZr0Kn2LLYG1OUfDuvnO6tk75NKCHpKX6oI8moNYhgcCQU4AoCKXZ_"
      "u-oMl54QTx9lX9xZ2VUWKTxcJEOnpoJb-DVv_FgIG9ETe5wcCS8Y9pQ2-hxtO1_LWYok1-"
      "A01Q4929u6WNw_Og4rFXR6VSpZxXHOQrEwW44D2-Lngu1PtPjWIz3rO6cOiYaTGCS6-"
      "TVeLFnB32KQg823WhFhWzzHjhYRO7NOrl-IjfGn3zYD_"
      "DfSoMY3A6LeOFCPp0JX1gcKcs2mxaF6e3LfVoBiOBZGvgG_"
      "jx3y85hF2BZiANbSf1nlLQFdjk_CWbLPhTWeSfLXMOg";

  // JWT with nonexist kid
  // Header:  {"alg":"RS256","typ":"JWT","kid":"blahblahblah"}
  // Payload:
  // {"iss":"https://example.com","sub":"test@example.com","exp":1501281058}
  const std::string kJwtWithNonExistKid =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6ImJsYWhibGFoYmxhaCJ9."
      "eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIs"
      "ImV4cCI6MTUwMTI4MTA1OH0.digk0Fr_IdcWgJNVyeVDw2dC1cQG6LsHwg5pIN93L4_"
      "xhEDI3ZFoZ8aE44kvQHWLicnHDlhELqtF-"
      "TqxrhfnitpLE7jiyknSu6NVXxtRBcZ3dOTKryVJDvDXcYXOaaP8infnh82loHfhikgg1xmk9"
      "rcH50jtc3BkxWNbpNgPyaAAE2tEisIInaxeX0gqkwiNVrLGe1hfwdtdlWFL1WENGlyniQBvB"
      "Mwi8DgG_F0eyFKTSRWoaNQQXQruEK0YIcwDj9tkYOXq8cLAnRK9zSYc5-"
      "15Hlzfb8eE77pID0HZN-Axeui4IY22I_kYftd0OEqlwXJv_v5p6kNaHsQ9QbtAkw";

  // JWT with bad-formatted kid
  // Header:  {"alg":"RS256","typ":"JWT","kid":1}
  // Payload:
  // {"iss":"https://example.com","sub":"test@example.com","exp":1501281058}
  const std::string kJwtWithBadFormatKid =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6MX0."
      "eyJpc3MiOiJodHRwczovL2V4YW1wbGUuY29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIs"
      "ImV4cCI6MTUwMTI4MTA1OH0."
      "oYq0UkokShprH2YO5b84CI5fEu0sKWmEJimyJQ9YZbvaGtf6zaLbdVJBTbh6plBno-"
      "miUhjqXZtDdmBexQzp5HPHoIUwQxlGggCuJRdEnmw65Ul9WFWtS7M9g8DqVKaCo9MO-"
      "apCsylPZsRSzzZuaTPorZktELt6XcUIxeXOKOSZJ78sHsRrDeLhlELd9Q0b6hzAdDEYCvYE6"
      "woc3DiRHk19nsEgdg5O1RWKjTAcdd3oD9ecznzvVmAZT8gXrGXPd49tn1qHkVr1G621Ypi9V"
      "37BD2KXH3jN9_EBocxwcxhkPwSLtP3dgkfls_f5GoWCgmp-c5ycIskCDcIjxRnPjg";

  // JWT payload JSON with ES256
  const std::string kJwtPayloadEC =
      R"EOF({"iss":"628645741881-noabiu23f5a8m8ovd8ucv698lj78vv0l@developer.gserviceaccount.com",
      "sub":"628645741881-noabiu23f5a8m8ovd8ucv698lj78vv0l@developer.gserviceaccount.com",
      "aud":"http://myservice.com/myapi"})EOF";

  // Please see jwt_generator.py and jwk_generator.py under /tools/.
  // for ES256-signed jwt token and public jwk generation, respectively.
  // jwt_generator.py uses ES256 private key file to generate JWT token.
  // ES256 private key file can be generated by:
  // $ openssl ecparam -genkey -name prime256v1 -noout -out private_key.pem
  // jwk_generator.py uses ES256 public key file to generate JWK. ES256
  // public key file can be generated by:
  // $ openssl ec -in private_key.pem -pubout -out public_key.pem.

  // ES256 private key:
  // "-----BEGIN EC PRIVATE KEY-----"
  // "MHcCAQEEIOyf96eKdFeSFYeHiM09vGAylz+/auaXKEr+fBZssFsJoAoGCCqGSM49"
  // "AwEHoUQDQgAEEB54wykhS7YJFD6RYJNnwbWEz3cI7CF5bCDTXlrwI5n3ZsIFO8wV"
  // "DyUptLYxuCNPdh+Zijoec8QTa2wCpZQnDw=="
  // "-----END EC PRIVATE KEY-----"

  // ES256 public key:
  // "-----BEGIN PUBLIC KEY-----"
  // "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEEB54wykhS7YJFD6RYJNnwbWEz3cI"
  // "7CF5bCDTXlrwI5n3ZsIFO8wVDyUptLYxuCNPdh+Zijoec8QTa2wCpZQnDw=="
  // "-----END PUBLIC KEY-----"

  const std::string kPublicKeyJwkEC =
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

  // "{"kid":"abc"}"
  const std::string kTokenEC =
      "eyJhbGciOiJFUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6ImFiYyJ9.eyJpc3MiOiI2Mj"
      "g2NDU3NDE4ODEtbm9hYml1MjNmNWE4bThvdmQ4dWN2Njk4bGo3OHZ2MGxAZGV2ZWxvc"
      "GVyLmdzZXJ2aWNlYWNjb3VudC5jb20iLCJzdWIiOiI2Mjg2NDU3NDE4ODEtbm9hYml1"
      "MjNmNWE4bThvdmQ4dWN2Njk4bGo3OHZ2MGxAZGV2ZWxvcGVyLmdzZXJ2aWNlYWNjb3V"
      "udC5jb20iLCJhdWQiOiJodHRwOi8vbXlzZXJ2aWNlLmNvbS9teWFwaSJ9.T2KAwChqg"
      "o2ZSXyLh3IcMBQNSeRZRe5Z-MUDl-s-F99XGoyutqA6lq8bKZ6vmjZAlpVG8AGRZW9J"
      "Gp9lq3cbEw";

  // "{"kid":"abcdef"}"
  const std::string kJwtWithNonExistKidEC =
      "eyJhbGciOiJFUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6ImFiY2RlZiJ9.eyJpc3MiOi"
      "I2Mjg2NDU3NDE4ODEtbm9hYml1MjNmNWE4bThvdmQ4dWN2Njk4bGo3OHZ2MGxAZ"
      "GV2ZWxvcGVyLmdzZXJ2aWNlYWNjb3VudC5jb20iLCJzdWIiOiI2Mjg2NDU3NDE4"
      "ODEtbm9hYml1MjNmNWE4bThvdmQ4dWN2Njk4bGo3OHZ2MGxAZGV2ZWxvcGVyLmd"
      "zZXJ2aWNlYWNjb3VudC5jb20iLCJhdWQiOiJodHRwOi8vbXlzZXJ2aWNlLmNvbS"
      "9teWFwaSJ9.rWSoOV5j7HxHc4yVgZEZYUSgY7AUarG3HxdfPON1mw6II_pNUsc8"
      "_sVf7Yv2-jeVhmf8BtR99wnOwEDhVYrVpQ";

  const std::string kTokenECNoKid =
      "eyJhbGciOiJFUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiI2Mjg2NDU3NDE4ODEtbm"
      "9hYml1MjNmNWE4bThvdmQ4dWN2Njk4bGo3OHZ2MGxAZGV2ZWxvcGVyLmdzZXJ2a"
      "WNlYWNjb3VudC5jb20iLCJzdWIiOiI2Mjg2NDU3NDE4ODEtbm9hYml1MjNmNWE4"
      "bThvdmQ4dWN2Njk4bGo3OHZ2MGxAZGV2ZWxvcGVyLmdzZXJ2aWNlYWNjb3VudC5"
      "jb20iLCJhdWQiOiJodHRwOi8vbXlzZXJ2aWNlLmNvbS9teWFwaSJ9.zlFcET8Fi"
      "OYcKe30A7qOD4TIBvtb9zIVhDcM8pievKs1Te-UOBcklQxhwXMnRSSEBY4P0pfZ"
      "qWJT_V5IVrKrdQ";

  const std::string kBadPublicKeyRSA =
      "{\n"
      " \"keys\": [\n"
      " {\n"
      " \"alg\": \"RS256\",\n"
      " \"kty\": \"RSA\",\n"
      " \"use\": \"sig\",\n"
      " \"x5c\": "
      "[\"MIIDjjCCAnYCCQDM2dGMrJDL3TANBgkqhkiG9w0BAQUFADCBiDEVMBMGA1UEAwwMd3d3L"
      "mRlbGwuY29tMQ0wCwYDVQQKDARkZWxsMQ0wCwYDVQQLDARkZWxsMRIwEAYDVQQHDAlCYW5nY"
      "WxvcmUxEjAQBgNVBAgMCUthcm5hdGFrYTELMAkGA1UEBhMCSU4xHDAaBgkqhkiG9w0BCQEWD"
      "WFiaGlAZGVsbC5jb20wHhcNMTkwNjI1MDcwNjM1WhcNMjAwNjI0MDcwNjM1WjCBiDEVMBMGA"
      "1UEAwwMd3d3LmRlbGwuY29tMQ0wCwYDVQQKDARkZWxsMQ0wCwYDVQQLDARkZWxsMRIwEAYDV"
      "QQHDAlCYW5nYWxvcmUxEjAQBgNVBAgMCUthcm5hdGFrYTELMAkGA1UEBhMCSU4xHDAaBgkqh"
      "kiG9w0BCQEWDWFiaGlAZGVsbC5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBA"
      "QDlE7W15NCXoIZX+"
      "uE7HF0LTnfgBpaqoYyQFDmVUNEd0WWV9nX04c3iyxZSpoTsoUZktNd0CUyC8oVRg2xxdPxA2"
      "aRVpNMwsDkuDnOZPNZZCS64QmMD7V5ebSAi4vQ7LH6zo9DCVwjzW10ZOZ3WHAyoKuNVGeb5w"
      "2+xDQM1mFqApy6KB7M/b3KG7cqpZfPn9Ebd1Uyk+8WY/"
      "IxJvb7EHt06Z+8b3F+LkRp7UI4ykkVkl3XaiBlG56ZyHfvH6R5Jy+"
      "8P0vl4wtX86N6MS48TZPhGAoo2KwWsOEGxve005ZK6LkHwxMsOD98yvLM7AG0SBxVF8O8KeZ"
      "/nbTP1oVSq6aEFAgMBAAEwDQYJKoZIhvcNAQEFBQADggEBAGEhT6xuZqyZb/"
      "K6aI61RYy4tnR92d97H+zcL9t9/"
      "8FyH3qIAjIM9+qdr7dLLnVcNMmwiKzZpsBywno72z5gG4l6/TicBIJfI2BaG9JVdU3/"
      "wscPlqazwI/"
      "d1LvIkWSzrFQ2VdTPSYactPzGWddlx9QKU9cIKcNPcWdg0S0q1Khu8kejpJ+"
      "EUtSMc8OonFV99r1juFzVPtwGihuc6R7T/"
      "GnWgYLmhoCCaQKdLWn7FIyQH2WZ10CI6as+"
      "zKkylDkVnbsJYFabvbgRrNNl4RGXXm5D0lk9cwo1Srd28wEhi35b8zb1p0eTamS6qTpjHtc6"
      "DpgZK3MavFVdaFfR9bEYpHc=\"],\n"
      " \"n\": "
      "\"5RO1teTQl6CGV/"
      "rhOxxdC0534AaWqqGMkBQ5lVDRHdFllfZ19OHN4ssWUqaE7KFGZLTXdAlMgvKFUYNscXT8QN"
      "mkVaTTMLA5Lg5zmTzWWQkuuEJjA+1eXm0gIuL0Oyx+s6PQwlcI81tdGTmd1hwMqCrjVRnm+"
      "cNvsQ0DNZhagKcuigezP29yhu3KqWXz5/"
      "RG3dVMpPvFmPyMSb2+xB7dOmfvG9xfi5Eae1COMpJFZJd12ogZRuemch37x+"
      "keScvvD9L5eMLV/OjejEuPE2T4RgKKNisFrDhBsb3tNOWSui5B8MTLDg/"
      "fMryzOwBtEgcVRfDvCnmf520z9aFUqumhBQ==\",\n"
      " \"e\": \"AQAB\",\n"
      " \"kid\": \"F46BB2F600BF3BBB53A324F12B290846\",\n"
      " \"x5t\": \"F46BB2F600BF3BBB53A324F12B290846\"\n"
      " }\n"
      " ]\n"
      "}";
};

namespace {

bool EqJson(Json::ObjectSharedPtr p1, Json::ObjectSharedPtr p2) {
  return p1->asJsonString() == p2->asJsonString();
}
}  // namespace

class JwtTest : public testing::Test {
 protected:
  void DoTest(std::string jwt_str, std::string pkey, std::string pkey_type,
              bool verified, Status status, Json::ObjectSharedPtr payload) {
    Jwt jwt(jwt_str);
    Verifier v;
    std::unique_ptr<Pubkeys> key;
    if (pkey_type == "pem") {
      key = Pubkeys::CreateFrom(pkey, Pubkeys::Type::PEM);
    } else if (pkey_type == "jwks") {
      key = Pubkeys::CreateFrom(pkey, Pubkeys::Type::JWKS);
    } else {
      ASSERT_TRUE(0);
    }
    EXPECT_EQ(verified, v.Verify(jwt, *key));
    EXPECT_EQ(status, v.GetStatus());
    if (verified) {
      ASSERT_TRUE(jwt.Payload());
      EXPECT_TRUE(EqJson(payload, jwt.Payload()));
    }
  }
};

// Test cases w/ PEM-formatted public key

class JwtTestPem : public JwtTest {
 protected:
  DatasetPem ds;
};

TEST_F(JwtTestPem, OK) {
  auto payload = Json::Factory::loadFromString(ds.kJwtPayload);
  DoTest(ds.kJwt, ds.kPublicKey, "pem", true, Status::OK, payload);
}

TEST_F(JwtTestPem, OKWithAlgRs384) {
  auto payload = Json::Factory::loadFromString(ds.kJwtPayload);
  DoTest(ds.kJwtRs384, ds.kPublicKey, "pem", true, Status::OK, payload);
}

TEST_F(JwtTestPem, OKWithAlgRs512) {
  auto payload = Json::Factory::loadFromString(ds.kJwtPayload);
  DoTest(ds.kJwtRs512, ds.kPublicKey, "pem", true, Status::OK, payload);
}

TEST_F(JwtTestPem, MultiAudiences) {
  Jwt jwt(ds.kJwtMultiSub);
  ASSERT_EQ(jwt.Aud(), std::vector<std::string>({"aud1", "aud2"}));
}

TEST_F(JwtTestPem, InvalidSignature) {
  auto invalid_jwt = ds.kJwt;
  invalid_jwt[ds.kJwt.length() - 2] =
      ds.kJwt[ds.kJwt.length() - 2] != 'a' ? 'a' : 'b';
  DoTest(invalid_jwt, ds.kPublicKey, "pem", false,
         Status::JWT_INVALID_SIGNATURE, nullptr);
}

TEST_F(JwtTestPem, InvalidPublicKey) {
  auto invalid_pubkey = ds.kPublicKey;
  invalid_pubkey[0] = ds.kPublicKey[0] != 'a' ? 'a' : 'b';
  DoTest(ds.kJwt, invalid_pubkey, "pem", false, Status::PEM_PUBKEY_PARSE_ERROR,
         nullptr);
}

TEST_F(JwtTestPem, PublicKeyInvalidBase64) {
  auto invalid_pubkey = "a";
  DoTest(ds.kJwt, invalid_pubkey, "pem", false, Status::PEM_PUBKEY_BAD_BASE64,
         nullptr);
}

TEST_F(JwtTestPem, Base64urlBadInputHeader) {
  auto invalid_header = ds.kJwtHeaderEncoded + "a";
  auto invalid_jwt = StringUtil::join(
      std::vector<std::string>{invalid_header, ds.kJwtPayloadEncoded,
                               ds.kJwtSignatureEncoded},
      ".");
  DoTest(invalid_jwt, ds.kPublicKey, "pem", false,
         Status::JWT_HEADER_PARSE_ERROR, nullptr);
}

TEST_F(JwtTestPem, Base64urlBadInputPayload) {
  auto invalid_payload = ds.kJwtPayloadEncoded + "a";
  auto invalid_jwt = StringUtil::join(
      std::vector<std::string>{ds.kJwtHeaderEncoded, invalid_payload,
                               ds.kJwtSignatureEncoded},
      ".");
  DoTest(invalid_jwt, ds.kPublicKey, "pem", false,
         Status::JWT_PAYLOAD_PARSE_ERROR, nullptr);
}

TEST_F(JwtTestPem, Base64urlBadinputSignature) {
  auto invalid_signature = "a";
  auto invalid_jwt = StringUtil::join(
      std::vector<std::string>{ds.kJwtHeaderEncoded, ds.kJwtPayloadEncoded,
                               invalid_signature},
      ".");
  DoTest(invalid_jwt, ds.kPublicKey, "pem", false,
         Status::JWT_SIGNATURE_PARSE_ERROR, nullptr);
}

TEST_F(JwtTestPem, JwtInvalidNumberOfDots) {
  auto invalid_jwt = ds.kJwt + '.';
  DoTest(invalid_jwt, ds.kPublicKey, "pem", false, Status::JWT_BAD_FORMAT,
         nullptr);
}

TEST_F(JwtTestPem, JsonBadInputHeader) {
  DoTest(ds.kJwtWithBadJsonHeader, ds.kPublicKey, "pem", false,
         Status::JWT_HEADER_PARSE_ERROR, nullptr);
}

TEST_F(JwtTestPem, JsonBadInputPayload) {
  DoTest(ds.kJwtWithBadJsonPayload, ds.kPublicKey, "pem", false,
         Status::JWT_PAYLOAD_PARSE_ERROR, nullptr);
}

TEST_F(JwtTestPem, AlgAbsentInHeader) {
  DoTest(ds.kJwtWithAlgAbsent, ds.kPublicKey, "pem", false,
         Status::JWT_HEADER_NO_ALG, nullptr);
}

TEST_F(JwtTestPem, AlgIsNotString) {
  DoTest(ds.kJwtWithAlgIsNotString, ds.kPublicKey, "pem", false,
         Status::JWT_HEADER_BAD_ALG, nullptr);
}

TEST_F(JwtTestPem, InvalidAlg) {
  DoTest(ds.kJwtWithInvalidAlg, ds.kPublicKey, "pem", false,
         Status::ALG_NOT_IMPLEMENTED, nullptr);
}

TEST_F(JwtTestPem, Es256Alg) {
  DoTest(ds.kJwtWithES256Alg, ds.kPublicKey, "pem", false,
         Status::JWT_INVALID_SIGNATURE, nullptr);
}

TEST(JwtSubExtractionTest, NonEmptyJwtSubShouldEqual) {
  DatasetPem ds;
  Jwt jwt(ds.kJwt);
  EXPECT_EQ(jwt.Sub(), ds.kJwtSub);
}

TEST(JwtSubExtractionTest, EmptyJwtSubShouldEqual) {
  Jwt jwt("");
  EXPECT_EQ(jwt.Sub(), "");
}

// Test cases w/ JWKs-formatted public key

class JwtTestJwks : public JwtTest {
 protected:
  DatasetJwk ds;
};

TEST_F(JwtTestJwks, OkNoKid) {
  auto payload = Json::Factory::loadFromString(ds.kJwtPayload);
  DoTest(ds.kJwtNoKid, ds.kPublicKeyRSA, "jwks", true, Status::OK, payload);
}

TEST_F(JwtTestJwks, OkTokenJwkRSAPublicKeyOptionalAlgKid) {
  auto payload = Json::Factory::loadFromString(ds.kJwtPayload);
  // Remove "alg" claim from public key.
  std::string alg_claim = "\"alg\": \"RS256\",";
  std::string pubkey_no_alg = ds.kPublicKeyRSA;
  std::size_t alg_pos = pubkey_no_alg.find(alg_claim);
  while (alg_pos != std::string::npos) {
    pubkey_no_alg.erase(alg_pos, alg_claim.length());
    alg_pos = pubkey_no_alg.find(alg_claim);
  }
  DoTest(ds.kJwtNoKid, pubkey_no_alg, "jwks", true, Status::OK, payload);

  // Remove "kid" claim from public key.
  std::string kid_claim1 =
      ",\"kid\": \"62a93512c9ee4c7f8067b5a216dade2763d32a47\"";
  std::string kid_claim2 =
      ",\"kid\": \"b3319a147514df7ee5e4bcdee51350cc890cc89e\"";
  std::string pubkey_no_kid = ds.kPublicKeyRSA;
  std::size_t kid_pos = pubkey_no_kid.find(kid_claim1);
  pubkey_no_kid.erase(kid_pos, kid_claim1.length());
  kid_pos = pubkey_no_kid.find(kid_claim2);
  pubkey_no_kid.erase(kid_pos, kid_claim2.length());
  DoTest(ds.kJwtNoKid, pubkey_no_kid, "jwks", true, Status::OK, payload);
}

TEST_F(JwtTestJwks, OkNoKidLogExp) {
  auto payload = Json::Factory::loadFromString(ds.kJwtPayloadLongExp);
  DoTest(ds.kJwtNoKidLongExp, ds.kPublicKeyRSA, "jwks", true, Status::OK,
         payload);
}

TEST_F(JwtTestJwks, OkCorrectKid) {
  auto payload = Json::Factory::loadFromString(ds.kJwtPayload);
  DoTest(ds.kJwtWithCorrectKid, ds.kPublicKeyRSA, "jwks", true, Status::OK,
         payload);
}

TEST_F(JwtTestJwks, IncorrectKid) {
  DoTest(ds.kJwtWithIncorrectKid, ds.kPublicKeyRSA, "jwks", false,
         Status::JWT_INVALID_SIGNATURE, nullptr);
}

TEST_F(JwtTestJwks, NonExistKid) {
  DoTest(ds.kJwtWithNonExistKid, ds.kPublicKeyRSA, "jwks", false,
         Status::KID_ALG_UNMATCH, nullptr);
}

TEST_F(JwtTestJwks, BadFormatKid) {
  DoTest(ds.kJwtWithBadFormatKid, ds.kPublicKeyRSA, "jwks", false,
         Status::JWT_HEADER_BAD_KID, nullptr);
}

TEST_F(JwtTestJwks, JwkBadJson) {
  std::string invalid_pubkey = "foobar";
  DoTest(ds.kJwtNoKid, invalid_pubkey, "jwks", false, Status::JWK_PARSE_ERROR,
         nullptr);
}

TEST_F(JwtTestJwks, JwkNoKeys) {
  std::string invalid_pubkey = R"EOF({"foo":"bar"})EOF";
  DoTest(ds.kJwtNoKid, invalid_pubkey, "jwks", false, Status::JWK_NO_KEYS,
         nullptr);
}

TEST_F(JwtTestJwks, JwkBadKeys) {
  std::string invalid_pubkey = R"EOF({"keys":"foobar"})EOF";
  DoTest(ds.kJwtNoKid, invalid_pubkey, "jwks", false, Status::JWK_BAD_KEYS,
         nullptr);
}

TEST_F(JwtTestJwks, JwkBadPublicKey) {
  std::string invalid_pubkey = R"EOF({"keys":[]})EOF";
  DoTest(ds.kJwtNoKid, invalid_pubkey, "jwks", false,
         Status::JWK_NO_VALID_PUBKEY, nullptr);
}

TEST_F(JwtTestJwks, OkTokenJwkEC) {
  auto payload = Json::Factory::loadFromString(ds.kJwtPayloadEC);
  // ES256-signed token with kid specified.
  DoTest(ds.kTokenEC, ds.kPublicKeyJwkEC, "jwks", true, Status::OK, payload);
  // ES256-signed token without kid specified.
  DoTest(ds.kTokenECNoKid, ds.kPublicKeyJwkEC, "jwks", true, Status::OK,
         payload);
}

TEST_F(JwtTestJwks, OkTokenJwkECPublicKeyOptionalAlgKid) {
  auto payload = Json::Factory::loadFromString(ds.kJwtPayloadEC);
  // Remove "alg" claim from public key.
  std::string alg_claim = "\"alg\": \"ES256\",";
  std::string pubkey_no_alg = ds.kPublicKeyJwkEC;
  std::size_t alg_pos = pubkey_no_alg.find(alg_claim);
  while (alg_pos != std::string::npos) {
    pubkey_no_alg.erase(alg_pos, alg_claim.length());
    alg_pos = pubkey_no_alg.find(alg_claim);
  }
  DoTest(ds.kTokenEC, pubkey_no_alg, "jwks", true, Status::OK, payload);

  // Remove "kid" claim from public key.
  std::string kid_claim1 = ",\"kid\": \"abc\"";
  std::string kid_claim2 = ",\"kid\": \"xyz\"";
  std::string pubkey_no_kid = ds.kPublicKeyJwkEC;
  std::size_t kid_pos = pubkey_no_kid.find(kid_claim1);
  pubkey_no_kid.erase(kid_pos, kid_claim1.length());
  kid_pos = pubkey_no_kid.find(kid_claim2);
  pubkey_no_kid.erase(kid_pos, kid_claim2.length());
  DoTest(ds.kTokenEC, pubkey_no_kid, "jwks", true, Status::OK, payload);
}

TEST_F(JwtTestJwks, NonExistKidEC) {
  DoTest(ds.kJwtWithNonExistKidEC, ds.kPublicKeyJwkEC, "jwks", false,
         Status::KID_ALG_UNMATCH, nullptr);
}

TEST_F(JwtTestJwks, InvalidPublicKeyEC) {
  auto invalid_pubkey = ds.kPublicKeyJwkEC;
  invalid_pubkey.replace(12, 9, "kty\":\"RSA");
  DoTest(ds.kTokenEC, invalid_pubkey, "jwks", false, Status::KID_ALG_UNMATCH,
         nullptr);
}

TEST_F(JwtTestJwks, DebugSegFault) {
  DoTest(ds.kJwtNoKid, ds.kBadPublicKeyRSA, "jwks", false,
         Status::JWK_RSA_PUBKEY_PARSE_ERROR, nullptr);
}

}  // namespace JwtAuth
}  // namespace Http
}  // namespace Envoy
