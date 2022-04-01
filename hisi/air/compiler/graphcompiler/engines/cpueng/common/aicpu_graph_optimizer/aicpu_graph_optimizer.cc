/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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

#include "aicpu_graph_optimizer.h"

#include "common/util/platform_info.h"
#include "config/config_file.h"
#include "engine/base_engine.h"
#include "error_code/error_code.h"
#include "ge/ge_api_types.h"
#include "graph_optimizer_utils.h"
#include "optimizer.h"
#include "util/log.h"
#include "util/util.h"
#include "util/constant.h"

using namespace ge;
using namespace std;

namespace {
// need delete when update libgraph.so in blue zone
const std::string AICPU_ATTR_NAME_OP_TILING_INLINE_ENGINE = "_op_tiling_inline_engine";
const std::string AICPU_ATTR_NAME_OP_EXPORT_SHAPE_ENGINE = "_op_export_shape_engine";
}

namespace aicpu {
ge::Status AicpuGraphOptimizer::Initialize(const map<string, string> &options,
                                           ge::OptimizeUtility *const optimize_utility) {
  // initial optimizers
  auto iter = options.find(ge::SOC_VERSION);
  AICPU_IF_BOOL_EXEC(iter == options.end(),
      AICPU_REPORT_INNER_ERROR(
          "cannot find [%s] in param of optimizer initialize function.",
          ge::SOC_VERSION.c_str());
      return INPUT_PARAM_VALID)
  soc_version_ = iter->second;
  std::string auto_cast_mode;
  if (ConfigFile::GetInstance().GetValue(kAutoCastMode, auto_cast_mode)) {
    auto_cast_mode_ = kAutoCastModeOff;
    if (StringToNum(auto_cast_mode, auto_cast_mode_).state != ge::SUCCESS) {
      AICPUE_LOGW("Tran auto_cast_mode[%s] to integer failed. default value is [%d].",
                  auto_cast_mode.c_str(), auto_cast_mode_);
    }
  } else {
    AICPUE_LOGW("Get Value[AutoCastMode] failed");
  }
  AICPU_CHECK_RES(Finalize())
  string optimizers_str;
  string optimizers_config = Stringcat(engine_name_, "GraphOptimizer");
  AICPU_IF_BOOL_EXEC(
      !ConfigFile::GetInstance().GetValue(optimizers_config, optimizers_str),
      AICPU_REPORT_INNER_ERROR("[%s] not exist.", optimizers_config.c_str());
      return LOAD_OPTIMIZER_CONFIG_FAILED)
  vector<string> optimizers;
  ConfigFile::GetInstance().SplitValue(optimizers_str, optimizers);
  for (auto optimizer : optimizers) {
    FACTORY_GRAPH_OPTIMIZER::FactoryType optimizers_ptr =
        FACTORY_GRAPH_OPTIMIZER::Produce(optimizer);
    AICPU_IF_BOOL_EXEC(optimizers_ptr == nullptr,
        AICPU_REPORT_INNER_ERROR("[%s] instantiate failed", optimizer.c_str());
        return GRAPH_OPTIMIZER_INSTANCE_FAILED)
    optimizer_map_[optimizer] = optimizers_ptr;
    AICPU_CHECK_RES(optimizers_ptr->Initialize());
  }
  // get ops parallel rule info
  GetOpsParallelRule();
  return SUCCESS;
}

ge::Status AicpuGraphOptimizer::Finalize() {
  optimizer_map_.clear();
  ops_parallel_rule_infos_.clear();
  return SUCCESS;
}

ge::Status AicpuGraphOptimizer::GetAttributes(
    GraphOptimizerAttribute &attrs) const {
  attrs.engineName = engine_name_;
  return SUCCESS;
}

ge::Status AicpuGraphOptimizer::OptimizeOriginalGraph(ge::ComputeGraph &graph) {
  return AddTilingModeAttr(graph);
}

ge::Status AicpuGraphOptimizer::AddTilingModeAttr(ge::ComputeGraph &graph) const {
  if (IsEmptyGraph(graph)) {
    return SUCCESS;
  }
  map<string, OpFullInfo> all_op_info;
  AICPU_CHECK_RES(GetOpsInfo(all_op_info));
  for (const NodePtr &curr_node : graph.GetDirectNode()) {
    AICPU_CHECK_NOTNULL(curr_node)
    OpDescPtr curr_op_desc_ptr = curr_node->GetOpDesc();
    AICPU_CHECK_NOTNULL(curr_op_desc_ptr)
    string op_type = curr_op_desc_ptr->GetType();

    auto const iter = all_op_info.find(op_type);
    if (iter == all_op_info.end()) {
      AICPUE_LOGI("Op type[%s] is not exist in current kernel librarys.", op_type.c_str());
    } else {
      auto op_info = iter->second;
      bool no_tiling = op_info.noTiling;
      AICPUE_LOGD("Op type[%s], no tiling[%d].", op_type.c_str(), no_tiling);
      if (no_tiling) {
        vector<string> engines;
        (void)AttrUtils::GetListStr(curr_op_desc_ptr, AICPU_ATTR_NAME_OP_TILING_INLINE_ENGINE, engines);
        engines.emplace_back(engine_name_);
        bool ret = AttrUtils::SetListStr(curr_op_desc_ptr, AICPU_ATTR_NAME_OP_TILING_INLINE_ENGINE, engines);
        AICPU_IF_BOOL_EXEC(!(ret),
                           AICPU_REPORT_CALL_ERROR("Call ge::AttrUtils::SetListStr failed to set attr[%s], op[%s]",
                                                   AICPU_ATTR_NAME_OP_TILING_INLINE_ENGINE.c_str(),
                                                   curr_op_desc_ptr->GetName().c_str());
                           return FAILED)
        engines.clear();
        (void)AttrUtils::GetListStr(curr_op_desc_ptr, AICPU_ATTR_NAME_OP_EXPORT_SHAPE_ENGINE, engines);
        engines.emplace_back(engine_name_);
        ret = AttrUtils::SetListStr(curr_op_desc_ptr, AICPU_ATTR_NAME_OP_EXPORT_SHAPE_ENGINE, engines);
        AICPU_IF_BOOL_EXEC(!(ret),
                           AICPU_REPORT_CALL_ERROR("Call ge::AttrUtils::SetListStr failed to set attr[%s], op[%s]",
                                                   AICPU_ATTR_NAME_OP_EXPORT_SHAPE_ENGINE.c_str(),
                                                   curr_op_desc_ptr->GetName().c_str());
                           return FAILED)
        AICPUE_LOGD("Add no tiling engine[%s] for op type[%s], attrs[%s, %s].", engine_name_.c_str(), op_type.c_str(),
                    AICPU_ATTR_NAME_OP_TILING_INLINE_ENGINE.c_str(), AICPU_ATTR_NAME_OP_EXPORT_SHAPE_ENGINE.c_str());
      }
    }
  }
  return SUCCESS;
}

ge::Status AicpuGraphOptimizer::OptimizeOriginalGraphJudgeInsert(
    ComputeGraph &graph) {
  if (IsEmptyGraph(graph)) {
    return SUCCESS;
  }
  map<string, OpFullInfo> all_op_info;
  AICPU_CHECK_RES(GetOpsInfo(all_op_info));
  for (auto optimizer : optimizer_map_) {
    AICPU_CHECK_RES(
        optimizer.second->OptimizeOriginalGraphJudgeInsert(graph, all_op_info))
  }
  return SUCCESS;
}

ge::Status AicpuGraphOptimizer::OptimizeFusedGraph(ComputeGraph &graph) {
  string graph_name = graph.GetName();
  AICPUE_LOGI("begin to optimizer graph[%s]", graph_name.c_str());
  if (IsEmptyGraph(graph)) {
    return SUCCESS;
  }
  // vertify placehold and end node make sure format equals client format
  AICPU_CHECK_RES(GraphOptimizerUtils::VerifyPldAndEndNode(graph))

  string suffix = "Before_Aicpu_Optimized";
  GraphOptimizerUtils::DumpGraph(graph, suffix);

  fe::PlatFormInfos platform_infos;
  fe::OptionalInfos opt_infos;
  AICPU_IF_BOOL_EXEC(
      fe::PlatformInfoManager::Instance().GetPlatformInfos(soc_version_,
          platform_infos, opt_infos) != ge::GRAPH_SUCCESS,
      AICPU_REPORT_CALL_ERROR("Call fe::PlatformInfoManager::GetPlatformInfos "
          "function failed. soc version[%s]", soc_version_.c_str());
      return ge::FAILED)
  // cache coherence need to be guaranteed
  string aicpu_cache_enable;
  AICPU_IF_BOOL_EXEC(
      !platform_infos.GetPlatformRes("CPUCache", "AICPUSyncBySW", aicpu_cache_enable),
      AICPU_REPORT_CALL_ERROR("Call fe::PlatFormInfos::GetPlatformRes failed to"
          " get aicpu cache synchronous status");
      return ge::FAILED)
  AICPUE_LOGI("aicpu cache enable flag[%s]", aicpu_cache_enable.c_str());
  if (aicpu_cache_enable.find("1") != string::npos) {
    AICPU_CHECK_RES_WITH_LOG(CacheGraph::GenerateNoCacheGraph(graph),
        "Call GenerateCacheGraph function failed, graph[%s].",
        graph_name.c_str())
    suffix = "After_Insert_Cache_op";
    GraphOptimizerUtils::DumpGraph(graph, suffix);
  }
  map<string, OpFullInfo> all_op_info;
  AICPU_CHECK_RES(GetOpsInfo(all_op_info));

  if (auto_cast_mode_ == kAutoCastModeOn) {
    AICPU_CHECK_RES_WITH_LOG(AutoCastGraph::GenerateAutoCastGraph(graph, all_op_info), "AutoCast failed");
    suffix = "After_Auto_Cast";
    GraphOptimizerUtils::DumpGraph(graph, suffix);
  }

  SetStreamLabelForOpsParallel(graph);

  for (auto optimizer : optimizer_map_) {
    AICPU_CHECK_RES(optimizer.second->OptimizeFusedGraph(graph, all_op_info))
  }
  suffix = "After_Aicpu_Optimized";
  GraphOptimizerUtils::DumpGraph(graph, suffix);
  AICPUE_LOGI("success to optimizer graph[%s]", graph_name.c_str());
  return ge::SUCCESS;
}

bool AicpuGraphOptimizer::IsEmptyGraph(const ComputeGraph &graph) const {
  string graph_name = graph.GetName();
  if (graph.GetDirectNodesSize() == 0) {
    AICPUE_LOGW("No ge node exists in graph[%s].", graph_name.c_str());
    return true;
  }
  return false;
}

ge::Status AicpuGraphOptimizer::GetOpsInfo(
    map<string, OpFullInfo> &all_op_info) const {
  FACTORY_ENGINE::FactoryType engine_ptr = FACTORY_ENGINE::Produce(engine_name_);
  AICPU_CHECK_NOTNULL(engine_ptr)
  AicpuOpsKernelInfoStorePtr aicpu_ops_kernel_info_store_ptr =
      engine_ptr->GetAicpuOpsKernelInfoStore();
  AICPU_CHECK_NOTNULL(aicpu_ops_kernel_info_store_ptr)
  aicpu_ops_kernel_info_store_ptr->GetAllOpsFullKernelInfo(all_op_info);
  return ge::SUCCESS;
}

bool AicpuGraphOptimizer::ReadOpsParallelRuleFromJsonFile()
{
  string realConfigFilePath = GetSoPath(reinterpret_cast<void *>(&AicpuGraphOptimizer::Initialize));
  AICPUE_LOGD("realConfigFilePath is %s.", realConfigFilePath.c_str());
  string opsParallelRuleFilePath = realConfigFilePath + kAiCpuOpsParallelRuleFileRelativePath;
  AICPUE_LOGD("opsParallelRuleFilePath is %s.", opsParallelRuleFilePath.c_str());
  return OpsParallelRuleJsonFile::Instance().ParseUnderPath(opsParallelRuleFilePath,
                                                            ops_parallel_rule_json_file_).state == ge::SUCCESS;
}

ge::Status AicpuGraphOptimizer::GetOpsParallelRule()
{
  if (!ReadOpsParallelRuleFromJsonFile()) {
    AICPU_REPORT_CALL_ERROR("Call Aicpu ops_parallel_rule_file_path to read ops paralle info from json file failed.");
    return LOAD_CONFIG_JSON_FILE_FAILED;
  }

  AICPU_IF_BOOL_EXEC(
    (ops_parallel_rule_json_file_.find(kOpsParallelRule) == ops_parallel_rule_json_file_.end()),
    AICPUE_LOGW("Json file does not have ops parallel rule infos") return SUCCESS)

  try {
    RuleInfoDesc ops_parallel_info = ops_parallel_rule_json_file_;
    AICPUE_LOGI("Read json file, support parallel ops size is:%lu.",
        ops_parallel_info.rule_info.ops_list.size());
    return FillOpsParallelRuleInfos(ops_parallel_info);
  } catch (const nlohmann::json::exception &e) {
    AICPU_REPORT_INNER_ERROR("Parse ops parallel rule json file[%s] failed, %s.",
        ops_parallel_rule_json_file_.dump().c_str(), e.what());
      return LOAD_CONFIG_JSON_FILE_FAILED;
  }
}

ge::Status AicpuGraphOptimizer::FillOpsParallelRuleInfos(RuleInfoDesc &ops_parallel_info)
{
  if (ops_parallel_info.rule_name.size() == 0) {
    AICPUE_LOGW("Json file does not have ops parallel rule info.");
    return ge::SUCCESS;
  }

  for (const auto &op : ops_parallel_info.rule_info.ops_list) {
    ops_parallel_rule_infos_.push_back(op);
    AICPUE_LOGD("Insert a op[%s] success.", op.c_str());
  }
  AICPUE_LOGD("Read json file, rule name[%s].", ops_parallel_info.rule_name.c_str());
  return ge::SUCCESS;
}

ge::Status AicpuGraphOptimizer::GetOpsParallelInfo(std::unordered_set<string> &ops_parallel_info) const
{
  if (ops_parallel_rule_infos_.size() == 0) {
    AICPUE_LOGW("ops parallel rule infos size is 0.");
    return ge::FAILED;
  }
  for (const auto &op : ops_parallel_rule_infos_) {
    ops_parallel_info.insert(op);
    AICPUE_LOGD("Insert a op[%s] in set success.", op.c_str());
    if (ops_parallel_info.size() >= kMaxOpsParallelNum) {
      AICPUE_LOGW("ops parallel rule support the max num is %zu.", ops_parallel_info.size());
      break;
    }
  }
  return ge::SUCCESS;
}

ge::Status AicpuGraphOptimizer::SetStreamLabel(const ge::NodePtr &node, const std::string &label) const
{
  AICPU_CHECK_NOTNULL(node);
  const OpDescPtr tmp_desc = node->GetOpDesc();
  AICPU_CHECK_NOTNULL(tmp_desc);

  if (!AttrUtils::SetStr(tmp_desc, "_stream_label", label)) {
    REPORT_INNER_ERROR("E19999", "Set Attr:fail for op:%s(%s)",
        node->GetName().c_str(), node->GetType().c_str());
    AICPUE_LOGE("[Set][Attr] fail for op:%s(%s)", node->GetName().c_str(),
        node->GetType().c_str());
    return ge::FAILED;
  }

  return ge::SUCCESS;
}

ge::Status AicpuGraphOptimizer::SetStreamLabelForOpsParallel(ge::ComputeGraph &graph) const
{
  unordered_set<string> ops_parallel_info;
  AICPU_CHECK_RES(GetOpsParallelInfo(ops_parallel_info));
  for (ge::NodePtr &node : graph.GetDirectNode()) {
    AICPU_CHECK_NOTNULL(node);
    ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
    AICPU_CHECK_NOTNULL(op_desc_ptr);
    string op_type = op_desc_ptr->GetType();

    auto iter = ops_parallel_info.find(op_type);
    if (iter == ops_parallel_info.end()) {
      AICPUE_LOGD("Current op type [%s]. Don't exist in ops parallel rule list.", op_type.c_str());
      continue;
    }

    const string lable = node->GetName();
    auto status = SetStreamLabel(node, lable);
    if (status != ge::SUCCESS) {
      AICPUE_LOGW("[Set][Streamlabel] %s to op:%s(%s) failed.", lable.c_str(), node->GetName().c_str(),
          op_type.c_str());
      return ge::FAILED;
    }
    AICPUE_LOGD("[Set][Streamlabel] %s to op:%s(%s) success.", lable.c_str(), node->GetName().c_str(),
        op_type.c_str());
  }
  return ge::SUCCESS;
}
}  // namespace aicpu
