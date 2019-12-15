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

#ifndef ISTIO_MIXERCLIENT_OPTIONS_H
#define ISTIO_MIXERCLIENT_OPTIONS_H

#include <memory>
#include <set>
#include <vector>

namespace istio {
namespace mixerclient {

// Options controlling check behavior.
struct CheckOptions {
  // Default constructor.
  // Default options are chosen from experience.
  CheckOptions() : num_entries(10000) {}

  // Constructor.
  // cache_entries is the maximum number of cache entries that can be kept in
  // the cache. Cache is disabled when cache_entries <= 0.
  CheckOptions(int cache_entries) : num_entries(cache_entries) {}

  // Maximum number of cache entries kept in the cache.
  // Set to 0 will disable caching.
  const int num_entries;

  // If true, Check is passed for any network failures.
  bool network_fail_open{true};

  // Number of retries on transport error
  uint32_t retries{0};

  // Base milliseconds to sleep between retries.  Will be adjusted by
  // exponential backoff and jitter.
  uint32_t base_retry_ms{80};

  // Max milliseconds to sleep between retries.
  uint32_t max_retry_ms{1000};
};

const int DEFAULT_BATCH_REPORT_MAX_ENTRIES = 100;
const int DEFAULT_BATCH_REPORT_MAX_TIME_MS = 1000;

// Options controlling report batch.
struct ReportOptions {
  // Default constructor.
  // Default to batch up to 100 reports or 1000 milliseconds (1 second).
  ReportOptions()
      : max_batch_entries(DEFAULT_BATCH_REPORT_MAX_ENTRIES),
        max_batch_time_ms(DEFAULT_BATCH_REPORT_MAX_TIME_MS) {}

  // Constructor.
  ReportOptions(int max_batch_entries, int max_batch_time_ms)
      : max_batch_entries(max_batch_entries),
        max_batch_time_ms(max_batch_time_ms) {}

  // Maximum number of reports to be batched.
  const int max_batch_entries;

  // Maximum milliseconds a report item stayed in the buffer for batching.
  const int max_batch_time_ms;
};

// Options controlling quota behavior.
struct QuotaOptions {
  // Default constructor.
  QuotaOptions() : num_entries(10000), expiration_ms(600000) {}

  // Constructor.
  // cache_entries is the maximum number of cache entries that can be kept in
  // the cache. Cache is disabled when cache_entries <= 0.
  // expiration_ms is the maximum milliseconds an idle cached quota is removed.
  QuotaOptions(int cache_entries, int expiration_ms)
      : num_entries(cache_entries), expiration_ms(expiration_ms) {}

  // Maximum number of cache entries kept in the cache.
  // Set to 0 will disable caching.
  const int num_entries;

  // Maximum milliseconds before an idle cached quota should be deleted.
  const int expiration_ms;
};

}  // namespace mixerclient
}  // namespace istio

#endif  // ISTIO_MIXERCLIENT_OPTIONS_H
