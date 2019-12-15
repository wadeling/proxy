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

#include "src/istio/mixerclient/check_cache.h"

#include "include/istio/utils/protobuf.h"
#include "src/istio/utils/logger.h"

using namespace std::chrono;
using ::google::protobuf::util::Status;
using ::google::protobuf::util::error::Code;
using ::istio::mixer::v1::Attributes;
using ::istio::mixer::v1::CheckResponse;

namespace istio {
namespace mixerclient {

void CheckCache::CacheElem::CacheElem::SetResponse(
    const CheckResponse &response, Tick time_now) {
  if (response.has_precondition()) {
    status_ = parent_.ConvertRpcStatus(response.precondition().status());

    if (response.precondition().has_valid_duration()) {
      expire_time_ = time_now + utils::ToMilliseonds(
                                    response.precondition().valid_duration());
    } else {
      // never expired.
      expire_time_ = time_point<system_clock>::max();
    }
    use_count_ = response.precondition().valid_use_count();
    route_directive_ = response.precondition().route_directive();
  } else {
    status_ = Status(Code::INVALID_ARGUMENT,
                     "CheckResponse doesn't have PreconditionResult");
    use_count_ = 0;           // 0 for not used this cache.
    expire_time_ = time_now;  // expired now.
  }
}

// check if the item is expired.
bool CheckCache::CacheElem::CacheElem::IsExpired(Tick time_now) {
  if (time_now > expire_time_ || use_count_ == 0) {
    return true;
  }
  if (use_count_ > 0) {
    --use_count_;
  }
  return false;
}

CheckCache::CheckResult::CheckResult() : status_(Code::UNAVAILABLE, "") {}

bool CheckCache::CheckResult::IsCacheHit() const {
  return status_.error_code() != Code::UNAVAILABLE;
}

CheckCache::CheckCache(const CheckOptions &options) : options_(options) {
  if (options.num_entries > 0) {
    cache_.reset(new CheckLRUCache(options.num_entries));
  }
}

CheckCache::~CheckCache() {
  // FlushAll() will remove all cache items.
  FlushAll();
}

void CheckCache::Check(const Attributes &attributes, CheckResult *result) {
  Status status = Check(attributes, system_clock::now(), result);
  if (status.error_code() != Code::NOT_FOUND) {
    result->status_ = status;
  }

  result->on_response_ = [this](const Status &status,
                                const Attributes &attributes,
                                const CheckResponse &response) -> Status {
    if (!status.ok()) {
      if (options_.network_fail_open) {
        return Status::OK;
      } else {
        return status;
      }
    } else {
      return CacheResponse(attributes, response, system_clock::now());
    }
  };
}

Status CheckCache::Check(const Attributes &attributes, Tick time_now,
                         CheckResult *result) {
  if (!cache_) {
    // By returning NOT_FOUND, caller will send request to server.
    return Status(Code::NOT_FOUND, "");
  }

  std::lock_guard<std::mutex> lock(cache_mutex_);
  for (const auto &it : referenced_map_) {
    const Referenced &reference = it.second;
    utils::HashType signature;
    if (!reference.Signature(attributes, "", &signature)) {
      continue;
    }

    CheckLRUCache::ScopedLookup lookup(cache_.get(), signature);
    if (lookup.Found()) {
      CacheElem *elem = lookup.value();
      if (elem->IsExpired(time_now)) {
        cache_->Remove(signature);
        return Status(Code::NOT_FOUND, "");
      }
      if (result) {
        result->route_directive_ = elem->route_directive();
      }
      return elem->status();
    }
  }

  return Status(Code::NOT_FOUND, "");
}

Status CheckCache::CacheResponse(const Attributes &attributes,
                                 const CheckResponse &response, Tick time_now) {
  if (!cache_ || !response.has_precondition()) {
    if (response.has_precondition()) {
      return ConvertRpcStatus(response.precondition().status());
    } else {
      return Status(Code::INVALID_ARGUMENT,
                    "CheckResponse doesn't have PreconditionResult");
    }
  }

  Referenced referenced;
  if (!referenced.Fill(attributes,
                       response.precondition().referenced_attributes())) {
    // Failed to decode referenced_attributes, not to cache this result.
    return ConvertRpcStatus(response.precondition().status());
  }
  utils::HashType signature;
  if (!referenced.Signature(attributes, "", &signature)) {
    MIXER_WARN(
        "Response referenced does not match request.  Request attributes: "
        "%s.  Referenced attributes: %s",
        attributes.DebugString().c_str(), referenced.DebugString().c_str());
    return ConvertRpcStatus(response.precondition().status());
  }

  std::lock_guard<std::mutex> lock(cache_mutex_);
  utils::HashType hash = referenced.Hash();
  if (referenced_map_.find(hash) == referenced_map_.end()) {
    referenced_map_[hash] = referenced;
    MIXER_DEBUG("Add a new Referenced for check cache: %s",
                referenced.DebugString().c_str());
  }

  CheckLRUCache::ScopedLookup lookup(cache_.get(), signature);
  if (lookup.Found()) {
    lookup.value()->SetResponse(response, time_now);
    return lookup.value()->status();
  }

  CacheElem *cache_elem = new CacheElem(*this, response, time_now);
  cache_->Insert(signature, cache_elem, 1);
  return cache_elem->status();
}

// Flush out aggregated check requests, clear all cache items.
// Usually called at destructor.
Status CheckCache::FlushAll() {
  if (cache_) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_->RemoveAll();
  }

  return Status::OK;
}

Status CheckCache::ConvertRpcStatus(const ::google::rpc::Status &status) const {
  // If server status code is INTERNAL, check network_fail_open flag.
  if (status.code() == Code::INTERNAL && options_.network_fail_open) {
    return Status::OK;
  } else {
    return Status(static_cast<Code>(status.code()), status.message());
  }
}

}  // namespace mixerclient
}  // namespace istio
