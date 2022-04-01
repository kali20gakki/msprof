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
#ifndef AICPU_TF_ENGINE_H_
#define AICPU_TF_ENGINE_H_

#include "common/engine/base_engine.h"
#include "common/util/constant.h"

namespace aicpu {
using BaseEnginePtr = std::shared_ptr<BaseEngine>;
/**
 * tf engine.
 * Used for the ops belong to tf engine.
 */
class TfEngine : public BaseEngine{
 public:
  /**
   * get TfEngine instance.
   * @return  TfEngine instance.
   */
  static BaseEnginePtr Instance();

  /**
   * Destructor
   */
  virtual ~TfEngine() = default;

  /**
   * When Ge start, GE will invoke this interface
   * @param options option flag map
   * @return The status whether initialize successfully
   */
  ge::Status Initialize(const std::map<std::string, std::string> &options) override;

  /**
   * After the initialize, GE will invoke this interface
   * to get the Ops kernel Store.
   * @param ops_ernel_info_stores The tf ops kernel info
   */
  void GetOpsKernelInfoStores(std::map<std::string, OpsKernelInfoStorePtr> &ops_kernel_info_stores) const override;

  /**
   * After the initialize, GE will invoke this interface
   * to get the Graph Optimizer.
   * @param graph_optimizers The tf Graph Optimizer objs
   */
  void GetGraphOptimizerObjs(std::map<std::string, GraphOptimizerPtr> &graph_optimizers) const override;

  /**
   * After the initialize, GE will invoke this interface
   * to get the Kernel Builders.
   * @param ops_kernel_builders The tf ops kernel builder objs
   */
  void GetOpsKernelBuilderObjs(std::map<std::string, OpsKernelBuilderPtr> &ops_kernel_builders) const override;

  /**
   * When the graph finished, GE will invoke this interface
   * @return The status whether initialize successfully
   */
  ge::Status Finalize() override;

  // Copy prohibited
  TfEngine(const TfEngine &tf_engine) = delete;

  // Move prohibited
  TfEngine(const TfEngine &&tf_engine) = delete;

  // Copy prohibited
  TfEngine &operator=(const TfEngine &tf_engine) = delete;

  // Move prohibited
  TfEngine &operator=(TfEngine &&tf_engine) = delete;

 private:
  /**
   * Contructor
   */
  TfEngine() : BaseEngine(kTfEngine) {}

 private:
  static BaseEnginePtr instance_;
};
}  // namespace aicpu
#endif
