/**
 * Copyright 2021 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "utils/data_buffers.h"
#include "framework/common/ge_types.h"

USING_FAKE_NS;

DataBuffers::DataBuffers(int size, bool is_huge_buffer) {
  if (is_huge_buffer) {
    for (int i = 0; i < size; i++) {
        buffers_.emplace_back(DataBuffer(huge_buffer_addr + i * 202400, 202400, false));
    }
  } else {
    for (int i = 0; i < size; i++) {
      buffers_.emplace_back(DataBuffer(buffer_addr + i * 4, 202400, false));
    }
  }
}

DataBuffers::DataBuffers(int size, int placement) {
  if (placement == 1) {
    for (int i = 0; i < size; i++) {
      buffers_.emplace_back(DataBuffer(buffer_addr + i * 4, 10240, false));
    }
  } else {
    for (int i = 0; i < size; i++) {
      buffers_.emplace_back(DataBuffer(buffer_addr + i * 4, 202400, false));
    }
  }
}