/**
 * Copyright 2021-2022 Huawei Technologies Co., Ltd
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

#include <gtest/gtest.h>
#include <map>
#include <string>
#include <vector>

#define protected public
#define private public
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/fixpipe_common.h"
#include "common/fe_utils.h"
#include "common/pass_manager.h"
#include "common/fe_fp16_t.h"
#include "common/util/op_info_util.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "platform_info.h"
#undef protected
#undef private
using namespace std;
using namespace ge;
using namespace fe;
namespace fe {
class fixpipe_common_ut : public testing::Test {
 public:
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(fixpipe_common_ut, CalDSize01) {
  size_t data_size;
  data_size = CalDSize(ge::DT_FLOAT);
  EXPECT_EQ(sizeof(uint32_t), data_size);
  data_size = CalDSize(ge::DT_FLOAT16);
  EXPECT_EQ(sizeof(uint16_t), data_size);
  data_size = CalDSize(ge::DT_UINT8);
  EXPECT_EQ(sizeof(uint8_t), data_size);
}

TEST_F(fixpipe_common_ut, split01) {
  std::string input = "normal_relu,scalar_relu,vector_relu,clip_relu";
  std::vector<std::string> outputs = Split(input, ",");
  EXPECT_EQ("normal_relu", outputs[0]);
  EXPECT_EQ("scalar_relu", outputs[1]);
  EXPECT_EQ("vector_relu", outputs[2]);
  EXPECT_EQ("clip_relu", outputs[3]);
}

TEST_F(fixpipe_common_ut, ReadPlatFormConfig01) {
  std::string path = "./air/test/engines/nneng/config/data/platform_config";
  std::string real_path = RealPath(path);
  PlatformInfoManager::Instance().platform_info_map_.clear();
  PlatformInfoManager::Instance().platform_infos_map_.clear();
  uint32_t init_ret = PlatformInfoManager::Instance().LoadConfigFile(real_path);
  EXPECT_EQ(init_ret, ge::GRAPH_SUCCESS);
  std::string soc_version = "Ascend320";
  std::map<std::string, std::map<std::string, std::vector<CONFIGDTYPE>>> fixpipe_map_;
  std::vector<std::string> unit_list;
  std::map<std::string, std::vector<std::string>> depends_units;
  bool ret = ReadPlatFormConfig(soc_version, unit_list, depends_units, fixpipe_map_);
  for (auto iter : fixpipe_map_) {
    std::cout << "  unit_name = " << iter.first << std::endl;
    for (auto iiter : iter.second) {
      std::cout << "      opname = " << iiter.first << std::endl;
      for (auto iiiter : iiter.second) {
        std::cout << "          issecond = " << iiiter.has_output_dtype
                  << "  first = " << static_cast<int>(iiiter.input_dtype)
                  << "  second = " << static_cast<int>(iiiter.output_dtype) << std::endl;
      }
    }
  }
  for (auto iter : depends_units) {
    std::cout << "unit_name = " << iter.first << std::endl;
    for (auto iiter : iter.second) {
      std::cout << "depend unit_name = " << iiter << std::endl;
    }
  }
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_common_ut, ReadPlatFormConfig02) {
  std::string path = "./air/test/engines/nneng/config/data/platform_config";
  std::string real_path = RealPath(path);
  PlatformInfoManager::Instance().platform_info_map_.clear();
  PlatformInfoManager::Instance().platform_infos_map_.clear();
  uint32_t init_ret = PlatformInfoManager::Instance().LoadConfigFile(real_path);
  std::string FIXPIPE_CONFIG_KEY = "Intrinsic_fix_pipe_";
  std::string UINT_LIST_NAME = "unit_list";
  std::string FUNC_LIST = "_func_list";
  std::string soc_version = "Ascend320";

  PlatFormInfos platform_infos;
  OptionalInfos opti_compilation_infos;
  if (PlatformInfoManager::Instance().GetPlatformInfos(soc_version, platform_infos, opti_compilation_infos) ==
      SUCCESS) {
    std::map<std::string, std::vector<std::string>> inputmap_ = opti_compilation_infos.GetFixPipeDtypeMap();
    for (auto iter = inputmap_.begin(); iter != inputmap_.end(); iter++) {
      std::string key = iter->first;
      std::vector<std::string> dtype_vector = iter->second;
      std::cout << "fixpipe_test 3.1 key = " << key << std::endl;
      for (auto values : iter->second) { 
        std::cout << "fixpipe_test 3.2 value = " << values << "  "; 
      }
      std::cout << std::endl;
    }
  }

  EXPECT_EQ(init_ret, ge::GRAPH_SUCCESS);
}

TEST_F(fixpipe_common_ut, Judgedata01) {
  uint8_t *data = nullptr;
  bool ret = false;
  ret = Judgedata(data, 0, ge::DT_UINT32);

  uint32_t ui32 = 6;
  uint16_t ui16 = 6;
  uint8_t ui8 = 6;
  int32_t i32 = 6;
  int32_t i32_1 = -6;
  int16_t i16 = 6;
  int16_t i16_1 = -6;
  int8_t i8 = 6;
  int8_t i8_1 = -6;
  bool b1 = 1;
  bool b0 = 0;
  uint64_t u64 = 0x3200000000;
  fe::fp16_t fp2= ui16;
  EXPECT_EQ(ret, false);
  data = (uint8_t *)&ui32;
  ret = Judgedata(data, 4, ge::DT_UINT32);
  EXPECT_EQ(ret, true);

  data = (uint8_t *)&ui16;
  ret = Judgedata(data, 2, ge::DT_UINT16);
  EXPECT_EQ(ret, true);

  data = (uint8_t *)&ui8;
  ret = Judgedata(data, 1, ge::DT_UINT8);
  EXPECT_EQ(ret, true);

  data = (uint8_t *)&i32;
  ret = Judgedata(data, 4, ge::DT_INT32);
  EXPECT_EQ(ret, true);

  data = (uint8_t *)&i32_1;
  ret = Judgedata(data, 4, ge::DT_INT32);
  EXPECT_EQ(ret, false);

  data = (uint8_t *)&i16;
  ret = Judgedata(data, 2, ge::DT_INT16);
  EXPECT_EQ(ret, true);

  data = (uint8_t *)&i16_1;
  ret = Judgedata(data, 2, ge::DT_INT16);
  EXPECT_EQ(ret, false);

  data = (uint8_t *)&i8;
  ret = Judgedata(data, 1, ge::DT_INT8);
  EXPECT_EQ(ret, true);

  data = (uint8_t *)&i8_1;
  ret = Judgedata(data, 1, ge::DT_INT8);
  EXPECT_EQ(ret, false);

  data = (uint8_t *)&b1;
  ret = Judgedata(data, 1, ge::DT_BOOL);
  EXPECT_EQ(ret, true);

  data = (uint8_t *)&b0;
  ret = Judgedata(data, 1, ge::DT_BOOL);
  EXPECT_EQ(ret, true);

  data = (uint8_t *)&u64;
  ret = Judgedata(data, 8, ge::DT_UINT64);
  EXPECT_EQ(ret, true);

  data = (uint8_t *)&fp2;
  ret = Judgedata(data, 2, ge::DT_FLOAT16);
  EXPECT_EQ(ret, true);

}

}  // namespace fe