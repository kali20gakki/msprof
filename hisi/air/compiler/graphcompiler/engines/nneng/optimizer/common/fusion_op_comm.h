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

#ifndef FUSION_ENGINE_OPTIMIZER_COMMON_FUSION_OP_COMM_H_
#define FUSION_ENGINE_OPTIMIZER_COMMON_FUSION_OP_COMM_H_

#include <map>
#include <string>
#include <vector>
#include "common/fe_inner_attr_define.h"
#include "common/fe_log.h"
#include "graph/compute_graph.h"

namespace fe {
class FusionOpComm {
 public:
  FusionOpComm(){};
  virtual ~FusionOpComm(){};
  FusionOpComm(const FusionOpComm &in) = delete;
  FusionOpComm &operator=(const FusionOpComm &in) = delete;

  ge::OpDescPtr CreateFusionOp(std::vector<ge::NodePtr> &fus_nodelist, const string &engine_name);

 private:
  Status CopyFusionScopeAttr(const ge::OpDescPtr &src_op_desc, ge::OpDescPtr &dst_op_desc) const;
  ge::OpDescPtr SetMultiKernelTBEFusionOp(const vector<ge::NodePtr> &fus_nodelist, ge::OpDescPtr &fus_opdef,
                                  const string &engine_name);
  ge::OpDescPtr SetTBEFusionOp(const vector<ge::NodePtr> &fus_nodelist, ge::OpDescPtr &fus_opdef,
                               const string &engine_name);
  Status AddTvmNode(ge::OpDescPtr op_desc) const;

  void SetDataDumpAttrToFusionOp(const vector<ge::NodePtr> &fus_nodelist, ge::OpDescPtr &fus_opdef) const;
  void SetOriginalNodesOpDescToFusionOp(const vector<ge::NodePtr> &fus_nodelist, const ge::OpDescPtr &fus_opdef) const;
  void RecordOriginalNames(std::vector<ge::NodePtr> original_nodes, ge::OpDescPtr &op_desc) const;
};

}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_COMMON_FUSION_OP_COMM_H_
