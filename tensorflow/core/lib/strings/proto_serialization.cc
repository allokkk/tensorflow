/* Copyright 2018 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#include "tensorflow/core/lib/strings/proto_serialization.h"

#include <cstring>
#include "absl/memory/memory.h"
#include "absl/strings/string_view.h"
#include "tensorflow/core/lib/hash/hash.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/macros.h"

namespace tensorflow {

bool SerializeToStringDeterministic(const protobuf::MessageLite& msg,
                                    string* result) {
  const size_t size = msg.ByteSizeLong();
  DCHECK_LE(size, static_cast<size_t>(INT_MAX));
  *result = string(size, '\0');
  return SerializeToBufferDeterministic(msg, const_cast<char*>(result->data()),
                                        result->size());
}

bool SerializeToBufferDeterministic(const protobuf::MessageLite& msg,
                                    char* buffer, size_t size) {
  DCHECK(msg.ByteSizeLong() == size && size <= static_cast<size_t>(INT_MAX));
  protobuf::io::ArrayOutputStream array_stream(buffer, size);
  protobuf::io::CodedOutputStream output_stream(&array_stream);
  output_stream.SetSerializationDeterministic(true);
  msg.SerializeWithCachedSizes(&output_stream);
  return !output_stream.HadError() && size == output_stream.ByteCount();
}

bool AreSerializedProtosEqual(const protobuf::MessageLite& x,
                              const protobuf::MessageLite& y) {
  const size_t size = x.ByteSizeLong();
  if (size != y.ByteSizeLong()) return false;
  if (size == 0) return true;
  auto x_serialized = absl::make_unique<char[]>(size);
  bool success_x = SerializeToBufferDeterministic(x, x_serialized.get(), size);
  DCHECK(success_x);
  auto y_serialized = absl::make_unique<char[]>(size);
  bool success_y = SerializeToBufferDeterministic(y, y_serialized.get(), size);
  DCHECK(success_y);
  return memcmp(x_serialized.get(), y_serialized.get(), size) == 0;
}

uint64 DeterministicProtoHash64(const protobuf::MessageLite& proto,
                                uint64 seed) {
  const size_t size = proto.ByteSizeLong();
  auto serialized = absl::make_unique<char[]>(size);
  SerializeToBufferDeterministic(proto, serialized.get(), size);
  return Hash64(serialized.get(), size, seed);
}

uint64 DeterministicProtoHash64(const protobuf::MessageLite& proto) {
  const size_t size = proto.ByteSizeLong();
  auto serialized = absl::make_unique<char[]>(size);
  SerializeToBufferDeterministic(proto, serialized.get(), size);
  return Hash64(serialized.get(), size);
}

}  // namespace tensorflow
