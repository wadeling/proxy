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

#include "src/istio/control/http/attributes_builder.h"

#include "gmock/gmock.h"
#include "google/protobuf/stubs/status.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/util/message_differencer.h"
#include "gtest/gtest.h"
#include "include/istio/utils/attribute_names.h"
#include "include/istio/utils/attributes_builder.h"
#include "src/istio/control/http/mock_check_data.h"
#include "src/istio/control/http/mock_report_data.h"

using ::google::protobuf::TextFormat;
using ::google::protobuf::util::MessageDifferencer;
using ::istio::mixer::v1::Attributes;
using ::istio::mixer::v1::Attributes_StringMap;

using ::testing::_;
using ::testing::Invoke;
using ::testing::ReturnRef;

namespace istio {
namespace control {
namespace http {
namespace {

MATCHER_P(EqualsAttribute, expected, "") {
  const auto matched = MessageDifferencer::Equals(arg, expected);
  if (!matched) {
    GOOGLE_LOG(INFO) << arg.DebugString() << " vs " << expected.DebugString();
  }
  return matched;
}
const char kCheckAttributesWithoutAuthnFilter[] = R"(
attributes {
  key: "connection.mtls"
  value {
    bool_value: true
  }
}
attributes {
  key: "connection.requested_server_name"
  value {
    string_value: "www.google.com"
  }
}
attributes {
  key: "context.protocol"
  value {
    string_value: "http"
  }
}
attributes {
  key: "destination.principal"
  value {
    string_value: "destination_user"
  }
}
attributes {
  key: "origin.ip"
  value {
    bytes_value: "1.2.3.4"
  }
}
attributes {
  key: "request.headers"
  value {
    string_map_value {
      entries {
        key: "host"
        value: "localhost"
      }
      entries {
        key: "path"
        value: "/books?a=b&c=d"
      }
    }
  }
}
attributes {
  key: "request.host"
  value {
    string_value: "localhost"
  }
}
attributes {
  key: "request.path"
  value {
    string_value: "/books?a=b&c=d"
  }
}
attributes {
  key: "request.query_params"
  value {
    string_map_value {
      entries {
        key: "a"
        value: "b"
      }
      entries {
        key: "c"
        value: "d"
      }
    }
  }
}
attributes {
  key: "request.scheme"
  value {
    string_value: "http"
  }
}
attributes {
  key: "request.time"
  value {
    timestamp_value {
    }
  }
}
attributes {
  key: "request.url_path"
  value {
    string_value: "/books"
  }
}
attributes {
  key: "source.principal"
  value {
    string_value: "sa/test_user/ns/ns_ns/"
  }
}
)";

const char kCheckAttributes[] = R"(
attributes {
  key: "context.protocol"
  value {
    string_value: "http"
  }
}
attributes {
  key: "request.headers"
  value {
    string_map_value {
      entries {
        key: "host"
        value: "localhost"
      }
      entries {
        key: "path"
        value: "/books?a=b&c=d"
      }
    }
  }
}
attributes {
  key: "request.host"
  value {
    string_value: "localhost"
  }
}
attributes {
  key: "request.path"
  value {
    string_value: "/books?a=b&c=d"
  }
}
attributes {
  key: "request.url_path"
  value {
    string_value: "/books"
  }
}
attributes {
  key: "request.query_params"
  value {
    string_map_value {
      entries {
        key: "a"
        value: "b"
      }
      entries {
        key: "c"
        value: "d"
      }
    }
  }
}
attributes {
  key: "request.scheme"
  value {
    string_value: "http"
  }
}
attributes {
  key: "request.time"
  value {
    timestamp_value {
    }
  }
}
attributes {
  key: "connection.mtls"
  value {
    bool_value: true
  }
}
attributes {
  key: "connection.requested_server_name"
  value {
    string_value: "www.google.com"
  }
}
attributes {
  key: "source.namespace"
  value {
    string_value: "ns_ns"
  }
}
attributes {
  key: "source.principal"
  value {
    string_value: "sa/test_user/ns/ns_ns/"
  }
}
attributes {
  key: "source.user"
  value {
    string_value: "sa/test_user/ns/ns_ns/"
  }
}
attributes {
  key: "origin.ip"
  value {
    bytes_value: "1.2.3.4"
  }
}
attributes {
  key: "destination.principal"
  value {
    string_value: "destination_user"
  }
}
attributes {
  key: "request.auth.audiences"
  value {
    string_value: "thisisaud"
  }
}
attributes {
  key: "request.auth.claims"
  value {
    string_map_value {
      entries {
        key: "aud"
        value: "thisisaud"
      }
      entries {
        key: "azp"
        value: "thisisazp"
      }
      entries {
        key: "email"
        value: "thisisemail@email.com"
      }
      entries {
        key: "exp"
        value: "5112754205"
      }
      entries {
        key: "iat"
        value: "1512754205"
      }
      entries {
        key: "iss"
        value: "thisisiss"
      }
      entries {
        key: "sub"
        value: "thisissub"
      }
    }
  }
}
attributes {
  key: "request.auth.presenter"
  value {
    string_value: "thisisazp"
  }
}
attributes {
  key: "request.auth.principal"
  value {
    string_value: "thisisiss/thisissub"
  }
}
attributes {
  key: "request.auth.raw_claims"
  value {
    string_value: "test_raw_claims"
  }
}
)";

const char kReportAttributes[] = R"(
attributes {
  key: "request.size"
  value {
    int64_value: 100
  }
}
attributes {
  key: "response.code"
  value {
    int64_value: 404
  }
}
attributes {
  key: "response.duration"
  value {
    duration_value {
      nanos: 1
    }
  }
}
attributes {
  key: "destination.ip"
  value {
    bytes_value: "1.2.3.4"
  }
}
attributes {
  key: "destination.port"
  value {
    int64_value: 8080
  }
}
attributes {
  key: "request.headers"
  value {
    string_map_value {
      entries {
        key: "x-b3-traceid"
        value: "abc"
      }
      entries {
        key: "x-b3-spanid"
        value: "def"
      }
    }
  }
}
attributes {
  key: "response.headers"
  value {
    string_map_value {
      entries {
        key: "content-length"
        value: "123456"
      }
      entries {
        key: "server"
        value: "my-server"
      }
    }
  }
}
attributes {
  key: "response.size"
  value {
    int64_value: 200
  }
}
attributes {
  key: "response.total_size"
  value {
    int64_value: 120
  }
}
attributes {
  key: "request.total_size"
  value {
    int64_value: 240
  }
}
attributes {
  key: "response.time"
  value {
    timestamp_value {
    }
  }
}
attributes {
  key: "context.proxy_error_code"
  value {
    string_value: "NR"
  }
}
attributes {
  key: "rbac.permissive.response_code"
  value {
    string_value: "403"
  }
}
attributes {
  key: "rbac.permissive.effective_policy_id"
  value {
    string_value: "policy-foo"
  }
}
attributes {
  key: "foo.bar.com"
  value {
    string_map_value {
      entries {
        key: "str"
        value: "abc"
      }
      entries {
        key: "list"
        value: "a,b,c"
      }
    }
  }
}
)";

constexpr char kAuthenticationResultStruct[] = R"(
fields {
  key: "request.auth.audiences"
  value {
    string_value: "thisisaud"
  }
}
fields {
  key: "request.auth.claims"
  value {
    struct_value {
      fields {
        key: "iss"
        value {
          string_value: "thisisiss"
        }
      }
      fields {
        key: "sub"
        value {
          string_value: "thisissub"
        }
      }
      fields {
        key: "aud"
        value {
          string_value: "thisisaud"
        }
      }
      fields {
        key: "azp"
        value {
          string_value: "thisisazp"
        }
      }
      fields {
        key: "email"
        value {
          string_value: "thisisemail@email.com"
        }
      }
      fields {
        key: "iat"
        value {
          string_value: "1512754205"
        }
      }
      fields {
        key: "exp"
        value {
          string_value: "5112754205"
        }
      }
    }
  }
}
fields {
  key: "request.auth.groups"
  value {
    list_value {
      values {
        string_value: "group1"
      }
      values {
        string_value: "group2"
      }
    }
  }
}
fields {
  key: "request.auth.presenter"
  value {
    string_value: "thisisazp"
  }
}
fields {
  key: "request.auth.principal"
  value {
    string_value: "thisisiss/thisissub"
  }
}
fields {
  key: "request.auth.raw_claims"
  value {
    string_value: "test_raw_claims"
  }
}
fields {
  key: "source.namespace"
  value {
    string_value: "ns_ns"
  }
}
fields {
  key: "source.principal"
  value {
    string_value: "sa/test_user/ns/ns_ns/"
  }
}
fields {
  key: "source.user"
  value {
    string_value: "sa/test_user/ns/ns_ns/"
  }
}
)";

void ClearContextTime(const std::string &name,
                      istio::mixer::v1::Attributes *attributes) {
  // Override timestamp with -
  utils::AttributesBuilder builder(attributes);
  std::chrono::time_point<std::chrono::system_clock> time0;
  builder.AddTimestamp(name, time0);
}

void SetDestinationIp(istio::mixer::v1::Attributes *attributes,
                      const std::string &ip) {
  utils::AttributesBuilder builder(attributes);
  builder.AddBytes(utils::AttributeName::kDestinationIp, ip);
}

TEST(AttributesBuilderTest, TestExtractForwardedAttributes) {
  Attributes attr;
  (*attr.mutable_attributes())["source.uid"].set_string_value("test_value");

  ::testing::StrictMock<MockCheckData> mock_data;
  EXPECT_CALL(mock_data, ExtractIstioAttributes(_))
      .WillOnce(Invoke([&attr](std::string *data) -> bool {
        attr.SerializeToString(data);
        return true;
      }));

  istio::mixer::v1::Attributes attributes;
  AttributesBuilder builder(&attributes);
  builder.ExtractForwardedAttributes(&mock_data);
  EXPECT_THAT(attributes, EqualsAttribute(attr));
}

TEST(AttributesBuilderTest, TestForwardAttributes) {
  Attributes forwarded_attr;
  ::testing::StrictMock<MockHeaderUpdate> mock_header;
  EXPECT_CALL(mock_header, AddIstioAttributes(_))
      .WillOnce(Invoke([&forwarded_attr](const std::string &data) {
        EXPECT_TRUE(forwarded_attr.ParseFromString(data));
      }));

  Attributes origin_attr;
  (*origin_attr.mutable_attributes())["test_key"].set_string_value(
      "test_value");

  AttributesBuilder::ForwardAttributes(origin_attr, &mock_header);
  EXPECT_THAT(forwarded_attr, EqualsAttribute(origin_attr));
}

TEST(AttributesBuilderTest, TestCheckAttributesWithoutAuthnFilter) {
  // In production, it is expected that authn filter always available whenver
  // mTLS or JWT is in used. This test case merely for completness to illustrate
  // what attributes are populated if authn filter is missing.
  ::testing::StrictMock<MockCheckData> mock_data;
  EXPECT_CALL(mock_data, GetPrincipal(_, _))
      .WillRepeatedly(Invoke([](bool peer, std::string *user) -> bool {
        if (peer) {
          *user = "sa/test_user/ns/ns_ns/";
        } else {
          *user = "destination_user";
        }
        return true;
      }));
  EXPECT_CALL(mock_data, IsMutualTLS()).WillOnce(Invoke([]() -> bool {
    return true;
  }));
  EXPECT_CALL(mock_data, GetRequestedServerName(_))
      .WillOnce(Invoke([](std::string *name) -> bool {
        *name = "www.google.com";
        return true;
      }));
  EXPECT_CALL(mock_data, GetSourceIpPort(_, _))
      .WillOnce(Invoke([](std::string *ip, int *port) -> bool {
        *ip = "1.2.3.4";
        *port = 8080;
        return true;
      }));
  EXPECT_CALL(mock_data, GetRequestHeaders())
      .WillOnce(Invoke([]() -> std::map<std::string, std::string> {
        std::map<std::string, std::string> map;
        map["path"] = "/books?a=b&c=d";
        map["host"] = "localhost";
        return map;
      }));
  EXPECT_CALL(mock_data, FindHeaderByType(_, _))
      .WillRepeatedly(Invoke(
          [](CheckData::HeaderType header_type, std::string *value) -> bool {
            if (header_type == CheckData::HEADER_PATH) {
              *value = "/books?a=b&c=d";
              return true;
            } else if (header_type == CheckData::HEADER_HOST) {
              *value = "localhost";
              return true;
            }
            return false;
          }));
  EXPECT_CALL(mock_data, GetAuthenticationResult())
      .WillOnce(testing::Return(nullptr));

  EXPECT_CALL(mock_data, GetUrlPath(_))
      .WillOnce(Invoke([](std::string *path) -> bool {
        *path = "/books";
        return true;
      }));
  EXPECT_CALL(mock_data, GetRequestQueryParams(_))
      .WillOnce(Invoke([](std::map<std::string, std::string> *map) -> bool {
        (*map)["a"] = "b";
        (*map)["c"] = "d";
        return true;
      }));

  istio::mixer::v1::Attributes attributes;
  AttributesBuilder builder(&attributes);
  builder.ExtractCheckAttributes(&mock_data);

  ClearContextTime(utils::AttributeName::kRequestTime, &attributes);

  Attributes expected_attributes;
  ASSERT_TRUE(TextFormat::ParseFromString(kCheckAttributesWithoutAuthnFilter,
                                          &expected_attributes));
  EXPECT_THAT(attributes, EqualsAttribute(expected_attributes));
}

TEST(AttributesBuilderTest, TestCheckAttributes) {
  ::testing::StrictMock<MockCheckData> mock_data;
  EXPECT_CALL(mock_data, IsMutualTLS()).WillOnce(Invoke([]() -> bool {
    return true;
  }));
  EXPECT_CALL(mock_data, GetPrincipal(_, _))
      .WillRepeatedly(Invoke([](bool peer, std::string *user) -> bool {
        if (peer) {
          *user = "sa/test_user/ns/ns_ns/";
        } else {
          *user = "destination_user";
        }
        return true;
      }));
  EXPECT_CALL(mock_data, GetRequestedServerName(_))
      .WillOnce(Invoke([](std::string *name) -> bool {
        *name = "www.google.com";
        return true;
      }));
  EXPECT_CALL(mock_data, GetSourceIpPort(_, _))
      .WillOnce(Invoke([](std::string *ip, int *port) -> bool {
        *ip = "1.2.3.4";
        *port = 8080;
        return true;
      }));
  EXPECT_CALL(mock_data, GetRequestHeaders())
      .WillOnce(Invoke([]() -> std::map<std::string, std::string> {
        std::map<std::string, std::string> map;
        map["path"] = "/books?a=b&c=d";
        map["host"] = "localhost";
        return map;
      }));
  EXPECT_CALL(mock_data, FindHeaderByType(_, _))
      .WillRepeatedly(Invoke(
          [](CheckData::HeaderType header_type, std::string *value) -> bool {
            if (header_type == CheckData::HEADER_PATH) {
              *value = "/books?a=b&c=d";
              return true;
            } else if (header_type == CheckData::HEADER_HOST) {
              *value = "localhost";
              return true;
            }
            return false;
          }));
  google::protobuf::Struct authn_result;
  ASSERT_TRUE(
      TextFormat::ParseFromString(kAuthenticationResultStruct, &authn_result));

  EXPECT_CALL(mock_data, GetAuthenticationResult())
      .WillOnce(testing::Return(&authn_result));
  EXPECT_CALL(mock_data, GetUrlPath(_))
      .WillOnce(Invoke([](std::string *path) -> bool {
        *path = "/books";
        return true;
      }));
  EXPECT_CALL(mock_data, GetRequestQueryParams(_))
      .WillOnce(Invoke([](std::map<std::string, std::string> *map) -> bool {
        (*map)["a"] = "b";
        (*map)["c"] = "d";
        return true;
      }));

  istio::mixer::v1::Attributes attributes;
  AttributesBuilder builder(&attributes);
  builder.ExtractCheckAttributes(&mock_data);

  ClearContextTime(utils::AttributeName::kRequestTime, &attributes);

  Attributes expected_attributes;
  ASSERT_TRUE(
      TextFormat::ParseFromString(kCheckAttributes, &expected_attributes));
  EXPECT_THAT(attributes, EqualsAttribute(expected_attributes));
}

TEST(AttributesBuilderTest, TestReportAttributes) {
  ::testing::StrictMock<MockReportData> mock_data;

  ::google::protobuf::Map<std::string, ::google::protobuf::Struct>
      filter_metadata;
  ::google::protobuf::Struct struct_obj;
  ::google::protobuf::Value strval, numval, boolval, listval;
  strval.set_string_value("abc");
  (*struct_obj.mutable_fields())["str"] = strval;
  numval.set_number_value(12.3);
  (*struct_obj.mutable_fields())["num"] = numval;
  boolval.set_bool_value(true);
  (*struct_obj.mutable_fields())["bool"] = boolval;
  listval.mutable_list_value()->add_values()->set_string_value("a");
  listval.mutable_list_value()->add_values()->set_string_value("b");
  listval.mutable_list_value()->add_values()->set_string_value("c");
  (*struct_obj.mutable_fields())["list"] = listval;
  filter_metadata["foo.bar.com"] = struct_obj;
  filter_metadata["istio.mixer"] = struct_obj;  // to be ignored

  EXPECT_CALL(mock_data, GetDestinationIpPort(_, _))
      .WillOnce(Invoke([](std::string *ip, int *port) -> bool {
        *ip = "1.2.3.4";
        *port = 8080;
        return true;
      }));
  EXPECT_CALL(mock_data, GetDestinationUID(_))
      .WillOnce(Invoke([](std::string *uid) -> bool {
        *uid = "pod1.ns2";
        return true;
      }));
  EXPECT_CALL(mock_data, GetResponseHeaders())
      .WillOnce(Invoke([]() -> std::map<std::string, std::string> {
        std::map<std::string, std::string> map;
        map["content-length"] = "123456";
        map["server"] = "my-server";
        return map;
      }));
  EXPECT_CALL(mock_data, GetTracingHeaders(_))
      .WillOnce(Invoke([](std::map<std::string, std::string> &m) {
        m["x-b3-traceid"] = "abc";
        m["x-b3-spanid"] = "def";
      }));
  EXPECT_CALL(mock_data, GetReportInfo(_))
      .WillOnce(Invoke([](ReportData::ReportInfo *info) {
        info->request_body_size = 100;
        info->response_body_size = 200;
        info->response_total_size = 120;
        info->request_total_size = 240;
        info->duration = std::chrono::nanoseconds(1);
        info->response_code = 404;
        info->response_flags = "NR";
      }));
  EXPECT_CALL(mock_data, GetGrpcStatus(_))
      .WillOnce(Invoke([](ReportData::GrpcStatus *status) -> bool {
        status->status = "grpc-status";
        status->message = "grpc-message";
        return true;
      }));
  EXPECT_CALL(mock_data, GetRbacReportInfo(_))
      .WillOnce(Invoke([](ReportData::RbacReportInfo *report_info) -> bool {
        report_info->permissive_resp_code = "403";
        report_info->permissive_policy_id = "policy-foo";
        return true;
      }));
  EXPECT_CALL(mock_data, GetDynamicFilterState())
      .WillOnce(ReturnRef(filter_metadata));

  istio::mixer::v1::Attributes attributes;
  AttributesBuilder builder(&attributes);
  builder.ExtractReportAttributes(::google::protobuf::util::Status::OK,
                                  &mock_data);

  ClearContextTime(utils::AttributeName::kResponseTime, &attributes);

  Attributes expected_attributes;
  ASSERT_TRUE(
      TextFormat::ParseFromString(kReportAttributes, &expected_attributes));
  (*expected_attributes
        .mutable_attributes())[utils::AttributeName::kDestinationUID]
      .set_string_value("pod1.ns2");
  (*expected_attributes
        .mutable_attributes())[utils::AttributeName::kResponseGrpcStatus]
      .set_string_value("grpc-status");
  (*expected_attributes
        .mutable_attributes())[utils::AttributeName::kResponseGrpcMessage]
      .set_string_value("grpc-message");
  EXPECT_THAT(attributes, EqualsAttribute(expected_attributes));
}

TEST(AttributesBuilderTest, TestReportAttributesWithDestIP) {
  ::testing::StrictMock<MockReportData> mock_data;

  ::google::protobuf::Map<std::string, ::google::protobuf::Struct>
      filter_metadata;
  ::google::protobuf::Struct struct_obj;
  ::google::protobuf::Value strval, numval, boolval, listval;
  strval.set_string_value("abc");
  (*struct_obj.mutable_fields())["str"] = strval;
  numval.set_number_value(12.3);
  (*struct_obj.mutable_fields())["num"] = numval;
  boolval.set_bool_value(true);
  (*struct_obj.mutable_fields())["bool"] = boolval;
  listval.mutable_list_value()->add_values()->set_string_value("a");
  listval.mutable_list_value()->add_values()->set_string_value("b");
  listval.mutable_list_value()->add_values()->set_string_value("c");
  (*struct_obj.mutable_fields())["list"] = listval;
  filter_metadata["foo.bar.com"] = struct_obj;

  EXPECT_CALL(mock_data, GetDestinationIpPort(_, _))
      .WillOnce(Invoke([](std::string *ip, int *port) -> bool {
        *ip = "2.3.4.5";
        *port = 8080;
        return true;
      }));
  EXPECT_CALL(mock_data, GetDestinationUID(_)).WillOnce(testing::Return(false));
  EXPECT_CALL(mock_data, GetResponseHeaders())
      .WillOnce(Invoke([]() -> std::map<std::string, std::string> {
        std::map<std::string, std::string> map;
        map["content-length"] = "123456";
        map["server"] = "my-server";
        return map;
      }));
  EXPECT_CALL(mock_data, GetTracingHeaders(_))
      .WillOnce(Invoke([](std::map<std::string, std::string> &m) {
        m["x-b3-traceid"] = "abc";
        m["x-b3-spanid"] = "def";
      }));
  EXPECT_CALL(mock_data, GetReportInfo(_))
      .WillOnce(Invoke([](ReportData::ReportInfo *info) {
        info->request_body_size = 100;
        info->response_body_size = 200;
        info->response_total_size = 120;
        info->request_total_size = 240;
        info->duration = std::chrono::nanoseconds(1);
        info->response_code = 404;
        info->response_flags = "NR";
      }));
  EXPECT_CALL(mock_data, GetGrpcStatus(_)).WillOnce(testing::Return(false));
  EXPECT_CALL(mock_data, GetRbacReportInfo(_))
      .WillOnce(Invoke([](ReportData::RbacReportInfo *report_info) -> bool {
        report_info->permissive_resp_code = "403";
        report_info->permissive_policy_id = "policy-foo";
        return true;
      }));
  EXPECT_CALL(mock_data, GetDynamicFilterState())
      .WillOnce(ReturnRef(filter_metadata));

  istio::mixer::v1::Attributes attributes;
  SetDestinationIp(&attributes, "1.2.3.4");
  AttributesBuilder builder(&attributes);
  builder.ExtractReportAttributes(::google::protobuf::util::Status::OK,
                                  &mock_data);

  ClearContextTime(utils::AttributeName::kResponseTime, &attributes);

  Attributes expected_attributes;
  ASSERT_TRUE(
      TextFormat::ParseFromString(kReportAttributes, &expected_attributes));
  EXPECT_THAT(attributes, EqualsAttribute(expected_attributes));
}

}  // namespace
}  // namespace http
}  // namespace control
}  // namespace istio
