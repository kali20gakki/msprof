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
#ifndef INC_A1978929F76A40E88093DE26127A4E25
#define INC_A1978929F76A40E88093DE26127A4E25

#include "stdint.h"
#include "vector"
#include "ge_running_env/fake_ns.h"

FAKE_NS_BEGIN

struct DataBuffer;

struct DataBuffers {
  DataBuffers(int size, int placement);

  DataBuffers(int size, bool is_huge_buffer = false);

  inline std::vector<DataBuffer>& Value(){
    return buffers_;
  }

 private:
  std::vector<DataBuffer> buffers_;
  uint8_t buffer_addr[10240];
  // just for st
  uint8_t huge_buffer_addr[202400*3];
};

FAKE_NS_END

#endif
