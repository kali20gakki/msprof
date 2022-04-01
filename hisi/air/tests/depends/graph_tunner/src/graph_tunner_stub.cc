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

#include "external/ge/ge_api_error_codes.h"

using Status = uint32_t;
#include "ffts_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

namespace tune {
Status FFTSOptimize(ge::ComputeGraph &compute_graph) {
  return compute_graph.GetAllNodes().empty() ? ge::FAILED : ge::SUCCESS;
}

Status FFTSGraphPreThread(ge::ComputeGraph &compute_graph) {
  return compute_graph.GetAllNodes().empty() ? ge::FAILED : ge::SUCCESS;
}

Status FFTSNodeThread(ge::ComputeGraph &compute_graph, const std::string &nodeName) {
  return compute_graph.GetAllNodes().empty() ? ge::FAILED : ge::SUCCESS;
}
} // namespace tune
#ifdef __cplusplus
}
#endif
