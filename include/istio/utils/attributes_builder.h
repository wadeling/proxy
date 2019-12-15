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

#ifndef ISTIO_UTILS_ATTRIBUTES_BUILDER_H
#define ISTIO_UTILS_ATTRIBUTES_BUILDER_H

#include <chrono>
#include <map>
#include <string>

#include "google/protobuf/struct.pb.h"
#include "mixer/v1/attributes.pb.h"

namespace istio {
namespace utils {

const char kMixerMetadataKey[] = "istio.mixer";

// Builder class to add attribute to protobuf Attributes.
// Its usage:
//    builder(attribute).Add("key", value)
//                      .Add("key2", value2);
class AttributesBuilder {
 public:
  AttributesBuilder(::istio::mixer::v1::Attributes *attributes)
      : attributes_(attributes) {}

  void AddString(const std::string &key, const std::string &str) {
    (*attributes_->mutable_attributes())[key].set_string_value(str);
  }

  void AddBytes(const std::string &key, const std::string &bytes) {
    (*attributes_->mutable_attributes())[key].set_bytes_value(bytes);
  }

  void AddInt64(const std::string &key, int64_t value) {
    (*attributes_->mutable_attributes())[key].set_int64_value(value);
  }

  void AddDouble(const std::string &key, double value) {
    (*attributes_->mutable_attributes())[key].set_double_value(value);
  }

  void AddBool(const std::string &key, bool value) {
    (*attributes_->mutable_attributes())[key].set_bool_value(value);
  }

  void AddTimestamp(
      const std::string &key,
      const std::chrono::time_point<std::chrono::system_clock> &value) {
    auto time_stamp =
        (*attributes_->mutable_attributes())[key].mutable_timestamp_value();
    long long nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
                          value.time_since_epoch())
                          .count();
    time_stamp->set_seconds(nanos / 1000000000);
    time_stamp->set_nanos(nanos % 1000000000);
  }

  void AddDuration(const std::string &key,
                   const std::chrono::nanoseconds &value) {
    auto duration =
        (*attributes_->mutable_attributes())[key].mutable_duration_value();
    duration->set_seconds(value.count() / 1000000000);
    duration->set_nanos(value.count() % 1000000000);
  }

  void AddStringMap(const std::string &key,
                    const std::map<std::string, std::string> &string_map) {
    if (string_map.size() == 0) {
      return;
    }
    auto entries = (*attributes_->mutable_attributes())[key]
                       .mutable_string_map_value()
                       ->mutable_entries();
    entries->clear();
    for (const auto &map_it : string_map) {
      (*entries)[map_it.first] = map_it.second;
    }
  }

  void InsertStringMap(const std::string &key,
                       const std::map<std::string, std::string> &string_map) {
    if (string_map.size() == 0) {
      return;
    }
    auto entries = (*attributes_->mutable_attributes())[key]
                       .mutable_string_map_value()
                       ->mutable_entries();
    for (const auto &map_it : string_map) {
      (*entries)[map_it.first] = map_it.second;
    }
  }

  void AddProtoStructStringMap(const std::string &key,
                               const google::protobuf::Struct &struct_map) {
    if (struct_map.fields().empty()) {
      return;
    }
    auto entries = (*attributes_->mutable_attributes())[key]
                       .mutable_string_map_value()
                       ->mutable_entries();
    entries->clear();
    for (const auto &field : struct_map.fields()) {
      // Ignore all fields that are not string or string list.
      switch (field.second.kind_case()) {
        case google::protobuf::Value::kStringValue:
          (*entries)[field.first] = field.second.string_value();
          break;
        case google::protobuf::Value::kListValue:
          if (field.second.list_value().values_size() > 0) {
            // The items in the list is converted into a
            // comma separated string
            std::string s;
            for (int i = 0; i < field.second.list_value().values_size(); i++) {
              s += field.second.list_value().values().Get(i).string_value();
              if (i + 1 < field.second.list_value().values_size()) {
                s += ",";
              }
            }
            (*entries)[field.first] = s;
          }
          break;
        default:
          break;
      }
    }

    if (entries->empty()) {
      attributes_->mutable_attributes()->erase(key);
    }
  }

  // Serializes all the keys in a map<string, struct> and builds attributes.
  // for example, foo.bar.com: struct {str:abc, list:[c,d,e]} will be emitted as
  // foo.bar.com: string_map[str:abc, list: c,d,e]
  // Only extracts strings and lists.
  // TODO: add the ability to pack bools and nums as strings and recurse down
  // the struct.
  void FlattenMapOfStringToStruct(
      const ::google::protobuf::Map<::std::string, ::google::protobuf::Struct>
          &filter_state) {
    if (filter_state.empty()) {
      return;
    }

    for (const auto &filter : filter_state) {
      if (FiltersToIgnore().find(filter.first) == FiltersToIgnore().end()) {
        AddProtoStructStringMap(filter.first, filter.second);
      }
    }
  }

  bool HasAttribute(const std::string &key) const {
    const auto &attrs_map = attributes_->attributes();
    return attrs_map.find(key) != attrs_map.end();
  }

 private:
  const std::unordered_set<std::string> &FiltersToIgnore() {
    static const auto *filters =
        new std::unordered_set<std::string>{kMixerMetadataKey};
    return *filters;
  }

  // TODO(jblatt) audit all uses of raw pointers and replace as many as possible
  // with unique/shared pointers.
  ::istio::mixer::v1::Attributes *attributes_;
};

}  // namespace utils
}  // namespace istio

#endif  // ISTIO_MIXERCLIENT_ATTRIBUTES_BUILDER_H
