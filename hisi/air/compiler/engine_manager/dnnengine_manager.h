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

#ifndef GE_ENGINE_MANAGER_DNNENGINE_MANAGER_H_
#define GE_ENGINE_MANAGER_DNNENGINE_MANAGER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <mutex>

#include "nlohmann/json.hpp"

#include "common/plugin/plugin_manager.h"
#include "framework/common/ge_inner_error_codes.h"
#include "framework/engine/dnnengine.h"
#include "graph/op_desc.h"
#include "graph/node.h"
#include "opskernel_manager/ops_kernel_manager.h"

using JsonHandle = void *;
namespace ge {
using nlohmann::json;

// Engine configuration
struct EngineConf {
  std::string id;                       // engine ID
  std::string name;                     // engine name
  bool independent{false};         // independent stream
  bool attach{false};              // attach stream
  bool skip_assign_stream{false};  // not assign stream
  std::string scheduler_id;             // scheduler ID
};
using EngineConfPtr = std::shared_ptr<EngineConf>;

// Configuration information of schedule unit
struct SchedulerConf {
  std::string id;                               // scheduler ID
  std::string name;                             // scheduler name
  std::string ex_attrs;                         // extra information
  std::map<std::string, EngineConfPtr> cal_engines;  // engine information
};

using DNNEnginePtr = std::shared_ptr<DNNEngine>;

class DNNEngineManager {
 public:
  friend class GELib;
  static DNNEngineManager &GetInstance();
  std::shared_ptr<ge::DNNEngine> GetEngine(const std::string &name) const;
  const std::map<std::string, DNNEnginePtr> &GetAllEngines() const { return engines_map_; }
  bool IsEngineRegistered(const std::string &name);
  // If can't find appropriate engine name, return "", report error
  std::string GetDNNEngineName(const ge::NodePtr &node_ptr);
  std::string GetCompositeEngineName(const ge::NodePtr &node_ptr, uint32_t recursive_depth = 1);
  std::string GetCompositeEngineName(const std::string &atomic_engine_name);
  std::string GetCompositeEngineKernelLibName(const std::string &composite_engine_name) const;
  const std::map<std::string, SchedulerConf> &GetSchedulers() const;
  const std::map<std::string, uint64_t> &GetCheckSupportCost() const;
  void InitPerformanceStatistic();
  bool IsNoTask(const NodePtr &node);
  bool IsStreamAssignSkip(const NodePtr &node);
  bool IsStreamAssignSkip(const std::string &engine_name);

 private:
  DNNEngineManager();
  ~DNNEngineManager();
  Status Initialize(const std::map<std::string, std::string> &options);
  Status Finalize();
  Status ReadJsonFile(const std::string &file_path, JsonHandle handle);
  Status ParserJsonFile();
  Status ParserEngineMessage(const json engines_json, const std::string &scheduler_mark,
                             std::map<std::string, EngineConfPtr> &engines) const;
  Status CheckJsonFile() const;
  std::string GetHostCpuEngineName(const std::vector<OpInfo> &op_infos, const OpDescPtr &op_desc) const;
  void GetOpInfos(std::vector<OpInfo> &op_infos, const OpDescPtr &op_desc, 
		  bool &is_op_specified_engine) const;

  void InitAtomicCompositeMapping();
  std::string GetCompositeEngine(const NodePtr &node);
  std::string GetCompositeEngine(const NodePtr &func_node, uint32_t recursive_depth);
  std::string GetCompositeEngine(const ComputeGraphPtr &subgraph, uint32_t recursive_depth);

  PluginManager plugin_mgr_;
  std::map<std::string, DNNEnginePtr> engines_map_;
  std::map<std::string, ge::DNNEngineAttribute> engines_attrs_map_;
  std::map<std::string, SchedulerConf> schedulers_;
  std::map<std::string, uint64_t> checksupport_cost_;
  // {atomic_engine, composite_engine}
  std::map<std::string, std::string> atomic_2_composite_{};
  bool init_flag_;
  mutable std::mutex mutex_;
};
}  // namespace ge

#endif  // GE_ENGINE_MANAGER_DNNENGINE_MANAGER_H_
