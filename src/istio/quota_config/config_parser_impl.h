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

#ifndef ISTIO_QUOTA_CONFIG_CONFIG_PARSER_IMPL_H_
#define ISTIO_QUOTA_CONFIG_CONFIG_PARSER_IMPL_H_

#include <regex>
#include <unordered_map>

#include "include/istio/quota_config/config_parser.h"

namespace istio {
namespace quota_config {

// An object to implement ConfigParser interface.
class ConfigParserImpl : public ConfigParser {
 public:
  ConfigParserImpl(
      const ::istio::mixer::v1::config::client::QuotaSpec& spec_pb);

  // Get quota requirements for a attribute set.
  void GetRequirements(const ::istio::mixer::v1::Attributes& attributes,
                       std::vector<Requirement>* results) const override;

 private:
  // Check one attribute match.
  bool MatchAttributes(
      const ::istio::mixer::v1::config::client::AttributeMatch& match,
      const ::istio::mixer::v1::Attributes& attributes) const;
  // the spec proto.
  const ::istio::mixer::v1::config::client::QuotaSpec& spec_pb_;

  // Stored regex objects.
  std::unordered_map<std::string, std::regex> regex_map_;
};

}  // namespace quota_config
}  // namespace istio

#endif  // ISTIO_QUOTA_CONFIG_CONFIG_PARSER_IMPL_H_
