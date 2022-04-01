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

#include <gtest/gtest.h>
#define private public
#include "graph/load/model_manager/model_manager.h"
#undef private

#include "external/ge/ge_api.h"
#include "graph/compute_graph.h"
#include "framework/common/types.h"
#include "graph/ge_local_context.h"
#include "graph/execute/model_executor.h"
#include "graph/load/graph_loader.h"
#include "hybrid/node_executor/node_executor.h"
#include "hybrid/node_executor/ge_local/ge_local_node_executor.h"
#include "hybrid/node_executor/aicore/aicore_node_executor.h"

#include "ge_graph_dsl/graph_dsl.h"

namespace ge {
namespace {
const int kAippModeDynamic = 2;
}
namespace hybrid {
REGISTER_NODE_EXECUTOR_BUILDER(NodeExecutorManager::ExecutorType::AICORE, AiCoreNodeExecutor);
REGISTER_NODE_EXECUTOR_BUILDER(NodeExecutorManager::ExecutorType::GE_LOCAL, GeLocalNodeExecutor);
}

class STEST_opt_info : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

///     data   aipp_data
///       \    /
///        aipp
ComputeGraphPtr BuildAippGraph() {
  NamedAttrs aipp_attr;
  aipp_attr.SetAttr("aipp_mode", GeAttrValue::CreateFrom<int64_t>(kAippModeDynamic));
  aipp_attr.SetAttr("related_input_rank", GeAttrValue::CreateFrom<int64_t>(0));
  aipp_attr.SetAttr("max_src_image_size", GeAttrValue::CreateFrom<int64_t>(2048));
  aipp_attr.SetAttr("support_rotation", GeAttrValue::CreateFrom<int64_t>(1));

  auto data = OP_CFG(DATA).Attr(ATTR_NAME_AIPP, aipp_attr)
                          .Attr(ATTR_DATA_RELATED_AIPP_MODE, "dynamic_aipp")
                          .Attr(ATTR_DATA_AIPP_DATA_NAME_MAP, "aipp_data");
  auto aipp_data = OP_CFG(AIPP_DATA_TYPE).Attr(ATTR_NAME_AIPP, aipp_attr)
                                         .Attr(ATTR_DATA_RELATED_AIPP_MODE, "dynamic_aipp")
                                         .Attr(ATTR_DATA_AIPP_DATA_NAME_MAP, "aipp_data");
  auto aipp = OP_CFG(AIPP).Attr(ATTR_NAME_AIPP, aipp_attr);
  DEF_GRAPH(g1) {
    CHAIN(NODE("data", data)->EDGE(0, 0)->NODE("aipp", aipp));
    CHAIN(NODE("aipp_data", aipp_data)->EDGE(0, 1)->NODE("aipp"));
  };
  ComputeGraphPtr compute_graph = ToComputeGraph(g1);
  compute_graph->SetGraphUnknownFlag(true);
  for (auto &node : compute_graph->GetDirectNode()) {
    if (node->GetOpDesc() == nullptr) {
      continue;
    }
    if (node->GetType() == DATA || node->GetType() == AIPP_DATA_TYPE) {
      node->GetOpDesc()->SetOpKernelLibName(kEngineNameGeLocal);
    } else {
      node->GetOpDesc()->SetOpKernelLibName(kEngineNameAiCore);
    }

  }
  return compute_graph;
}

TEST_F(STEST_opt_info, record_ts_snapshot) {
  const std::string kTriggerFile = "exec_record_trigger";
  const char_t * const kEnvRecordPath = "NPU_COLLECT_PATH";
  const std::string kRecordFilePrefix = "exec_record_";

  // 设置环境变量
  char_t npu_collect_path[MMPA_MAX_PATH] = {};
  mmRealPath(".", &npu_collect_path[0U], MMPA_MAX_PATH);
  mmSetEnv(kEnvRecordPath, &npu_collect_path[0U], 1);

  // 创建trigger文件
  const std::string trigger_file = std::string(&npu_collect_path[0U]) + "/" + kTriggerFile;
  auto trigger_fd = mmOpen(trigger_file.c_str(), M_WRONLY | M_CREAT);
  mmClose(trigger_fd);

  ModelManager *mm = new ModelManager();
  mmSleep(1000U);

  std::string record_file = std::string(&npu_collect_path[0U]) + "/" + kRecordFilePrefix + std::to_string(mmGetPid());
  const auto record_fd = mmOpen(record_file.c_str(), M_RDONLY);
  EXPECT_TRUE(record_fd >= 0);
  mmClose(record_fd);

  delete mm;
  trigger_fd = mmOpen(trigger_file.c_str(), M_RDONLY);
  EXPECT_TRUE(trigger_fd < 0);

  // 清理环境变量
  mmSetEnv(kEnvRecordPath, "", 1);
  // 清理record file
  mmUnlink(record_file.c_str());
}

}  // namespace ge
