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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_FIXPIPE_COMMON_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_FIXPIPE_COMMON_H_
#include <map>
#include <queue>
#include <stack>
#include <string>
#include <unordered_set>
#include <vector>
#include "common/fe_log.h"
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
#include "common/util/platform_info.h"
namespace fe {
using std::queue;
using std::set;
using std::unordered_set;
extern const std::string FIXPIPE_PASS_NAME;
extern const std::string PATTERN_CONV;
extern const std::string SOCINFO;
extern const std::string FIXPIPEINFO;
extern const std::string CONV2D_BACKPROPINPUTD;
extern const std::string CONV2D_BACKPROPINPUT;
extern const std::string DECONVOLUTION;
extern const std::string CONV2D_TRANSPOSED;
extern const std::string CONV2D_TRANSPOSE;
extern const std::string CONV2D_BACKPROPFILTERD;
extern const std::string CONV3D;
extern const std::string CONV3D_BACKPROPINPUTD;
extern const std::string CONV3D_TRANSPOSED;
extern const std::string CONV3D_BACKPROPINPUT;
extern const std::string CONV3D_TRANSPOSE;
extern const std::string CONV3D_BACKPROPFILTERD;
extern const std::string DEPTHWISE_CONV2D;
extern const std::string MATMUL;
extern const std::string MATMULV2;
extern const std::string BATCH_MATMUL;
extern const std::string BATCH_MATMULV2;
extern const std::string FULLYCONNECTION;
extern const std::string DEPTHWISECONV2DBACKPROPFILTERD;
extern const std::string DEPTHWISECONV2DBACKPROPINPUTD;
extern const std::string ASCENDREQUANT;
extern const std::string ASCENDREQUANTS16;
extern const std::string DEQUANTIZE;
extern const std::string ASCENDDEQUANT;
extern const std::string ASCENDDEQUANT16;
extern const std::string ASCENDQUANT;
extern const std::string QUANTIZE;
extern const std::string ASCENDANTIQUANT;
extern const std::string ADD;
extern const std::string ADDV2;
extern const std::string ADDN;
extern const std::string SUB;
extern const std::string RELUGRAD;
extern const std::string RELUGRADV2;
extern const std::string PRELUGRAD;
extern const std::string RELUV2;
extern const std::string RELU6D;
extern const std::string LEAKYRELU;
extern const std::string LEAKYRELUGRAD;
extern const std::string PRELU;
extern const std::string FIXPIPE;
extern const std::string FUSIONOPLIST;
extern const std::string ACTIVATIEUNIT;
extern const std::string ELTWISE_MODE;
extern const std::string CUBEUNIT;
extern const std::string PRECONVUNIT;
extern const std::string PREACTUNIT;
extern const std::string POSTELTWISEUNIT;
extern const std::string POSTACTUNIT;
extern const std::string POSTQUANTUNIT;
extern const std::string POSTTRANSFORMUNIT;
extern const std::string X1INPUTNAME;
extern const std::string X2INPUTNAME;
extern const std::string QUANTSCALE0INPUT;
extern const std::string RELUWEIGHT0INPUT;
extern const std::string CLIPVALUE0INPUT;
extern const std::string QUANTSCALE1INPUT;
extern const std::string RELUWEIGHT1INPUT;
extern const std::string CLIPVALUE1INPUT;
extern const std::string ANTIQUANTSACALEINPUT;
extern const std::string ANTIQUANTOFFSETINPUT;
extern const std::string ATTR_SCALA;
extern const std::string ATTR_ELTWISEMODE;
extern const std::vector<std::string> CUBE_OP_TYPE_VEC;
extern const std::map<std::string, std::string> INSTRUCNAME2FIXPIPENAME;
extern const bool ATTRTRUE;
constexpr float relu6_value = 6.0;
struct Configtype2Datatype {
  bool has_output_dtype;
  ge::DataType input_dtype;
  ge::DataType output_dtype;
};
using CONFIGDTYPE = Configtype2Datatype;

class FixPipeUnit {
 public:
  FixPipeUnit(const std::string &m_unitname, const std::map<std::string, std::vector<CONFIGDTYPE>> &m_optypes)
      : unitname_(m_unitname), opnodes_(m_optypes) {}
  const std::map<std::string, std::vector<CONFIGDTYPE>> &GetNode() const { return opnodes_; }
  const std::string &GetName() const { return unitname_; }
  const std::vector<std::string> &GetDependsUnits() const { return dependunits_; }
  void SetDependUnits(const std::vector<std::string> &depenunits) {
    dependunits_.assign(depenunits.begin(), depenunits.end());
  }
  void SetDependUnitsIndex(const std::vector<uint32_t> &index) { dependunitsindex_.assign(index.begin(), index.end()); }
  const std::vector<uint32_t> &GetDependsUnitsIndex() const { return dependunitsindex_; }
  ~FixPipeUnit();
 private:
  std::string unitname_;
  std::vector<std::string> dependunits_;
  std::vector<uint32_t> dependunitsindex_;
  std::map<std::string, std::vector<CONFIGDTYPE>> opnodes_;
};

class FixPipeNodeInfo {
 public:
  explicit FixPipeNodeInfo(ge::NodePtr node)
      : op_kernel_(node),
        belong_unit_type_(""),
        nodeInfo_fixpipeability_(0),
        isheadnode_(false),
        belong_unit_index_(0) {}
  ge::NodePtr GetNode() const { return op_kernel_; }
  const std::string &GetBelongUnitType() const { return belong_unit_type_; }
  char GetNodeFixpipeability() const { return nodeInfo_fixpipeability_; }
  bool GetIsHeadNode() const { return isheadnode_; }
  void SetBelongUnitType(const std::string &belong_unit_type) { belong_unit_type_ = belong_unit_type; }
  void SetNodeFixpipeability(char nodeInfo_fixpipeability) { nodeInfo_fixpipeability_ = nodeInfo_fixpipeability; }
  void SetIsHeadNode(bool isheadnode) { isheadnode_ = isheadnode; }
  void SetBelongUnitIndex(uint32_t belong_unit_index) { belong_unit_index_ = belong_unit_index; }
  uint32_t GetBelongUnitIndex() const { return belong_unit_index_; }
  bool operator==(const ge::NodePtr input_node) const {
    return (*(this->GetNode()) == *input_node);
  }
  ~FixPipeNodeInfo();

 private:
  ge::NodePtr op_kernel_;
  std::string belong_unit_type_;
  char nodeInfo_fixpipeability_;
  bool isheadnode_;
  uint32_t belong_unit_index_;
  std::vector<ge::NodePtr> node_tofixpipelist_;
};

struct FixPipePassInfoData {
  std::vector<FixPipeNodeInfo> m_opnodes;
  uint32_t pass_index;
  uint32_t m_flag;
  uint32_t unit_index;
};
using FixPipePassInfo = FixPipePassInfoData;

class FixPipeNodePair {
 public:
  FixPipeNodePair(ge::NodePtr first, ge::NodePtr second) : parent_(first), child_(second) {}
  ge::NodePtr GetParent() const { return parent_; }
  ge::NodePtr GetChild() const { return child_; }
  ~FixPipeNodePair() {}

 private:
  ge::NodePtr parent_;
  ge::NodePtr child_;
};

class FixPipeFunctionParam {
 public:
  FixPipeFunctionParam(const std::string &inputname, ge::NodePtr &fixpipenode, const uint32_t &input_index)
      : inputname_(inputname),
        fixpipenode_(fixpipenode),
        inputindex_(input_index),
        firstnodeindex_(0),
        secondnodeindex_(0),
        datatype_(ge::DT_UNDEFINED),
        srcconstnode_(nullptr) {}
  void SetInputName(const std::string &inputname) { inputname_ = inputname; }
  void SetFixPipeNode(ge::NodePtr &fixpipenode) { fixpipenode_ = fixpipenode; }
  void SetInputindex(uint32_t input_index) { inputindex_ = input_index; }
  const std::string &GetInputName() const { return inputname_; }
  ge::NodePtr GetFixpipeNode() const { return fixpipenode_; }
  uint32_t GetParaIndex() const { return inputindex_; }
  void SetFirstIndex(uint32_t firstnodeindex) { firstnodeindex_ = firstnodeindex; }
  uint32_t GetFirstIndex() const { return firstnodeindex_; }
  void SetSecondIndex(uint32_t secondnodeindex) { secondnodeindex_ = secondnodeindex; }
  uint32_t GetSecondIndex() const { return secondnodeindex_; }
  void SetDataType(const ge::DataType datatype) { datatype_ = datatype; }
  ge::DataType GetDataType() const { return datatype_; }
  void SetSrcConstNode(ge::NodePtr &srcconstnode) { srcconstnode_ = srcconstnode; }
  ge::NodePtr GetSrcConstNode() const { return srcconstnode_; }
  ~FixPipeFunctionParam() {}

 private:
  std::string inputname_;
  ge::NodePtr fixpipenode_;
  uint32_t inputindex_;
  uint32_t firstnodeindex_;
  uint32_t secondnodeindex_;
  ge::DataType datatype_;
  ge::NodePtr srcconstnode_;
};

using FixPipeFunctionParamPtr = std::shared_ptr<FixPipeFunctionParam>;
extern const std::vector<std::string> Split(std::string &str, const std::string &pattern);
extern size_t CalDSize(const ge::DataType &data_type);
extern bool ReadPlatFormConfig(const std::string &soc_version, std::vector<std::string> &unit_list,
                               std::map<std::string, std::vector<std::string>> &depends_list,
                               std::map<std::string, std::map<std::string, std::vector<CONFIGDTYPE>>> &fixpipe_map);
extern bool RemoveAnchor(const ge::AnchorPtr &outputanchor, const ge::AnchorPtr &inputanchor);
extern bool HasEdge(const ge::AnchorPtr &outputanchor, const ge::AnchorPtr &inputanchor);
extern bool AddEdges(const ge::AnchorPtr &outputanchor, const ge::AnchorPtr &inputanchor, const ge::NodePtr output,
                     const ge::NodePtr input);
extern Status HasInput1Node(const ge::NodePtr &vectornode);
extern Status HasInputNode(const ge::NodePtr &vectornode, const uint32_t &input_index);
extern ge::NodePtr GetConstNode(const ge::NodePtr &vectornode, uint32_t &depth);
extern ge::NodePtr HasConstNode(const ge::NodePtr &vectornode);
extern bool JudegedataUInt64(uint8_t *data, const size_t &data_size);
extern bool JudgedataFp16(uint8_t *data, const size_t &data_size);
template <typename T>
bool JudgedataImpl(uint8_t *data, const size_t &data_size);
extern bool Judgedata(uint8_t *data, const size_t &data_size, const ge::DataType &data_type);
extern bool JudgeConstValue(const ge::NodePtr &vectornode);
extern bool CheckIsInVector(const std::vector<FixPipeNodeInfo> &m_opnodes, const uint32_t &index = 0);
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_FIXPIPE_COMMON_H_
