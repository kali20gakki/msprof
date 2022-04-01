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

#include <gtest/gtest.h>
#include <vector>
#include <memory>

#include "common/ge_inner_error_codes.h"
#include "common/util.h"
#include "runtime/mem.h"
#include "omg/omg_inner_types.h"

#define private public
#define protected public
#include "executor/ge_executor.h"
#include "common/auth/file_saver.h"
#include "common/properties_manager.h"
#include "graph/load/graph_loader.h"
#include "graph/load/model_manager/davinci_model.h"
#include "hybrid/hybrid_davinci_model.h"
#include "graph/load/model_manager/model_manager.h"
#include "graph/execute/graph_execute.h"

using namespace std;

namespace ge {
class UtestGraphLoader : public testing::Test {
 protected:
  void SetUp() {
  }
  void TearDown() {
    EXPECT_TRUE(ModelManager::GetInstance().model_map_.empty());
    EXPECT_TRUE(ModelManager::GetInstance().hybrid_model_map_.empty());
  }
};

TEST_F(UtestGraphLoader, LoadDataFromFile) {
  GeExecutor ge_executor;
  ge_executor.is_inited_ = true;

  string test_smap = "/tmp/" + std::to_string(getpid()) + "_maps";
  string self_smap = "/proc/" + std::to_string(getpid()) + "/maps";
  string copy_smap = "cp -f " + self_smap + " " + test_smap;
  EXPECT_EQ(system(copy_smap.c_str()), 0);

  string path = test_smap;
  int32_t priority = 0;
  ModelData model_data;
  auto ret = GraphLoader::LoadDataFromFile(path, priority, model_data);
  EXPECT_EQ(ret, SUCCESS);

  EXPECT_NE(model_data.model_data, nullptr);
  delete[] static_cast<char *>(model_data.model_data);
  model_data.model_data = nullptr;
  ge_executor.is_inited_ = false;
}

TEST_F(UtestGraphLoader, CommandHandle) {
  Command command;
  auto ret = GraphLoader::CommandHandle(command);
  EXPECT_EQ(ret, PARAM_INVALID);
}

TEST_F(UtestGraphLoader, LoadModelFromData) {
  uint32_t model_id = 0;
  ModelData model_data;
  uintptr_t mem_ptr;
  size_t mem_size = 1024;
  uintptr_t weight_ptr;
  size_t weight_size = 0;
  auto ret = GraphLoader::LoadModelFromData(model_id, model_data, mem_ptr, mem_size, weight_ptr, weight_size);
  EXPECT_EQ(ret, ACL_ERROR_GE_EXEC_MODEL_DATA_SIZE_INVALID);
}

TEST_F(UtestGraphLoader, LoadModelWithQ) {
  uint32_t model_id = 0;
  ModelData model_data;
  const std::vector<uint32_t> input_queue_ids;
  const std::vector<uint32_t> output_queue_ids;
  auto ret = GraphLoader::LoadModelWithQ(model_id, model_data, input_queue_ids, output_queue_ids);
  EXPECT_EQ(ret, ACL_ERROR_GE_EXEC_MODEL_DATA_SIZE_INVALID);
}

TEST_F(UtestGraphLoader, LoadModelWithQ2) {
  uint32_t model_id = 0;
  ComputeGraphPtr graph = make_shared<ComputeGraph>("default");
  const GeRootModelPtr root_model = MakeShared<GeRootModel>(graph);;
  const std::vector<uint32_t> input_queue_ids;
  const std::vector<uint32_t> output_queue_ids;
  auto ret = GraphLoader::LoadModelWithQ(model_id, root_model, input_queue_ids, output_queue_ids, false);
  EXPECT_EQ(ret, INTERNAL_ERROR);
}

TEST_F(UtestGraphLoader, ExecuteModel) {
  uint32_t model_id = 0;
  rtStream_t stream;
  bool async_mode;
  InputData input_data;
  std::vector<GeTensorDesc> input_desc;
  OutputData output_data;
  std::vector<GeTensorDesc> output_desc;
  auto ret = GraphLoader::ExecuteModel(model_id, stream, async_mode, input_data, input_desc, output_data, output_desc);
  EXPECT_EQ(ret, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID);
}

TEST_F(UtestGraphLoader, DestroyAicpuKernel) {
  uint64_t session_id;
  uint32_t model_id = 0;
  uint32_t sub_model_id = 0;
  auto ret = ModelManager::GetInstance().DestroyAicpuKernel(session_id, model_id, sub_model_id);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestGraphLoader, DestroyAicpuSessionForInfer) {
  uint32_t model_id = 0;
  auto ret = ModelManager::GetInstance().DestroyAicpuSessionForInfer(model_id);
  EXPECT_EQ(ret, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID);
}

TEST_F(UtestGraphLoader, LoadModelOnline_Invalid) {
  uint32_t model_id = 0;
  GeRootModelPtr ge_root_model = nullptr;
  GraphNodePtr graph_node;
  uint32_t device_id;
  error_message::Context error_context;
  int64_t die_id = 0;

  auto ret = GraphLoader::LoadModelOnline(model_id, ge_root_model, graph_node, device_id, error_context, die_id);
  EXPECT_EQ(ret, GE_GRAPH_PARAM_NULLPTR);

  device_id = UINT32_MAX;
  ge_root_model = make_shared<ge::GeRootModel>();
  ret = GraphLoader::LoadModelOnline(model_id, ge_root_model, graph_node, device_id, error_context, die_id);
  EXPECT_EQ(ret, RT_FAILED);
}

TEST_F(UtestGraphLoader, MultiLoadModelOnline_Invalid) {
  ComputeGraphPtr graph = make_shared<ComputeGraph>("default");
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(graph);

  uint32_t graph_id = 0;
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);

  std::vector<NamedAttrs> deploy_info;
  auto ret = GraphLoader::MultiLoadModelOnline(ge_root_model, graph_node, deploy_info);
  EXPECT_EQ(ret, SUCCESS); // empty instance.
}

TEST_F(UtestGraphLoader, GetMaxUsedMemory_Invalid) {
  uint32_t model_id = 1;
  uint64_t max_size;

  auto ret = ModelManager::GetInstance().GetMaxUsedMemory(model_id, max_size);
  EXPECT_EQ(ret, PARAM_INVALID);
}

TEST_F(UtestGraphLoader, LoadDataFromFile_Invalid) {
  int32_t priority = 0;
  ModelData model_data;
  string path = "/abc/model_file";

  auto ret = GraphLoader::LoadDataFromFile(path, priority, model_data);
  EXPECT_EQ(ret, ACL_ERROR_GE_EXEC_MODEL_PATH_INVALID);
}

}