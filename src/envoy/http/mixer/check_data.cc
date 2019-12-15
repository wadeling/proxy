/* Copyright 2018 Istio Authors. All Rights Reserved.
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

#include "src/envoy/http/mixer/check_data.h"

#include "absl/strings/string_view.h"
#include "common/common/base64.h"
#include "src/envoy/http/jwt_auth/jwt.h"
#include "src/envoy/http/jwt_auth/jwt_authenticator.h"
#include "src/envoy/utils/authn.h"
#include "src/envoy/utils/header_update.h"
#include "src/envoy/utils/utils.h"

using HttpCheckData = ::istio::control::http::CheckData;

namespace Envoy {
namespace Http {
namespace Mixer {
namespace {
// Referer header
const LowerCaseString kRefererHeaderKey("referer");

// Set of headers excluded from request.headers attribute.
const std::set<std::string> RequestHeaderExclusives = {
    Utils::HeaderUpdate::IstioAttributeHeader().get(),
};

}  // namespace

CheckData::CheckData(const HeaderMap& headers,
                     const envoy::api::v2::core::Metadata& metadata,
                     const Network::Connection* connection)
    : headers_(headers), metadata_(metadata), connection_(connection) {
  if (headers_.Path()) {
    query_params_ =
        Utility::parseQueryString(headers_.Path()->value().getStringView());
  }
}

bool CheckData::ExtractIstioAttributes(std::string* data) const {
  // Extract attributes from x-istio-attributes header
  const HeaderEntry* entry =
      headers_.get(Utils::HeaderUpdate::IstioAttributeHeader());
  if (entry) {
    *data = Base64::decode(std::string(entry->value().getStringView()));
    return true;
  }
  return false;
}

bool CheckData::GetSourceIpPort(std::string* ip, int* port) const {
  if (connection_) {
    return Utils::GetIpPort(connection_->remoteAddress()->ip(), ip, port);
  }
  return false;
}

bool CheckData::GetPrincipal(bool peer, std::string* user) const {
  return Utils::GetPrincipal(connection_, peer, user);
}

std::map<std::string, std::string> CheckData::GetRequestHeaders() const {
  std::map<std::string, std::string> header_map;
  Utils::ExtractHeaders(headers_, RequestHeaderExclusives, header_map);
  return header_map;
}

bool CheckData::IsMutualTLS() const { return Utils::IsMutualTLS(connection_); }

bool CheckData::GetRequestedServerName(std::string* name) const {
  return Utils::GetRequestedServerName(connection_, name);
}

bool CheckData::FindHeaderByType(HttpCheckData::HeaderType header_type,
                                 std::string* value) const {
  switch (header_type) {
    case HttpCheckData::HEADER_PATH:
      if (headers_.Path()) {
        *value = std::string(headers_.Path()->value().getStringView());
        return true;
      }
      break;
    case HttpCheckData::HEADER_HOST:
      if (headers_.Host()) {
        *value = std::string(headers_.Host()->value().getStringView());
        return true;
      }
      break;
    case HttpCheckData::HEADER_SCHEME:
      if (headers_.Scheme()) {
        *value = std::string(headers_.Scheme()->value().getStringView());
        return true;
      }
      break;
    case HttpCheckData::HEADER_USER_AGENT:
      if (headers_.UserAgent()) {
        *value = std::string(headers_.UserAgent()->value().getStringView());
        return true;
      }
      break;
    case HttpCheckData::HEADER_METHOD:
      if (headers_.Method()) {
        *value = std::string(headers_.Method()->value().getStringView());
        return true;
      }
      break;
    case HttpCheckData::HEADER_CONTENT_TYPE:
      if (headers_.ContentType()) {
        *value = std::string(headers_.ContentType()->value().getStringView());
        return true;
      }
      break;
    case HttpCheckData::HEADER_REFERER: {
      const HeaderEntry* referer = headers_.get(kRefererHeaderKey);
      if (referer) {
        *value = std::string(referer->value().getStringView());
        return true;
      }
    } break;
  }
  return false;
}

bool CheckData::FindHeaderByName(const std::string& name,
                                 std::string* value) const {
  const HeaderEntry* entry = headers_.get(LowerCaseString(name));
  if (entry) {
    *value = std::string(entry->value().getStringView());
    return true;
  }
  return false;
}

bool CheckData::FindQueryParameter(const std::string& name,
                                   std::string* value) const {
  const auto& it = query_params_.find(name);
  if (it != query_params_.end()) {
    *value = it->second;
    return true;
  }
  return false;
}

bool CheckData::FindCookie(const std::string& name, std::string* value) const {
  std::string cookie = Utility::parseCookieValue(headers_, name);
  if (cookie != "") {
    *value = cookie;
    return true;
  }
  return false;
}

const ::google::protobuf::Struct* CheckData::GetAuthenticationResult() const {
  return Utils::Authentication::GetResultFromMetadata(metadata_);
}

bool CheckData::GetUrlPath(std::string* url_path) const {
  if (!headers_.Path()) {
    return false;
  }
  const HeaderString& path = headers_.Path()->value();
  const absl::string_view path_view = path.getStringView();
  absl::string_view query_start = Utility::findQueryStringStart(path);
  if (query_start.length() > 0) {
    const size_t path_string_length = path.size() - query_start.length();
    *url_path =
        std::string(path_view.begin(), path_view.begin() + path_string_length);
  } else {
    *url_path = std::string(path_view);
  }
  return true;
}

bool CheckData::GetRequestQueryParams(
    std::map<std::string, std::string>* query_params) const {
  if (!headers_.Path()) {
    return false;
  }
  *query_params =
      Utility::parseQueryString(headers_.Path()->value().getStringView());
  return true;
}

}  // namespace Mixer
}  // namespace Http
}  // namespace Envoy
