/**
 * Copyright 2021-2022 Huawei Technologies Co., Ltd
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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_FIXPIPE_ADDINPUTSTRATEGY_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_FIXPIPE_ADDINPUTSTRATEGY_H_

#include <map>
#include <queue>
#include <string>
#include <unordered_set>
#include <vector>

#include "common/fe_log.h"
#include "fixpipe_common.h"
#include "graph/anchor.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/node.h"
#include "graph/op_desc.h"
#include "graph/range_vistor.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/fusion_common/pattern_fusion_base_pass.h"

namespace fe {
using std::queue;
using std::set;
using std::unordered_set;

class FixPipeAddInputBase;
using FixPipeAddInputPtr = std::shared_ptr<FixPipeAddInputBase>;

class FixPipeAddInputBase {
 public:
  FixPipeAddInputBase(){};
  virtual Status DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                            const FixPipeFunctionParamPtr &functpara, std::vector<ge::NodePtr> &new_nodes) const {
    return SUCCESS;
  }
  template <typename T>
  Status UpdateSalarInput(ge::GeTensorDescPtr tensor_desc, T value, ge::GeTensorPtr tensornode,
                          const ge::DataType &data_type) const;
  template <typename T>
  Status CreateAndUpdateSalarInput(ge::ComputeGraph &graph, const FixPipeFunctionParamPtr &functpara, T value,
                                   const ge::DataType &data_type, std::vector<ge::NodePtr> &new_nodes) const;
  void SetClipValue6(ge::ComputeGraph &graph, const FixPipeFunctionParamPtr &functpara,
                     ge::DataType dst_datatype, std::vector<ge::NodePtr> &new_nodes) const;
  Status CloneVectorInput(const FixPipeNodeInfo &tofuzednode,
                          const FixPipeFunctionParamPtr &functpara) const;
  Status CreateAndUpdateVectorMulsInput(ge::ComputeGraph &graph, const FixPipeFunctionParamPtr &functpara,
                                        const FixPipeNodeInfo &postfuzednode, const FixPipeNodeInfo &tofuzednode,
                                        std::vector<ge::NodePtr> &new_nodes) const;
  Status CreateNewDataNodeDirect(ge::ComputeGraph &graph, ge::GeTensorDescPtr tensor_desc, ge::GeTensorPtr tensornode,
                                 const FixPipeFunctionParamPtr &functpara, std::vector<ge::NodePtr> &new_nodes) const;
  ge::NodePtr CreateNewDataNodeOnly(ge::ComputeGraph &graph, ge::GeTensorDescPtr tensor_desc,
                                    ge::GeTensorPtr tensornode, const std::string &op_name,
                                    std::vector<ge::NodePtr> &new_nodes) const;
  ge::NodePtr GetQuantScaleOffset(const FixPipePassInfo &match_pass,
                                  const uint32_t &index,
                                  float &scale, float &offset) const;
  uint64_t SetM1OfQuant(const float &scale, const float &offset, const ge::NodePtr node) const;
  uint64_t SetM3OfQuant(const float &scale, const float &offset, const ge::NodePtr node) const;
  uint64_t TransM1Scale(const float &src_value) const;
  bool IsScalar(const ge::GeShape &origin_shape) const;
  template <typename T>
  Status CreateAndUpdateVectorMulScalarInput(ge::ComputeGraph &graph, const FixPipeFunctionParamPtr &functpara,
                                             const FixPipeNodeInfo &prefuzednode, const T &value,
                                             std::vector<ge::NodePtr> &new_nodes) const;
  virtual ~FixPipeAddInputBase() {}
};

class FixPipeAddInputStrategy21 : public FixPipeAddInputBase {
 public:
  Status DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                    const FixPipeFunctionParamPtr &functpara,
                    std::vector<ge::NodePtr> &new_nodes) const override;
};

class FixPipeAddInputStrategy22 : public FixPipeAddInputBase {
 public:
  Status DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                    const FixPipeFunctionParamPtr &functpara,
                    std::vector<ge::NodePtr> &new_nodes) const override;
};

class FixPipeAddInputStrategy31 : public FixPipeAddInputBase {
 public:
  Status DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                    const FixPipeFunctionParamPtr &functpara,
                    std::vector<ge::NodePtr> &new_nodes) const override;
};

class FixPipeAddInputStrategy32 : public FixPipeAddInputBase {
 public:
  Status DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                    const FixPipeFunctionParamPtr &functpara,
                    std::vector<ge::NodePtr> &new_nodes) const override;
};

class FixPipeAddInputStrategy33 : public FixPipeAddInputBase {
 public:
  Status DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                    const FixPipeFunctionParamPtr &functpara,
                    std::vector<ge::NodePtr> &new_nodes) const override;
};

class FixPipeAddInputStrategy34 : public FixPipeAddInputBase {
 public:
  Status DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                    const FixPipeFunctionParamPtr &functpara,
                    std::vector<ge::NodePtr> &new_nodes) const override;
};

class FixPipeAddInputStrategy35 : public FixPipeAddInputBase {
 public:
  Status DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                    const FixPipeFunctionParamPtr &functpara,
                    std::vector<ge::NodePtr> &new_nodes) const override;
};

class FixPipeAddInputStrategy36 : public FixPipeAddInputBase {
 public:
  Status DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                    const FixPipeFunctionParamPtr &functpara,
                    std::vector<ge::NodePtr> &new_nodes) const override;
};

class FixPipeAddInputStrategy37 : public FixPipeAddInputBase {
 public:
  Status DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                    const FixPipeFunctionParamPtr &functpara,
                    std::vector<ge::NodePtr> &new_nodes) const override;
};

class FixPipeAddInputStrategy38 : public FixPipeAddInputBase {
 public:
  Status DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                    const FixPipeFunctionParamPtr &functpara,
                    std::vector<ge::NodePtr> &new_nodes) const override;
};

class FixPipeAddInputStrategy41 : public FixPipeAddInputBase {
 public:
  Status DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                    const FixPipeFunctionParamPtr &functpara,
                    std::vector<ge::NodePtr> &new_nodes) const override;
};

class FixPipeAddInputStrategy42 : public FixPipeAddInputBase {
 public:
  Status DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                    const FixPipeFunctionParamPtr &functpara,
                    std::vector<ge::NodePtr> &new_nodes) const override;
};

class FixPipeAddInputStrategy43 : public FixPipeAddInputBase {
 public:
  Status DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                    const FixPipeFunctionParamPtr &functpara,
                    std::vector<ge::NodePtr> &new_nodes) const override;
};

class FixPipeAddInputStrategy44 : public FixPipeAddInputBase {
 public:
  Status DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                    const FixPipeFunctionParamPtr &functpara,
                    std::vector<ge::NodePtr> &new_nodes) const override;
};

class FixPipeAddInputStrategy51 : public FixPipeAddInputBase {
 public:
  Status DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                    const FixPipeFunctionParamPtr &functpara,
                    std::vector<ge::NodePtr> &new_nodes) const override;
};

class FixPipeAddInputStrategy61 : public FixPipeAddInputBase {
 public:
  Status DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                    const FixPipeFunctionParamPtr &functpara,
                    std::vector<ge::NodePtr> &new_nodes) const override;
};

class FixPipeAddInputStrategy62 : public FixPipeAddInputBase {
 public:
  Status DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                    const FixPipeFunctionParamPtr &functpara,
                    std::vector<ge::NodePtr> &new_nodes) const override;
};

class FixPipeAddInputStrategy63 : public FixPipeAddInputBase {
 public:
  Status DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                    const FixPipeFunctionParamPtr &functpara,
                    std::vector<ge::NodePtr> &new_nodes) const override;
};

class FixPipeAddInputStrategy64 : public FixPipeAddInputBase {
 public:
  Status DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                    const FixPipeFunctionParamPtr &functpara,
                    std::vector<ge::NodePtr> &new_nodes) const override;
};

class FixPipeAddInputStrategy81 : public FixPipeAddInputBase {
 public:
  Status DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                    const FixPipeFunctionParamPtr &functpara,
                    std::vector<ge::NodePtr> &new_nodes) const override;
};

class FixPipeAddInputStrategy91 : public FixPipeAddInputBase {
 public:
  Status DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                    const FixPipeFunctionParamPtr &functpara,
                    std::vector<ge::NodePtr> &new_nodes) const override;
};

class FixPipeAddInputStrategyDefault : public FixPipeAddInputBase {
 public:
  Status DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                    const FixPipeFunctionParamPtr &functpara,
                    std::vector<ge::NodePtr> &new_nodes) const override;
};

}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_FIXPIPE_ADDINPUTSTRATEGY_H_
