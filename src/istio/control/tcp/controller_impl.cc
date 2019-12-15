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

#include "src/istio/control/tcp/controller_impl.h"

#include "src/istio/control/tcp/request_handler_impl.h"

using ::istio::mixer::v1::config::client::TcpClientConfig;
using ::istio::mixerclient::Statistics;

namespace istio {
namespace control {
namespace tcp {

ControllerImpl::ControllerImpl(const Options& data) {
  client_context_.reset(new ClientContext(data));
}

std::unique_ptr<RequestHandler> ControllerImpl::CreateRequestHandler() {
  return std::unique_ptr<RequestHandler>(
      new RequestHandlerImpl(client_context_));
}

std::unique_ptr<Controller> Controller::Create(const Options& data) {
  return std::unique_ptr<Controller>(new ControllerImpl(data));
}

void ControllerImpl::GetStatistics(Statistics* stat) const {
  client_context_->GetStatistics(stat);
}

}  // namespace tcp
}  // namespace control
}  // namespace istio
