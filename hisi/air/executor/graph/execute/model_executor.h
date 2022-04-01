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
#ifndef GE_GRAPH_EXECUTE_MODEL_EXECUTOR_H
#define GE_GRAPH_EXECUTE_MODEL_EXECUTOR_H

#include <thread>

#include "common/executor.h"
#include "graph/execute/graph_execute.h"
#include "graph/passes/mds_kernels/mds_utils.h"

namespace ge {
class ModelExecutor : public Executor {
 public:
  ///
  /// @ingroup ge
  /// @brief graph executor init
  /// @param [in] options user config params
  /// @return Status result of function
  ///
  Status Initialize(const std::map<std::string, std::string> &options, const uint64_t session_id);

  ///
  /// @ingroup ge
  /// @brief graph executor finalize
  /// @return Status result of function
  ///
  Status Finalize();

  ///
  /// @ingroup ge
  /// @brief Load mode for graph.
  /// @param [in] FlowModel: root model of graph compiled.
  /// @param [in] GraphNode: node of graph.
  /// @return Status result of function
  ///
  Status LoadGraph(const FlowModelPtr &ge_root_model, const GraphNodePtr &graph_node) override;

  ///
  /// @ingroup ge
  /// @brief Unload mode for graph.
  /// @param [in] FlowModel: root model of graph compiled.
  /// @param [in] graph_id: graph identifier.
  /// @return Status result of function
  ///
  Status UnloadGraph(const FlowModelPtr &ge_root_model, const uint32_t graph_id) override;

  ///
  /// @ingroup ge
  /// @brief Push model execution params to queue.
  /// @param [in] RunArgs of for model execution.
  /// @return Status result of function
  ///
  Status PushRunArgs(const RunArgs &args) override;

  ///
  /// @ingroup ge
  /// @brief Run graph for synchronize model.
  /// @param [in] graph_node: node of graph.
  /// @param [in] graph_id: graph identifier.
  /// @param [in] inputs: input data for the graph running.
  /// @param [out] outputs: output data of the graph running
  /// @return Status result of function
  ///
  Status RunGraph(const GraphNodePtr &graph_node, const GraphId graph_id,
                  const std::vector<GeTensor> &inputs, std::vector<GeTensor> &outputs) override;

  ///
  /// @ingroup ge
  /// @brief Run graph for NN synchronize model.
  /// @param [in] graph_node: node of graph.
  /// @param [in] graph_id: graph identifier.
  /// @param [in] stream: Stream for model running.
  /// @param [in] inputs: input data for the graph running.
  /// @param [out] outputs: output data of the graph running
  /// @return Status result of function
  ///
  Status RunGraphWithStream(const GraphNodePtr &graph_node, const GraphId graph_id, const rtStream_t stream,
                            const std::vector<GeTensor> &inputs, const std::vector<GeTensor> &outputs) override;

 private:
  static bool ParseTrainGraphFlag();
  static Status GetTotalMemorySize(size_t &total_mem_size);

  void AddGraphNode(const GraphId graph_id, const GraphNodePtr &graph_node);
  void RemoveGraphNode(const GraphId graph_id);

  Status FlowModelLoad(const FlowModelPtr &flow_model, const GraphNodePtr &graph_node);

  Status ModelLoad(const FlowModelPtr &flow_model, const GraphNodePtr &graph_node);

  static Status UnloadModel(const FlowModelPtr &flow_model, const uint32_t graph_id);

  static void ReleaseMemory(const GeModelPtr &ge_model, const GraphNodePtr &graph_node,
                            const std::vector<uint32_t> &model_ids, const uint32_t graph_id, uint64_t session_id);
  Status CheckAndReleaseMemory(const GeModelPtr &ge_model, const GraphNodePtr &graph_node);
  static Status GetMemoryInfo(size_t &free);

  Status CheckFreeMemory(const GeModelPtr &ge_model, const GraphNodePtr &graph_node,
                                bool &is_enough, bool &release_all);
  Status GetMemorySizeAfterReuse(const GeModelPtr &ge_model, const GraphNodePtr &graph_node,
                                        int64_t &sum_size, bool &reuse);

  void RunThread();
  void StopQueue();
  void ReturnError(const RunAsyncCallback callback, const Status ret, const std::string &log_info);

  bool init_flag_{false};
  bool train_graph_flag_{false};
  uint64_t session_id_{0U};
  GraphExecutor graph_executor_;

  std::mutex mutex_;
  std::map<GraphId, GraphNodePtr> graph_nodes_;

  std::thread run_thread_;
  std::atomic_bool thread_run_flag_{false};
  BlockingQueue<RunArgs> run_args_q_;
};
}
#endif // GE_GRAPH_EXECUTE_MODEL_EXECUTOR_H