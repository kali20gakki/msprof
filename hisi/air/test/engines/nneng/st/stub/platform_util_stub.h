/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#ifndef LLT_FUSION_ENGINE_ST_STUB_PLATFORM_UTIL_STUB_H_
#define LLT_FUSION_ENGINE_ST_STUB_PLATFORM_UTIL_STUB_H_

#include "platform_util/inc/platform_util.h"

namespace tune {

class PlatformUtilStub {
 public:
  static inline void ES3796() {
    ChipInfo &chip_info = PlatformUtil::Instance().chip_info_;
    chip_info.core_num = 1; // 1 core number
    chip_info.cube_size[0] = 16; // 16 M
    chip_info.cube_size[1] = 16; // 16 K
    chip_info.cube_size[2] = 8; //  2 index 8 N
    chip_info.vector_size = 128; // 128 vector size
    chip_info.l1_size = 512 * 1024; // 512 * 1024 l1size
    chip_info.l2_size = 0; // 0 l2size
    chip_info.l0a_size = 32 * 1024; // 32 * 1024 l0asize
    chip_info.l0b_size = 32 * 1024; // 32 * 1024 l0bsize
    chip_info.l0c_size = 128 * 1024; // 128 * 1024 locsize
    chip_info.ub_size = 192 * 1024; // ub_size 192 * 1024
    chip_info.l1tol0a_b_w = 512;  // 512 Byte/cycle
    chip_info.l1tol0b_b_w = 256;  // 320 l1tol0b_b_w
    chip_info.l0cto_ub_w = 512; // TODO: Please check it
    chip_info.ubtol1_b_w = 256;  // 256 ubtol1_b_w
    chip_info.l1toub_b_w = 128; // 128 l1toub_b 
    chip_info.l2read_b_w = 0;
    chip_info.l2write_b_w = 0;
    chip_info.l2_rate = 0;
    chip_info.ddr_b_w = 40; // 20 ddr_b_w
    chip_info.ddr_read_rate = 40;
    chip_info.ddr_write_b_w = 40;
    chip_info.frequency = 300 * 1000 * 1000; // 300 * 1000 * 1000 frequency
    
    PlatformUtil::Instance().soc_version_ = "I don't know";
    return;
  }

  static inline void EVB1910() {
    ChipInfo &chip_info = PlatformUtil::Instance().chip_info_;
    chip_info.core_num = 2; // 2 core number
    chip_info.cube_size[0] = 16; // 16 M
    chip_info.cube_size[1] = 16; // 16 K
    chip_info.cube_size[2] = 16; // 2 index 16 N
    chip_info.vector_size = 256; // 256 vectorsize
    chip_info.l1_size = 1 * 1024 * 1024; // 1 * 1024 * 1024 l1_size
    chip_info.l2_size = 8 * 1024 * 1024; // 8 * 1024 * 1024 l2_size
    chip_info.l0a_size = 64 * 1024; // 64 * 1024 l0a_size
    chip_info.l0b_size = 64 * 1024; // 64 * 1024 l0b_size
    chip_info.l0c_size = 256 * 1024; // 256 * 1024 l0c_size
    chip_info.ub_size = 256 * 1024; // 256 * 1024 ub_size
    chip_info.l1tol0a_b_w = 512;  // 512 Byte/cycle
    chip_info.l1tol0b_b_w = 256; // 256 l1tol0b_b_w 
    chip_info.l0cto_ub_w = 512; // TODO: Please check it
    chip_info.ubtol1_b_w = 512; // 512 ubtol1_b_w
    chip_info.l1toub_b_w = 128; // 128 l1toub_b_w
    chip_info.l2read_b_w = 128; // 128 l2read_b_w
    chip_info.l2write_b_w = 64; // 64 l2write_b_w
    chip_info.l2_rate = 128;
    chip_info.ddr_b_w = 67; // 67 ddr_b_w
    chip_info.ddr_read_rate = 67;
    chip_info.ddr_write_b_w = 64; // 64 ddr_write_b_w
    chip_info.frequency = 680 * 1000 * 1000; // 680 * 1000 * 1000 frequency

    PlatformUtil::Instance().soc_version_ = "ascend310";
    return;
  }
};


};

#endif  // LLT_FUSION_ENGINE_ST_STUB_PLATFORM_UTIL_STUB_H_
