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

#ifndef ISTIO_UTILS_PROTOBUF_H_
#define ISTIO_UTILS_PROTOBUF_H_

#include <chrono>

#include "google/protobuf/duration.pb.h"
#include "google/protobuf/stubs/status.h"
#include "google/protobuf/timestamp.pb.h"

namespace istio {
namespace utils {

// Convert system_clock time to protobuf timestamp
::google::protobuf::Timestamp CreateTimestamp(
    std::chrono::system_clock::time_point tp);

// Convert from chrono duration to protobuf duration.
::google::protobuf::Duration CreateDuration(std::chrono::nanoseconds value);

// Convert from prtoobuf duration to chrono duration.
std::chrono::milliseconds ToMilliseonds(
    const ::google::protobuf::Duration& duration);

bool InvalidDictionaryStatus(const ::google::protobuf::util::Status& status);

}  // namespace utils
}  // namespace istio

#endif  // ISTIO_UTILS_PROTOBUF_H_
