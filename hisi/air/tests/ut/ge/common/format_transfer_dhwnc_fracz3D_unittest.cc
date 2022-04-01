/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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

#include "common/formats/format_transfers/format_transfer_dhwcn_fracz3D.h"
#include "common/fp16_t.h"
#include "framework/common/ge_inner_error_codes.h"

namespace ge {
namespace formats {

class UtestFormatTransferDhwcnFractalZ3D : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(UtestFormatTransferDhwcnFractalZ3D, dhwcn_invalid_data_type_string) {
  uint8_t data[1 * 4 * 4 * 1 * 16 * 16] = {1};

  TransArgs args{data, FORMAT_DHWCN, FORMAT_FRACTAL_Z_3D, {1, 4, 4, 1, 16, 16}, {4, 4, 3, 1}, DT_STRING};
  TransResult result;

  FormatTransferDhwcnFractalZ3D transfer;
  Status ret = transfer.TransFormat(args, result);
  EXPECT_EQ(ret, ACL_ERROR_GE_DATATYPE_INVALID);
}

TEST_F(UtestFormatTransferDhwcnFractalZ3D, dhwcn_data_type_uint8_invalid_shape) {
  uint8_t data[1 * 4 * 4 * 1 * 16 * 16] = {1};

  TransArgs args{data, FORMAT_DHWCN, FORMAT_FRACTAL_Z_3D, {1, 4, 4, 1, 16, 16}, {4, 4, 3, 1}, DT_UINT8};
  TransResult result;

  FormatTransferDhwcnFractalZ3D transfer;
  Status ret = transfer.TransFormat(args, result);
  EXPECT_EQ(ret, ACL_ERROR_GE_SHAPE_INVALID);
}

TEST_F(UtestFormatTransferDhwcnFractalZ3D, dhwcn_data_type_uint8_invalid_format) {
  uint8_t data[1 * 4 * 4 * 1 * 16 * 16] = {1};

  TransArgs args{data, FORMAT_DHWCN, FORMAT_DHWCN, {1, 4, 4, 1, 16, 16}, {4, 4, 3, 1}, DT_UINT8};
  TransResult result;

  FormatTransferDhwcnFractalZ3D transfer;
  Status ret = transfer.TransFormat(args, result);
  EXPECT_EQ(ret, ACL_ERROR_GE_FORMAT_INVALID);
}

TEST_F(UtestFormatTransferDhwcnFractalZ3D, dhwcn_data_type_uint8_dst_invalid_shape) {
  uint8_t data[1 * 4 * 4 * 16 * 16] = {1};

  TransArgs args{data, FORMAT_DHWCN, FORMAT_FRACTAL_Z_3D, {1, 4, 4, 16, 16}, {4, 4, 3, 1}, DT_UINT8};
  TransResult result;

  FormatTransferDhwcnFractalZ3D transfer;
  Status ret = transfer.TransFormat(args, result);
  EXPECT_EQ(ret, ACL_ERROR_GE_SHAPE_INVALID);
}

TEST_F(UtestFormatTransferDhwcnFractalZ3D, dhwcn_data_type_uint8_success) {
  uint8_t data[1 * 4 * 4 * 16 * 16] = {1};

  TransArgs args{data, FORMAT_DHWCN, FORMAT_FRACTAL_Z_3D, {1, 4, 4, 16, 16}, {16, 1, 16, 32}, DT_UINT8};
  TransResult result;

  FormatTransferDhwcnFractalZ3D transfer;
  Status ret = transfer.TransFormat(args, result);
  EXPECT_EQ(ret, SUCCESS);
}

}  // namespace formats
}  // namespace ge
