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

#include "src/envoy/tcp/forward_downstream_sni/config.h"

#include "envoy/registry/registry.h"
#include "envoy/server/filter_config.h"
#include "src/envoy/tcp/forward_downstream_sni/forward_downstream_sni.h"

namespace Envoy {
namespace Tcp {
namespace ForwardDownstreamSni {

Network::FilterFactoryCb
ForwardDownstreamSniNetworkFilterConfigFactory::createFilterFactory(
    const Json::Object&, Server::Configuration::FactoryContext&) {
  // Only used in v1 filters.
  NOT_IMPLEMENTED_GCOVR_EXCL_LINE;
}

Network::FilterFactoryCb
ForwardDownstreamSniNetworkFilterConfigFactory::createFilterFactoryFromProto(
    const Protobuf::Message&, Server::Configuration::FactoryContext&) {
  return [](Network::FilterManager& filter_manager) -> void {
    filter_manager.addReadFilter(
        std::make_shared<ForwardDownstreamSniFilter>());
  };
}

ProtobufTypes::MessagePtr
ForwardDownstreamSniNetworkFilterConfigFactory::createEmptyConfigProto() {
  return std::make_unique<ProtobufWkt::Empty>();
}

/**
 * Static registration for the forward_original_sni filter. @see
 * RegisterFactory.
 */
static Registry::RegisterFactory<
    ForwardDownstreamSniNetworkFilterConfigFactory,
    Server::Configuration::NamedNetworkFilterConfigFactory>
    registered_;

}  // namespace ForwardDownstreamSni
}  // namespace Tcp
}  // namespace Envoy
