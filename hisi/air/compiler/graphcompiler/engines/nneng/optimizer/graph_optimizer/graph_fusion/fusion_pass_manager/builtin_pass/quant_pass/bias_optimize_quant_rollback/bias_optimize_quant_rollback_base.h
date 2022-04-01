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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_QUANT_PASS_BIAS_OPTIMIZE_QUANT_ROLLBACK_BIAS_OPTIMIZE_QUANT_ROLLBACK_BASE_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_QUANT_PASS_BIAS_OPTIMIZE_QUANT_ROLLBACK_BIAS_OPTIMIZE_QUANT_ROLLBACK_BASE_H_

#include <map>
#include <string>
#include <vector>
#include "common/configuration.h"
#include "common/fe_log.h"
#include "common/math_util.h"
#include "common/op_info_common.h"
#include "common/unknown_shape_util.h"
#include "external/graph/types.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/fusion_common/pattern_fusion_base_pass.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/quant_host_cpu_op_common.h"

namespace fe {
const string PATTERN_QUANT = "ascendquant";
const string PATTERN_CUBE = "cube";
const string PATTERN_DEQUANT = "ascenddequant";
const string PATTERN_PAD = "pad";
const string PATTERN_WEIGHT = "weight";
const string PATTERN_BIAS = "bias";
const string PATTERN_SCALE = "deqScale";
const string PATTERN_OUTPUT = "output";
const string QUANT = "AscendQuant";
const string DEQUANT = "AscendDequant";
const string PAD = "Pad";
const string ENTER = "Enter";
const string CONST = "Const";

const int32_t MIN_BIAS_OPTIMIZE_CHANNEL = 16;
const size_t INPUT_SIZE_CONTAINS_BIAS = 3;
const uint32_t DEQUANT_SCALE_INDEX = 1;

const std::vector<std::vector<std::string>> TENSOR_NAME_OF_HOST_CPU_OP = {
    {CUBE_FILTER}, {CUBE_BIAS}, {CUBE_OPTIMIZATION_BIAS, CUBE_OPTIMIZATION_FILTER}};

const std::map<ISAArchVersion, std::string> ISA_ARCH_VERSION_STRING_MAP {
        {ISAArchVersion::EN_ISA_ARCH_V100, "v100"},
        {ISAArchVersion::EN_ISA_ARCH_V200, "v200"},
        {ISAArchVersion::EN_ISA_ARCH_V210, "v210"},
        {ISAArchVersion::EN_ISA_ARCH_V300, "v300"}
};

// the first index is the main tensor. 1: filter, 2: bias
const std::vector<std::vector<uint32_t>> ORIGINAL_CONV_ANCHOR_INDEX = {{1}, {2}, {2, 1}};

enum class HostOpMode { WEIGHT_ROLL_BACK_MODE = 0, BIAS_ROLL_BACK_MODE = 1, BIAS_OPTIMIZATION_MODE = 2, MODE_BOTTOM = 3 };

struct PatternNodes {
  ge::NodePtr cube_node;
  ge::NodePtr dequant_node;
  ge::NodePtr quant_node;
  ge::NodePtr weight_const_node;
  ge::NodePtr ascend_weight_quant_node;
};

enum class QuantProcessMode { BIAS_OPTIMIZE = 0, QUANT_ROLLBACK, QUANT_UNDIFINED };

class BiasOptimizeQuantRollbackBase : public PatternFusionBasePass {
 protected:
  /*
   * Get cube node co dim value
   */
  virtual Status GetCoValue(ge::NodePtr &cube_node, int64_t &co);

  /*
   * some op like deconvolution, depthwiseconv2d and convedtransposed need to set attr cin_cout_reverse
   * cause those ops bias channel is equal to weight's C channel dim value
   */
  virtual void SetCinCoutReverse(ge::NodePtr &nodePtr);

  /*
   * set cube node bias name
   */
  Status SetBiasName(const std::string &bias_name);

  /*
   * judge quant process mode
   * bias optimize or quant rollback
   */
  virtual Status GetQuantProcessMode(ge::NodePtr &quant_node, ge::NodePtr &cube_node,
                                     QuantProcessMode &quant_process_mode);

  /*
   * judge deq_scale shape info
   * only support 1-D shape
   * soc_version is v100: offset_w should be zero
   */
  Status JudgeDeqscaleShape(const ge::NodePtr &dequant_node) const;

  /*
   * remove Enter node
   */
  Status RemoveEnter(ge::ComputeGraph &graph, const ge::NodePtr node) const;

  /*
   * set offset attr of quant node to cube node
   */
  Status SetQuantParameters(const ge::NodePtr &cube_node, const ge::NodePtr &quant_node) const;

  /*
   * create bias input for cube node
   * value is all zero, case bias optimize will insert host op
   */
  Status CreateBiasInput(ge::ComputeGraph &graph, ge::NodePtr &cube_node, const int64_t &co);

  // judge cube node has bias input or not
  bool IsCubeHasBiasInput(ge::NodePtr cube_node) const;

  /*
   * judge cube node need bias input or not
   * sometimes cube node has no bias input,
   * and it does not need that
   */
  virtual bool IsCubeNeedBiasInput(ge::NodePtr cube_node);

  /*
   * do bias optimize, entry function
   */
  virtual Status BiasOptimize(ge::ComputeGraph &graph, ge::NodePtr &cube_node, ge::NodePtr &dequant_node,
                              ge::NodePtr &quant_node, vector<ge::NodePtr> &fusion_nodes);

  /*
   * after do quant rolback, we need refresh cube node dtype
   */
  Status SetCubeNodeDataType(const ge::NodePtr &cube_node, const ge::DataType &data_type,
                             const ge::DataType &target_data_type) const;

  virtual Status SetDataTypeOfNodes(const ge::NodePtr &cube_node);

  std::string GetIsaArchVersionStr() const;

  /* In this function we will remove the edge between node and its predecessors,
   * so the param peer_out_anchors_of_node is used for recording the predecessors'
   * output data anchor before removing the edge. Because if the edges are
   * removed, we will not able to get the peer out anchor any more.
   */
  Status RemoveInputEdgeAndSingleConstInput(const ge::NodePtr &node,
                                            std::vector<ge::OutDataAnchorPtr> &peer_out_anchors_of_node) const;

  Status LinkOutputEdgeWithoutControl(ge::NodePtr old_node, ge::NodePtr new_node, const string &cube_name,
                                      const std::vector<ge::OutDataAnchorPtr> &peer_out_anchors_of_old_node) const;

  /*
   * create host op function
   * host op type: QuantWeightRollBack, QuantBiasRollBack, QuantBiasOptimization
   */
  Status CreateNewHostCpuOp(const string &op_type, const struct PatternNodes &pattern_node, ge::ComputeGraph &graph,
                            uint32_t mode, /* HostOpMode */
                            vector<ge::NodePtr> &fus_nodes);

  Status LinkHostOpEdge(const struct PatternNodes &pattern_node, ge::NodePtr &host_node,
                        const uint32_t &anchor_index, const size_t &index) const;
  // quant rolback
  // create rollback host op
  Status DoFusion(ge::ComputeGraph &graph, const ge::NodePtr &cube_node, const ge::NodePtr &quant_node,
                  const ge::NodePtr &dequant_node, vector<ge::NodePtr> &fus_nodes);

  /*
   * a transData operator will be inserted before AscendWeightQuant operator to realize HWCN->Fractal_Z
   * group info should be set to AscendWeightQuant which can be obtained by transData accordingly.
   */
  Status SetGroupInfoForAscendWeightQuantNodes(const ge::NodePtr &cube_node) const;

  /*
   * when change quant node edge, if cube node is conv2d,
   * may has pad node before, so we need find node after quant
   * then remove edge between qunat and node after quant
   */
  virtual ge::NodePtr GetCubeNodeInputNode(const ge::NodePtr &cube_node) const;
/*
 * *****************************caffe case**********************************
 *
 *                 bias_const
 *                   \
 *                    v
 * weight_const--->cube_node
 *                    ^
 *                   /
 *              ascendquant
 *
 * *****************************other case**********************************
 *
 *                    offset_const     bias_const
 *                        \               \
 *                         v               v
 * weight_const--->ascendweightquant--->cube_node
 *                                        ^
 *                                       /
 *                                  ascendquant
 *
 * weightnodes include weight_const and ascendweightquant(ascendweightquant is nullptr if caffe case)
 * */
  Status GetWeightNodesOfCubeNode(const ge::NodePtr &cube_node, ge::NodePtr &weight_const_node,
                                  ge::NodePtr &ascend_weight_quant_node) const;

/*
 * *****************************caffe case**********************************
 *
 *                 bias_const
 *                   \
 *                    v
 * weight_const--->cube_node
 *                    ^
 *                   /
 *              ascendquant
 *
 * *****************************other case**********************************
 *
 *                    offset_const     bias_const
 *                        \               \
 *                         v               v
 * weight_const--->ascendweightquant--->cube_node
 *                                        ^
 *                                       /
 *                                  ascendquant
 *
 * after weightRollback, ascendweightquant and offset_const is isolate, delete them(NOTICE: IN CAFFE CASE DONT NEED DO THIS)
 * */
  Status WeightRollbackDeleteIsolateNodes(ge::ComputeGraph &graph, ge::NodePtr &ascend_weight_quant_node) const;

  Status SetInt4OrInt8Flag(const ge::NodePtr &ascend_weight_quant_node,
                           const ge::NodePtr &quant_host_cpu_op_node) const;

  // do quant rollback, remove edge of quant node
  // before remove quant node, we need to judge quant has other out node or not,
  // case we only remove quant node when it has one output node
  Status ChangeQuantNodeEdge(ge::ComputeGraph &graph, const ge::NodePtr &cube_node, ge::NodePtr &quant_node) const;

  // do quant rollback, remove edge of dequant node
  // then remove dequant node
  virtual Status ChangeDequantNodeEdge(ge::ComputeGraph &graph, ge::NodePtr &cube_node, ge::NodePtr &dequant_node);

  /*
   * do quant rollback, entry function
   */
  virtual Status QuantRollback(ge::ComputeGraph &graph, ge::NodePtr &cube_node, ge::NodePtr &dequant_node,
                               ge::NodePtr &quant_node, vector<ge::NodePtr> &fusion_nodes);

  Status ConstructDesc(const size_t &i, const size_t &inputs_size, const uint32_t &weight_anchor_index,
                       const struct PatternNodes &pattern_node, const vector<string> &tensor_desc_name_list,
                       const ge::OpDescPtr &quant_host_cpu_op, ge::GeTensorDesc &output_desc) const;

 protected:
  Status Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &fusion_nodes) override;

 private:
  // different cube node has different bias name
  std::string bias_name_ = "bias";
};

}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_QUANT_PASS_BIAS_OPTIMIZE_QUANT_ROLLBACK_BIAS_OPTIMIZE_QUANT_ROLLBACK_BASE_H_
