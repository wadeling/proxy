#include "envoy/registry/registry.h"
#include "envoy/server/filter_config.h"

#include "src/envoy/tcp/echo2/echo2.h"

namespace Envoy {
namespace Tcp{
namespace Echo2 {

/**
 * Config registration for the echo filter. @see NamedNetworkFilterConfigFactory.
 */
class Echo2ConfigFactory : public Server::Configuration::NamedNetworkFilterConfigFactory {
public:
  // NamedNetworkFilterConfigFactory
  Network::FilterFactoryCb createFilterFactory(const Json::Object&,
                                               Server::Configuration::FactoryContext&) override {
//    ENVOY_LOG(trace, "echo2 create filter factory");

    return [](Network::FilterManager& filter_manager) -> void {
      filter_manager.addReadFilter(std::make_shared<Echo2Filter>());
    };
  }

  Network::FilterFactoryCb
  createFilterFactoryFromProto(const Protobuf::Message&,
                               Server::Configuration::FactoryContext&) override {
//    ENVOY_LOG(trace, "echo2 create filter factory from proto");

    return [](Network::FilterManager& filter_manager) -> void {
      filter_manager.addReadFilter(std::make_shared<Echo2Filter>());
    };
  }

  ProtobufTypes::MessagePtr createEmptyConfigProto() override {
    return ProtobufTypes::MessagePtr{new Envoy::ProtobufWkt::Empty()};
  }

  std::string name() override { return "echo2"; }
};

/**
 * Static registration for the echo filter. @see RegisterFactory.
 */
REGISTER_FACTORY(Echo2ConfigFactory, Server::Configuration::NamedNetworkFilterConfigFactory);

} // namespace Echo2
} // namespace Tcp
} // namespace Envoy
