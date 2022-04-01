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

#include "fixpipe_common.h"
#include <map>
#include <queue>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_set>
#include <vector>
#include "common/configuration.h"
#include "common/fe_fp16_t.h"
#include "common/fe_log.h"
#include "common/op_info_common.h"
#include "common/util/platform_info.h"
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
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/quant_host_cpu_op_common.h"
namespace fe {
using std::queue;
using std::set;
using std::unordered_set;
const std::string PATTERN_CONV = "conv_pattern";
const std::string SOCINFO = "SoCInfo";
const std::string CONV2D_BACKPROPINPUTD = "Conv2DBackpropInputD";
const std::string CONV2D_BACKPROPINPUT = "Conv2DBackpropInput";
const std::string DECONVOLUTION = "Deconvolution";
const std::string CONV2D_TRANSPOSED = "Conv2DTransposeD";
const std::string CONV2D_TRANSPOSE = "Conv2DTranspose";
const std::string CONV2D_BACKPROPFILTERD = "Conv2DBackpropFilterD";
const std::string MATMUL = "MatMul";
const std::string MATMULV2 = "MatMulV2";
const std::string CONV3D = "Conv3D";
const std::string BATCH_MATMUL = "BatchMatMul";
const std::string BATCH_MATMULV2 = "BatchMatMulV2";
const std::string FULLYCONNECTION = "FullyConnection";
const std::string DEPTHWISECONV2DBACKPROPFILTERD = "DepthwiseConv2DBackpropFilterD";
const std::string DEPTHWISECONV2DBACKPROPINPUTD = "DepthwiseConv2DBackpropInputD";
const std::string CONV3D_BACKPROPINPUTD = "Conv3DBackpropInputD";
const std::string CONV3D_BACKPROPFILTERD = "Conv3DBackpropFilterD";
const std::string CONV3D_TRANSPOSED = "Conv3DTransposeD";
const std::string CONV3D_BACKPROPINPUT = "Conv3DBackpropInput";
const std::string CONV3D_TRANSPOSE = "Conv3DTranspose";
const std::string DEPTHWISE_CONV2D = "DepthwiseConv2D";
const std::string ASCENDREQUANT = "AscendRequant";
const std::string ASCENDREQUANTS16 = "AscendRequantS16";
const std::string DEQUANTIZE = "Dequantize";
const std::string ASCENDDEQUANT = "AscendDequant";
const std::string ASCENDDEQUANT16 = "AscendDequantS16";
const std::string ASCENDQUANT = "AscendQuant";
const std::string QUANTIZE = "Quantize";
const std::string ASCENDANTIQUANT = "AscendAntiQuant";
const std::string ADD = "Add";
const std::string ADDV2 = "AddV2";
const std::string ADDN = "AddN";
const std::string SUB = "Sub";
const std::string RELUGRAD = "ReluGrad";
const std::string RELUGRADV2 = "ReluGradV2";
const std::string PRELUGRAD = "PReluGrad";
const std::string RELUV2 = "ReluV2";
const std::string RELU6D = "Relu6D";
const std::string LEAKYRELU = "LeakyRelu";
const std::string LEAKYRELUGRAD = "LeakyReluGrad";
const std::string PRELU = "PRelu";
const std::string FIXPIPE = "FixPipe";
const std::string FUSIONOPLIST = "fusion_op_list";
const std::string ACTIVATIEUNIT = "unit_list";
const std::string ELTWISE_MODE = "eltwise_mode";
const std::string CUBEUNIT = "CUBE_UNIT";
const std::string PRECONVUNIT = "pre_conv";
const std::string PREACTUNIT = "pre_act";
const std::string POSTELTWISEUNIT = "post_eltwise";
const std::string POSTACTUNIT = "post_act";
const std::string POSTQUANTUNIT = "post_quant";
const std::string POSTTRANSFORMUNIT = "post_transform";
const std::string X1INPUTNAME = "x1";
const std::string X2INPUTNAME = "x2";
const std::string QUANTSCALE0INPUT = "quant_scale_0";
const std::string RELUWEIGHT0INPUT = "relu_weight_0";
const std::string CLIPVALUE0INPUT = "clip_value_0";
const std::string QUANTSCALE1INPUT = "quant_scale_1";
const std::string RELUWEIGHT1INPUT = "relu_weight_1";
const std::string CLIPVALUE1INPUT = "clip_value_1";
const std::string ANTIQUANTSACALEINPUT = "anti_quant_scale";
const std::string ANTIQUANTOFFSETINPUT = "anti_quant_offset";
const std::string FIXPIPEINFO = "Fixpipe";
const std::string ATTR_SCALA = "scalarattr";
const std::string ATTR_ELTWISEMODE = "mode";
static constexpr char const *CASTNEW = "Cast";
static constexpr char const *RELUNEW = "Relu";
static constexpr char const *RELU6NEW = "Relu6";
static constexpr char const *TRANSDATANEW = "TransData";
const std::vector<std::string> CUBE_OP_TYPE_VEC = {CONV2D,
                                                   MATMUL,
                                                   MATMULV2,
                                                   CONV2D_BACKPROPINPUTD,
                                                   DECONVOLUTION,
                                                   CONV2D_TRANSPOSED,
                                                   CONV2D_BACKPROPFILTERD,
                                                   BATCH_MATMUL,
                                                   BATCH_MATMULV2,
                                                   FULLYCONNECTION,
                                                   DEPTHWISECONV2DBACKPROPFILTERD,
                                                   DEPTHWISECONV2DBACKPROPINPUTD,
                                                   DEPTHWISE_CONV2D};
const std::map<std::string, std::string> INSTRUCNAME2FIXPIPENAME{{"requant", ASCENDREQUANT},
                                                                 {"quant", ASCENDQUANT},
                                                                 {"dequant", ASCENDDEQUANT},
                                                                 {"cast", CASTNEW},
                                                                 {"add", ADD},
                                                                 {"nz2nd", TRANSDATANEW},
                                                                 {"anti_sub", ASCENDANTIQUANT},
                                                                 {"anti_add", ASCENDANTIQUANT},
                                                                 {"sub", SUB},
                                                                 {"normal_relu", RELUNEW},
                                                                 {"scalar_relu", LEAKYRELU},
                                                                 {"vector_relu", PRELU},
                                                                 {"clip_relu", RELU6NEW}};
const std::string FIXPIPE_CONFIG_KEY = "Intrinsic_fix_pipe_";
const std::string UNIT_LIST_NAME = "unit_list";
const std::string FUNC_LIST = "_func_list";
const std::string DEPEND_UNITS = "_depend_unit";
const bool ATTRTRUE = true;
const uint32_t MAX_DEPTH = 7;
constexpr uint32_t MAX_CONFIG_MATCHLENG = 5;
constexpr uint32_t TRAN_CONFIG_INDEX = 3;
constexpr uint32_t TRAN_CONFIG_SECONDINDEX = 4;
FixPipeUnit::~FixPipeUnit() {
  dependunits_.clear();
  dependunitsindex_.clear();
  opnodes_.clear();
}
FixPipeNodeInfo::~FixPipeNodeInfo() {
  node_tofixpipelist_.clear();
}

inline ge::DataType TranFerString(const std::string &configstr) {
  if (configstr == "s4") {
    return ge::DT_INT4;
  } else if (configstr == "s8") {
    return ge::DT_INT8;
  } else if (configstr == "s16") {
    return ge::DT_INT16;
  } else if (configstr == "s32") {
    return ge::DT_INT32;
  } else if (configstr == "s64") {
    return ge::DT_INT64;
  } else if (configstr == "u8") {
    return ge::DT_UINT8;
  } else if (configstr == "u16") {
    return ge::DT_UINT16;
  } else if (configstr == "u32") {
    return ge::DT_UINT32;
  } else if (configstr == "u64") {
    return ge::DT_UINT64;
  } else if (configstr == "f16") {
    return ge::DT_FLOAT16;
  } else if (configstr == "bf16") {
    return ge::DT_BF16;
  } else if (configstr == "f32") {
    return ge::DT_FLOAT;
  }
  return ge::DT_UNDEFINED;
}

static CONFIGDTYPE TransFerConfig2Dtype(const std::string &configstr) {
  CONFIGDTYPE ret;
  ret.has_output_dtype = false;
  ret.input_dtype = ge::DT_UNDEFINED;
  ret.output_dtype = ge::DT_UNDEFINED;
  if (configstr.size() >= MAX_CONFIG_MATCHLENG) {
    std::string tmpsubstr = configstr.substr(0, TRAN_CONFIG_INDEX);
    if (tmpsubstr == "f32" || tmpsubstr == "f16" || tmpsubstr == "s32") {
      if (configstr.at(TRAN_CONFIG_INDEX) == '2') {
        std::string secondstr = configstr.substr(TRAN_CONFIG_SECONDINDEX);
        ret.has_output_dtype = true;
        ret.input_dtype = TranFerString(tmpsubstr);
        ret.output_dtype = TranFerString(secondstr);
      } else {
        ret.input_dtype = TranFerString(tmpsubstr);
      }
    }
  } else {
    ret.input_dtype = TranFerString(configstr);
  }
  return ret;
}

bool ReadPlatFormConfig(const std::string &soc_version, std::vector<std::string> &unit_list,
                        std::map<std::string, std::vector<std::string>> &depends_list,
                        std::map<std::string, std::map<std::string, std::vector<CONFIGDTYPE>>> &fixpipe_map) {
  PlatFormInfos platform_infos;
  OptionalInfos opti_compilation_infos;
  if (PlatformInfoManager::Instance().GetPlatformInfos(soc_version, platform_infos, opti_compilation_infos) !=
      SUCCESS) {
    FE_LOGW("Fail to get platform info by soc version[%s].", soc_version.c_str());
    return false;
  }
  // inputmap first key is startwith "Intrinsic_fix_pipe_" second fixpipe config value
  std::map<std::string, std::vector<std::string>> input_map = opti_compilation_infos.GetFixPipeDtypeMap();
  for (auto &iter : input_map[FIXPIPE_CONFIG_KEY + UNIT_LIST_NAME]) {
    std::vector<std::string> &oplist = input_map[FIXPIPE_CONFIG_KEY + iter + FUNC_LIST];
    std::map<std::string, std::vector<CONFIGDTYPE>> outputinnermap;
    for (auto &opname : oplist) {
      std::string outstring;
      auto item = INSTRUCNAME2FIXPIPENAME.find(opname);
      if (item != INSTRUCNAME2FIXPIPENAME.end()) {
        outstring = item->second;
      }
      vector<std::string> &dtypelist = input_map[FIXPIPE_CONFIG_KEY + iter + "_" + opname];
      std::vector<CONFIGDTYPE> outputsinner;
      for (auto &dtype : dtypelist) {
        CONFIGDTYPE dtypestr = TransFerConfig2Dtype(dtype);
        outputsinner.push_back(dtypestr);
      }
      outputinnermap.emplace(make_pair(outstring, outputsinner));
    }
    std::vector<std::string> &depends_units = input_map[FIXPIPE_CONFIG_KEY + iter + DEPEND_UNITS];
    depends_list.emplace(make_pair(iter, depends_units));
    fixpipe_map.emplace(make_pair(iter, outputinnermap));
    unit_list.push_back(iter);
  }
  return true;
}

const std::vector<std::string> Split(std::string &str, const std::string &pattern) {
  std::string::size_type pos;
  std::vector<std::string> result;
  str += pattern;
  size_t m_size = str.size();
  for (size_t i = 0; i < m_size; i++) {
    pos = str.find(pattern, i);
    if (static_cast<size_t>(pos) < m_size) {
      std::string s = str.substr(i, pos - i);
      result.push_back(s);
      i = pos + pattern.size() - 1;
    }
  }
  return result;
}

size_t CalDSize(const ge::DataType &data_type) {
  size_t data_size = 0;
  switch (data_type) {
    case ge::DT_BOOL:
    case ge::DT_UINT8:
    case ge::DT_INT8:
    case ge::DT_INT4:
      data_size = sizeof(uint8_t);
      break;
    case ge::DT_FLOAT16:
    case ge::DT_UINT16:
    case ge::DT_INT16:
      data_size = sizeof(uint16_t);
      break;
    case ge::DT_FLOAT:
    case ge::DT_UINT32:
    case ge::DT_INT32:
      data_size = sizeof(uint32_t);
      break;
    case ge::DT_DOUBLE:
    case ge::DT_UINT64:
    case ge::DT_INT64:
      data_size = sizeof(uint64_t);
      break;
    default:
      break;
  }
  return data_size;
}

bool RemoveAnchor(const ge::AnchorPtr &outputanchor, const ge::AnchorPtr &inputanchor) {
  if (outputanchor == nullptr || inputanchor == nullptr) {
    return false;
  }
  if (outputanchor->IsLinkedWith(inputanchor)) {
    ge::GraphUtils::RemoveEdge(outputanchor, inputanchor);
    return true;
  }
  return false;
}

bool HasEdge(const ge::AnchorPtr &outputanchor, const ge::AnchorPtr &inputanchor) {
  if (outputanchor == nullptr || inputanchor == nullptr) {
    return false;
  }
  if (outputanchor->IsLinkedWith(inputanchor)) {
    return true;
  }
  return false;
}

bool AddEdges(const ge::AnchorPtr &outputanchor, const ge::AnchorPtr &inputanchor, const ge::NodePtr output,
              const ge::NodePtr input) {
  if (outputanchor == nullptr || inputanchor == nullptr) {
    return false;
  }
  if (ge::GraphUtils::AddEdge(outputanchor, inputanchor) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][FixpipePss][AddEdges] Fail to add data edge between src node[%s] and dst node[%s].",
                    output->GetName().c_str(), input->GetName().c_str());
    return false;
  }
  return true;
}

Status HasInput1Node(const ge::NodePtr &vectornode) {
  return HasInputNode(vectornode, 1);
}

Status HasInputNode(const ge::NodePtr &vectornode, const uint32_t &input_index) {
  FE_LOGD("name = %s type = %s index = %d", vectornode->GetName().c_str(), vectornode->GetType().c_str(), input_index);
  if (vectornode->GetOpDesc()->MutableInputDesc(input_index) == nullptr) {
    FE_LOGD("MutableInputDesc() = null");
    return FAILED;
  }
  if (vectornode->GetInDataAnchor(input_index) == nullptr) {
    FE_LOGD("GetInDataAnchor() = null");
    return FAILED;
  }
  if (vectornode->GetInDataAnchor(input_index)->GetPeerOutAnchor() == nullptr) {
    FE_LOGD("GetPeerOutAnchor() = null");
    return FAILED;
  }
  if (vectornode->GetInDataAnchor(input_index)->GetPeerOutAnchor()->GetOwnerNode() == nullptr) {
    FE_LOGD("GetOwnerNode() = null");
    return FAILED;
  }
  return SUCCESS;
}

ge::NodePtr GetConstNode(const ge::NodePtr &vectornode, uint32_t &depth) {
  FE_LOGD("name = %s type = %s depth = %d", vectornode->GetName().c_str(), vectornode->GetType().c_str(), depth);
  if (depth >= MAX_DEPTH) {
    return nullptr;
  }
  if (vectornode == nullptr) {
    return nullptr;
  }
  if (vectornode->GetType() == CONSTANT) {
    FE_LOGD("name = %s type = %s is constant", vectornode->GetName().c_str(), vectornode->GetType().c_str());
    return vectornode;
  } else {
    if (HasInputNode(vectornode, 0) == SUCCESS) {
      depth++;
      auto node = vectornode->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();
      FE_LOGD("name = %s type = %s depth = %d", node->GetName().c_str(), node->GetType().c_str(), depth);
      return GetConstNode(node, depth);
    }
    return nullptr;
  }
}

ge::NodePtr HasConstNode(const ge::NodePtr &vectornode) {
  FE_LOGD("name = %s type = %s", vectornode->GetName().c_str(), vectornode->GetType().c_str());
  if (HasInputNode(vectornode, 1) != SUCCESS) {
    FE_LOGD("name = %s type = %s hast input 1 node ", vectornode->GetName().c_str(), vectornode->GetType().c_str());
    return nullptr;
  }
  auto input_node = vectornode->GetInDataAnchor(1)->GetPeerOutAnchor()->GetOwnerNode();
  if (input_node->GetType() == CONSTANT) {
    FE_LOGD("name = %s type = %s is const node ", vectornode->GetName().c_str(), vectornode->GetType().c_str());
    return input_node;
  } else {
    uint32_t depth = 0;
    return GetConstNode(input_node, depth);
  }
}

template <typename T>
bool JudgedataImpl(uint8_t *data, const size_t &data_size) {
  T *shape_data = const_cast<T *>(reinterpret_cast<const T *>(data));
  for (size_t i = 0; i < static_cast<size_t>(data_size / sizeof(T)); i++) {
    if (shape_data[i] < 0) {
      FE_LOGD("shape_data = %f", static_cast<float>(shape_data[i]));
      return false;
    }
  }
  return true;
}

bool JudegedataUInt64(uint8_t *data, const size_t &data_size) {
  uint64_t *shape_data = const_cast<uint64_t *>(reinterpret_cast<const uint64_t *>(data));
  for (size_t i = 0; i < static_cast<size_t>(data_size / sizeof(uint64_t)); i++) {
    uint32_t scale_deq = GET_DEQUANT_SCALE_DEQ(shape_data[i]);
    float scale = 0;
    if (memcpy_s(&scale, sizeof(scale), &scale_deq, sizeof(uint32_t)) != 0) {
      return false;
    }
    if (scale < 0) {
      return false;
    }
  }
  return true;
}

bool JudgedataFp16(uint8_t *data, const size_t &data_size) {
  fe::fp16_t *shape_data = const_cast<fe::fp16_t *>(reinterpret_cast<const fe::fp16_t *>(data));
  for (size_t i = 0; i < (data_size / sizeof(int16_t)); i++) {
    if (shape_data[i].ToInt16() < 0) {
      return false;
    }
  }
  return true;
}

bool Judgedata(uint8_t *data, const size_t &data_size, const ge::DataType &data_type) {
  if (data == nullptr || data_size == 0) {
    return false;
  }
  if (data_type == ge::DT_UINT64) {
    return JudegedataUInt64(data, data_size);
  } else if (data_type == ge::DT_FLOAT16) {
    return JudgedataFp16(data, data_size);
  } else {
    switch (data_type) {
      case ge::DT_INT32:
        return JudgedataImpl<int32_t>(data, data_size);
      case ge::DT_FLOAT:
        return JudgedataImpl<float>(data, data_size);
      case ge::DT_DOUBLE:
        return JudgedataImpl<double>(data, data_size);
      case ge::DT_INT8:
        return JudgedataImpl<int8_t>(data, data_size);
      case ge::DT_INT16:
        return JudgedataImpl<int16_t>(data, data_size);
      case ge::DT_BOOL:
        return JudgedataImpl<int8_t>(data, data_size);
      case ge::DT_UINT8:
      case ge::DT_UINT16:
      case ge::DT_UINT32:
        return true;
      default:
        return false;
    }
  }
}

bool JudgeConstValue(const ge::NodePtr &vectornode) {
  auto constnode = HasConstNode(vectornode);
  if (constnode == nullptr || constnode->GetType() != CONSTANT) {
    FE_LOGD("hasnot find constnode");
    return false;
  }
  ge::GeTensorDescPtr cur_tensor_desc = constnode->GetOpDesc()->MutableOutputDesc(0);
  ge::GeTensorPtr weight_value = nullptr;
  FE_MAKE_SHARED(weight_value = std::make_shared<ge::GeTensor>(), return false);
  (void)ge::AttrUtils::MutableTensor(constnode->GetOpDesc(), ge::ATTR_NAME_WEIGHTS, weight_value);
  auto datatype = cur_tensor_desc->GetDataType();
  FE_LOGD("datatype = %d", static_cast<uint32_t>(datatype));
  return Judgedata(const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(weight_value->GetData().data())),
                   weight_value->GetData().size(), datatype);
}

bool CheckIsInVector(const std::vector<FixPipeNodeInfo> &m_opnodes, const uint32_t &index) {
  if (m_opnodes.size() == 0) {
    return false;
  }
  if (index <= (m_opnodes.size() - 1)) {
    return true;
  }
  return false;
}
}  // namespace fe
