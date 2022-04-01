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

#include <gtest/gtest.h>
#include <iostream>

#include <list>

#define protected public
#define private public

#include "common/fe_utils.h"
#include "common/format/axis_name_util.h"
#include "common/format/axis_util.h"
#undef private
#undef protected

using namespace std;
using namespace fe;
using namespace ge;

class axis_name_util_utest: public testing::Test
{

protected:
    void SetUp()
    {
    }

    void TearDown()
    {
    }
// AUTO GEN PLEASE DO NOT MODIFY IT
};

namespace{

}

TEST_F(axis_name_util_utest, get_nchw_reshape_type)
{
    ge::Format format = ge::FORMAT_NCHW;
    std::vector<vector<int64_t>> axis_values = {
        {0}, {1}, {2}, {3},
        {0,1}, {0,2}, {0,3}, {1,2}, {1,3}, {2,3},
        {0,1,2},{0,1,3}, {0,2,3}, {1,2,3}
    };
    std::vector<string> reshape_types = {
        "CHW", "NHW", "NCW", "NCH",
        "HW", "CW", "CH", "NW", "NH", "NC",
        "W", "H", "C", "N"
    };
    for (size_t i = 0; i < axis_values.size(); i++) {
      string reshape_type = AxisNameUtil::GetReshapeType(format, axis_values[i]);
      EXPECT_EQ(reshape_type, reshape_types[i]);
    }
}

TEST_F(axis_name_util_utest, get_nhwc_reshape_type)
{
  ge::Format format = ge::FORMAT_NHWC;
  std::vector<vector<int64_t>> axis_values = {
      {0}, {1}, {2}, {3},
      {0,1}, {0,2}, {0,3}, {1,2}, {1,3}, {2,3},
      {0,1,2},{0,1,3},{0,2,3},{1,2,3}
  };
  std::vector<string> reshape_types = {
      "HWC", "NWC", "NHC", "NHW",
      "WC", "HC", "HW", "NC", "NW", "NH",
      "C", "W", "H", "N"
  };
  for (size_t i = 0; i < axis_values.size(); i++) {
    string reshape_type = AxisNameUtil::GetReshapeType(format, axis_values[i]);
    EXPECT_EQ(reshape_type, reshape_types[i]);
  }
}

TEST_F(axis_name_util_utest, get_hwcn_reshape_type)
{
  ge::Format format = ge::FORMAT_HWCN;
  std::vector<vector<int64_t>> axis_values = {
      {0}, {1}, {2}, {3},
      {0,1}, {0,2}, {0,3}, {1,2}, {1,3}, {2,3},
      {0,1,2},{0,1,3},{0,2,3},{1,2,3}
  };
  std::vector<string> reshape_types = {
      "WCN", "HCN", "HWN", "HWC",
      "CN", "WN", "WC", "HN", "HC", "HW",
      "N", "C", "W", "H"
  };
  for (size_t i = 0; i < axis_values.size(); i++) {
    string reshape_type = AxisNameUtil::GetReshapeType(format, axis_values[i]);
    EXPECT_EQ(reshape_type, reshape_types[i]);
  }
}

TEST_F(axis_name_util_utest, get_chwn_reshape_type)
{
  ge::Format format = ge::FORMAT_CHWN;
  std::vector<vector<int64_t>> axis_values = {
      {0}, {1}, {2}, {3},
      {0,1}, {0,2}, {0,3}, {1,2}, {1,3}, {2,3},
      {0,1,2},{0,1,3},{0,2,3},{1,2,3}
  };
  std::vector<string> reshape_types = {
      "HWN", "CWN", "CHN", "CHW",
      "WN", "HN", "HW", "CN", "CW", "CH",
      "N", "W", "H", "C"
  };
  for (size_t i = 0; i < axis_values.size(); i++) {
    string reshape_type = AxisNameUtil::GetReshapeType(format, axis_values[i]);
    EXPECT_EQ(reshape_type, reshape_types[i]);
  }
}

TEST_F(axis_name_util_utest, GetCHWNAxisName_suc)
{
  vector<int64_t> axis_values = {0, 1, 2, 3};
  std::vector<string> axis_name;
  Status ret = AxisNameUtil::GetCHWNAxisName(axis_values, axis_name);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(axis_name_util_utest, check_params_test)
{
  vector<int64_t> original_dim_vec = {0, 1, 2};
  uint32_t c0 = 1;
  vector<int64_t> nd_value = {0, 1, 2, 3};
  size_t dim_default_size = DIM_DEFAULT_SIZE;
  Status status = AxisUtil::CheckParams(original_dim_vec, c0, nd_value, dim_default_size);
  EXPECT_EQ(status, fe::FAILED);
}