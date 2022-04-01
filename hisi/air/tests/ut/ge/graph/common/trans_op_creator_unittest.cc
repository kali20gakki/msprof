/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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

#include <cstdint>
#include <string>
#include <gtest/gtest.h>

#include "common/ge_inner_error_codes.h"
#include "graph/common/trans_op_creator.h"
#include "framework/common/types.h"
#include "graph/utils/attr_utils.h"
#include "framework/common/types.h"
#include "graph/utils/type_utils.h"
#include "graph/debug/ge_attr_define.h"

namespace ge {
class UtestGraphCreateTransOp : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

TEST(UtestGraphCreateTransOp, create_transdata_with_subformat) {
  Format input_format = static_cast<Format>(GetFormatFromSub(FORMAT_FRACTAL_Z, 32));
  auto trans_op = TransOpCreator::CreateTransDataOp("test_trans_data", input_format, FORMAT_NCHW);
  EXPECT_NE(trans_op, nullptr);
  int32_t groups = -1;
  EXPECT_TRUE(AttrUtils::GetInt(trans_op, "groups", groups));
  EXPECT_EQ(groups, 32);

  int32_t sub_srcformat = -1;
  EXPECT_TRUE(AttrUtils::GetInt(trans_op, FORMAT_TRANSFER_SRC_SUBFORMAT, sub_srcformat));
  EXPECT_EQ(sub_srcformat, 32);

  std::string src_format;
  EXPECT_TRUE(AttrUtils::GetStr(trans_op, FORMAT_TRANSFER_SRC_FORMAT, src_format));
  EXPECT_EQ(src_format, TypeUtils::FormatToSerialString(FORMAT_FRACTAL_Z));

  std::string dst_format;
  EXPECT_TRUE(AttrUtils::GetStr(trans_op, FORMAT_TRANSFER_DST_FORMAT, dst_format));
  EXPECT_EQ(dst_format, TypeUtils::FormatToSerialString(FORMAT_NCHW));
}

TEST(UtestGraphCreateTransOp, create_transdata_noneed_groups) {
  Format input_format = static_cast<Format>(GetFormatFromSub(FORMAT_FRACTAL_NZ, 32));
  auto trans_op = TransOpCreator::CreateTransDataOp("test_trans_data", input_format, FORMAT_NCHW);
  EXPECT_NE(trans_op, nullptr);
  int32_t groups = -1;
  EXPECT_FALSE(AttrUtils::GetInt(trans_op, "groups", groups));

  int32_t sub_srcformat = -1;
  EXPECT_TRUE(AttrUtils::GetInt(trans_op, FORMAT_TRANSFER_SRC_SUBFORMAT, sub_srcformat));
  EXPECT_EQ(sub_srcformat, 32);

  std::string src_format;
  EXPECT_TRUE(AttrUtils::GetStr(trans_op, FORMAT_TRANSFER_SRC_FORMAT, src_format));
  EXPECT_EQ(src_format, TypeUtils::FormatToSerialString(FORMAT_FRACTAL_NZ));

  std::string dst_format;
  EXPECT_TRUE(AttrUtils::GetStr(trans_op, FORMAT_TRANSFER_DST_FORMAT, dst_format));
  EXPECT_EQ(dst_format, TypeUtils::FormatToSerialString(FORMAT_NCHW));
}

TEST(UtestGraphCreateTransOp, create_transposed) {
  auto transpos_op = TransOpCreator::CreateTransPoseDOp("test_transposD", FORMAT_NHWC, FORMAT_NCHW);
  EXPECT_NE(transpos_op, nullptr);

  vector<int64_t> perm_args;
  EXPECT_TRUE(AttrUtils::GetListInt(transpos_op, PERMUTE_ATTR_PERM, perm_args));

  EXPECT_TRUE(!perm_args.empty());
}

TEST(UtestGraphCreateTransOp, create_cast) {
  auto cast_op = TransOpCreator::CreateCastOp("test_cast", DT_FLOAT, DT_BOOL);
  EXPECT_NE(cast_op, nullptr);

  int srct;
  EXPECT_TRUE(AttrUtils::GetInt(cast_op, CAST_ATTR_SRCT, srct));
  EXPECT_EQ(DT_FLOAT, static_cast<DataType>(srct));

  int dstt;
  EXPECT_TRUE(AttrUtils::GetInt(cast_op, CAST_ATTR_DSTT, dstt));
  EXPECT_EQ(DT_BOOL, static_cast<DataType>(dstt));
}

}  // namespace ge
