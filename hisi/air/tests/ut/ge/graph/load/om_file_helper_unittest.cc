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
#define private public
#define protected public
#include "framework/common/helper/om_file_helper.h"
#include "common/auth/file_saver.h"
#include "common/math/math_util.h"
#include "framework/common/debug/ge_log.h"
#include "framework/common/debug/log.h"
#include "framework/common/ge_inner_error_codes.h"
#include "framework/common/util.h"
#include "graph/def_types.h"
#undef private
#undef protected

#include "proto/task.pb.h"

using namespace std;

namespace ge {
class UtestOmFileHelper : public testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(UtestOmFileHelper, Normal)
{
  OmFileLoadHelper loader;
  OmFileSaveHelper saver;
}

TEST_F(UtestOmFileHelper, LoadInit)
{
  OmFileLoadHelper loader;
  ModelData md;
  EXPECT_EQ(loader.Init(md), FAILED);
  ModelFileHeader header;
  md.model_data = &header;
  md.model_len = sizeof(ModelFileHeader) + sizeof(ModelPartitionTable) + 10;
  header.length = md.model_len - sizeof(ModelFileHeader);
  header.magic = MODEL_FILE_MAGIC_NUM;
  EXPECT_EQ(loader.Init(md), ACL_ERROR_GE_PARAM_INVALID);
}

TEST_F(UtestOmFileHelper, GetModelPartition)
{
  OmFileLoadHelper loader;
  loader.is_inited_ = true;
  ModelPartitionType type;
  ModelPartition partition;
  size_t model_index = 10;
  EXPECT_EQ(loader.GetModelPartition(type, partition, model_index), PARAM_INVALID);
  loader.is_inited_ = false;
  EXPECT_EQ(loader.GetModelPartition(type, partition, model_index), PARAM_INVALID);
  loader.is_inited_ = true;
  model_index = 0;
  type = TASK_INFO;
  loader.context_.partition_datas_.clear();
  EXPECT_EQ(loader.GetModelPartition(type, partition, model_index), PARAM_INVALID);
}

TEST_F(UtestOmFileHelper, GetModelPartitionNoIndex)
{
  OmFileLoadHelper loader;
  ModelPartitionType type;
  ModelPartition partition;
  loader.is_inited_ = false;
  EXPECT_EQ(loader.GetModelPartition(type, partition), PARAM_INVALID);
  loader.is_inited_ = true;
  type = TASK_INFO;
  loader.context_.partition_datas_.clear();
  EXPECT_EQ(loader.GetModelPartition(type, partition), FAILED);
}

TEST_F(UtestOmFileHelper, LoadModelPartitionTable)
{
  OmFileLoadHelper loader;
  uint8_t* model_data = nullptr;
  uint32_t model_data_size = 0;
  EXPECT_EQ(loader.LoadModelPartitionTable(model_data, model_data_size), ACL_ERROR_GE_EXEC_MODEL_ADDR_INVALID);
}

TEST_F(UtestOmFileHelper, LoadModelPartitionTableWithNum)
{
  OmFileLoadHelper loader;
  uint8_t* model_data = nullptr;
  uint32_t model_data_size = 0;
  uint32_t model_num = 0;
  EXPECT_EQ(loader.LoadModelPartitionTable(model_data, model_data_size, model_num), PARAM_INVALID);
}

TEST_F(UtestOmFileHelper, SaverGetPartitionTable)
{
  OmFileSaveHelper saver;
  OmFileContext oc;
  oc.partition_datas_.push_back(ModelPartition());
  saver.model_contexts_.push_back(oc);
  size_t cur_ctx_index = 0;
  EXPECT_NE(saver.GetPartitionTable(cur_ctx_index), nullptr);
}

TEST_F(UtestOmFileHelper, SaveRootModel)
{
  OmFileSaveHelper saver;
  OmFileContext oc;
  oc.model_data_len_ = 1024;
  saver.model_contexts_.push_back(oc);
  SaveParam save_param;
  const char_t *const output_file = "root.model";
  ModelBufferData model;
  const bool is_offline = true;
  EXPECT_NE(saver.SaveRootModel(save_param, output_file, model, is_offline), SUCCESS);
}

TEST_F(UtestOmFileHelper, AddPartition)
{
  OmFileSaveHelper saver;
  ModelPartition partition;
  EXPECT_EQ(saver.AddPartition(partition), SUCCESS);
  saver.context_.model_data_len_ = 4000000000;
  partition.size = 2000000000;
  EXPECT_EQ(saver.AddPartition(partition), FAILED);
}

}  // namespace ge
