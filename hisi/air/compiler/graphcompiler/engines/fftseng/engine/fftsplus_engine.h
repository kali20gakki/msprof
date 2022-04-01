/**
 * Copyright 2022-2023 Huawei Technologies Co., Ltd
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

#ifndef FFTS_ENGINE_ENGINE_FFTSPLUS_ENGINE_H_
#define FFTS_ENGINE_ENGINE_FFTSPLUS_ENGINE_H_

#include "common/opskernel/ops_kernel_info_store.h"
#include "common/optimizer/graph_optimizer.h"
#include "inc/ffts_error_codes.h"

using OpsKernelInfoStorePtr = std::shared_ptr<ge::OpsKernelInfoStore>;
using GraphOptimizerPtr = std::shared_ptr<ge::GraphOptimizer>;

extern "C" {
/*
 * to invoke the Initialize function of FusionManager
 * param[in] the options of init
 * return Status(SUCCESS/FAILED)
 */
ffts::Status Initialize(const std::map<string, string> &options);
/*
 * to invoke the Finalize function to release the source of fusion manager
 * return Status(SUCCESS/FAILED)
 */
ffts::Status Finalize();
/*
 * to invoke the same-name function of FusionManager to get the information of OpsKernel InfoStores
 * param[out] the map of OpsKernel InfoStores
 */
void GetOpsKernelInfoStores(const std::map<string, OpsKernelInfoStorePtr> &op_kern_infos);

void GetCompositeEngines(std::map<string, std::set<std::string>> &compound_engine_contains,
                         std::map<std::string, std::string> &compound_engine_2_kernel_lib_name);
/*
 * to invoke the same-name function of FusionManager to get the information of Graph Optimizer
 * param[out] the map of Graph Optimizer
 */
void GetGraphOptimizerObjs(std::map<string, GraphOptimizerPtr> &graph_optimizers);
}
#endif  // FFTS_ENGINE_ENGINE_FFTSPLUS_ENGINE_H_
