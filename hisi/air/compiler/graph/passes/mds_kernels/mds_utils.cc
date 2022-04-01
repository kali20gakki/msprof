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
#include "graph/passes/mds_kernels/mds_utils.h"
#include "graph/ge_context.h"

namespace ge {
namespace {
// for count
thread_local int64_t data_slice_count = 0;
thread_local int64_t data_gather_count = 0;
thread_local int64_t data_reduce_count = 0;
const std::string kPrefix = "mds";
const std::string kGradPrefix = "gradients/";
const DataType kDefaultDataType = DT_INT64;
const size_t kSizeZero = 0U;
const size_t kSizeOne = 1U;
const size_t kSizeTwo = 2U;
const size_t kSizeThree = 3U;
// Invalid location index
const int64_t kIndexInvalid = -1;
enum class TensorCutInfo { kNotSupport = 0, kSplitCutSupported = 1, kAnyCutSupported = 3 };
const int64_t kDefaultFissionFactor = 1;
const int64_t kDefaultRankSize = 1;
const std::string kDefaultGroup = "hccl_world_group";
const std::string kDefaultReduction = "sum";
const char_t *const kDefaultDeviceType = "default_device_type";
const std::set<std::string> kValidDeviceType = {kDeviceChipType, kDeviceDieType};
// deploy info
const char_t *const kAttrNeedReturnResult = "_need_return_result";
const char_t *const kAttrDeviceType = "_device_type";
const char_t *const kAttrDeviceId = "_device_id";
const char_t *const kAttrGraphName = "_graph_name";
const char_t *const kAttrGraphInputs = "_graph_inputs";
}  // namespace
int64_t MdsUtils::GetNLocation(const Format fmt) {
  NCutIndex loc = NCutIndex::kNInvalidLocation;
  switch (fmt) {
    case FORMAT_NCHW:
    case FORMAT_NC1HWC0:
    case FORMAT_NC1HWC0_C04:
    case FORMAT_NHWC:
      loc = NCutIndex::kNLocation0;
      break;
    case FORMAT_CHWN:
    case FORMAT_HWCN:
      loc = NCutIndex::kNLocation3;
      break;
    default:GELOGW("[MDS]unsupported format:%d %s", fmt, TypeUtils::FormatToSerialString(fmt).c_str());
      break;
  }
  return static_cast<int64_t>(loc);
}
int64_t MdsUtils::GetHLocation(const Format fmt) {
  HCutIndex loc = HCutIndex::kHInvalidLocation;
  switch (fmt) {
    case FORMAT_HWCN:
      loc = HCutIndex::kHLocation0;
      break;
    case FORMAT_NHWC:
    case FORMAT_CHWN:
      loc = HCutIndex::kHLocation1;
      break;
    case FORMAT_NCHW:
      loc = HCutIndex::kHLocation2;
      break;
    default:GELOGE(FAILED, "[MDS]unsupported format:%d %s", fmt, TypeUtils::FormatToSerialString(fmt).c_str());
      break;
  }
  return static_cast<int64_t>(loc);
}
int64_t MdsUtils::GetIndexByFormat(const ConstGeTensorDescPtr &ge_tensor_desc, const CutType type) {
  int64_t format = kIndexInvalid;
  const Format fmt = ge_tensor_desc->GetFormat();
  switch (type) {
    case CutType::kCutN:
      format = GetNLocation(fmt);
      break;
    case CutType::kCutH:
      format = GetHLocation(fmt);
      break;
    default:
      GELOGE(FAILED, "[MDS]invalid CutType:%d", static_cast<int32_t>(type));
      break;
  }
  return format;
}
bool MdsUtils::IsDistributedDeploySupported(const GeTensorDescPtr &ge_tensor_desc, const CutType type,
                                            const bool check_divisible) {
  if (ge_tensor_desc == nullptr) {
    REPORT_INNER_ERROR("E19999", "invalid input param: tensor is null!");
    GELOGE(FAILED, "[MDS]invalid input param: tensor is null!");
    return false;
  }
  if ((type != CutType::kCutN) && (type != CutType::kCutH)) {
    REPORT_INNER_ERROR("E19999", "invalid CutType:%d", static_cast<int32_t>(type));
    GELOGE(FAILED, "[MDS]invalid CutType:%d", static_cast<int32_t>(type));
    return false;
  }
  const int64_t cut_index = GetIndexByFormat(ge_tensor_desc, type);
  if (cut_index == kIndexInvalid) {
    GELOGW("[MDS] can not get cut-able index from tensor, just return.");
    return false;
  }
  const auto dims = ge_tensor_desc->GetShape().GetDims();
  if ((cut_index < 0) || (cut_index >= static_cast<int64_t>(dims.size()))) {
    GELOGW("[MDS] cut_index %ld for CutType %d is out of range of dims size %zu", cut_index, static_cast<int32_t>(type),
           dims.size());
    return false;
  }
  if ((dims[static_cast<size_t>(cut_index)] % kDeployNumber) != 0) {
    GELOGW("[MDS] cut_index %ld for CutType %d with dim %ld can not deploy", cut_index, static_cast<int32_t>(type),
           dims[static_cast<size_t>(cut_index)]);
    if (check_divisible) {
      GELOGI("[MDS] deploy is not supported");
      return false;
    }
  }
  std::vector<std::vector<int64_t>> cut_support_info;
  if (!(AttrUtils::GetListListInt(*ge_tensor_desc, ATTR_NAME_CUT_INFO, cut_support_info))) {
    GELOGW("[MDS] get %s failed", ATTR_NAME_CUT_INFO.c_str());
    return false;
  }
  if (cut_support_info.empty()) {
    GELOGW("[MDS] %s is empty", ATTR_NAME_CUT_INFO.c_str());
    return false;
  }
  // for now, get cut_support_info[0] for cutn debug
  if ((cut_index < 0) || (cut_index >= static_cast<int64_t>(cut_support_info[0U].size()))) {
    REPORT_INNER_ERROR("E19999", "cut_index %ld for CutType %d is out of range of cut_support_info[0] size %zu",
                       cut_index, static_cast<int32_t>(type), cut_support_info[0U].size());
    GELOGE(FAILED, "[MDS] cut_index %ld for CutType %d is out of range of cut_support_info[0] size %zu",
           cut_index, static_cast<int32_t>(type), cut_support_info[0U].size());
    return false;
  }
  if ((cut_support_info[0U][static_cast<size_t>(cut_index)] < static_cast<int64_t>(TensorCutInfo::kNotSupport)) ||
      (cut_support_info[0U][static_cast<size_t>(cut_index)] > static_cast<int64_t>(TensorCutInfo::kAnyCutSupported))) {
    REPORT_INNER_ERROR("E19999", "invalid cut info value:%ld", cut_support_info[0U][static_cast<size_t>(cut_index)]);
    GELOGE(FAILED, "[MDS] invalid cut info value:%ld", cut_support_info[0U][static_cast<size_t>(cut_index)]);
    return false;
  }
  GELOGD("get cut info from attr successful");
  return cut_support_info[0U][static_cast<size_t>(cut_index)] ==
      static_cast<int64_t>(TensorCutInfo::kSplitCutSupported);
}

Status MdsUtils::SetAttrForHcomNode(const OpDescPtr &hcom_op, const int64_t fission_factor,
                                    const std::string &group_name) {
  GE_CHECK_NOTNULL(hcom_op);
  REQUIRE(fission_factor > kDefaultFissionFactor, "fission_factor %ld need be bigger than %ld", fission_factor,
          kDefaultFissionFactor);
  REQUIRE(ge::AttrUtils::SetInt(hcom_op, ATTR_NAME_FISSION_FACTOR, fission_factor),
          "Failed to set attr fission_factor %ld for op:%s(%s)", fission_factor, hcom_op->GetName().c_str(),
          hcom_op->GetType().c_str());

  if (!group_name.empty()) {
    REQUIRE(ge::AttrUtils::SetStr(hcom_op, HCOM_ATTR_GROUP, group_name), "Failed to set attr group %s for op:%s(%s)",
            group_name.c_str(), hcom_op->GetName().c_str(), hcom_op->GetType().c_str());
  }
  return SUCCESS;
}

bool MdsUtils::IsMDSNeeded() {
  std::string device_type(kDefaultDeviceType);
  if ((ge::GetContext().GetOption(ge::OPTION_DEVICE_TYPE, device_type) != GRAPH_SUCCESS) ||
      (device_type == kDefaultDeviceType)) {
    GELOGI("[MDS]device type is %s, skip mds", device_type.c_str());
    return false;
  }
  if (kValidDeviceType.count(device_type) == kSizeZero) {
    REPORT_INNER_ERROR("E19999", "invalid %s with value:%s", OPTION_DEVICE_TYPE, device_type.c_str());
    GELOGE(FAILED, "[MDS] invalid %s with value:%s", OPTION_DEVICE_TYPE, device_type.c_str());
    return false;
  }
  // Parse the configuration file of the system to get the sys_config_exe_unit in the future
  const std::string sys_config_exe_unit(kDeviceDieType);
  return device_type != sys_config_exe_unit;
}

Status MdsUtils::SetDevice(const int32_t device) {
  if (IsMDSNeeded()) {
    GE_CHK_RT_RET(rtSetDeviceV2(device, RT_DEVICE_MODE_MULTI_DIE));
  } else {
    GE_CHK_RT_RET(rtSetDevice(device));
  }
  return SUCCESS;
}

Status MdsUtils::CtxCreate(rtContext_t *const ctx, const uint32_t flags, const int32_t device) {
  if (IsMDSNeeded()) {
    GE_CHK_RT_RET(rtCtxCreateV2(ctx, flags, device, RT_DEVICE_MODE_MULTI_DIE));
  } else {
    GE_CHK_RT_RET(rtCtxCreate(ctx, flags, device));
  }
  return SUCCESS;
}

CutType MdsUtils::TryGetGraphCutType(const ComputeGraphPtr &compute_graph) {
  bool is_unknown_graph = false;
  if (GraphUtils::IsUnknownShapeGraph(compute_graph)) {
    GELOGI("Graph %s is unknown shape graph", compute_graph->GetName().c_str());
    is_unknown_graph = true;
  }
  CutType selected_cut_type = CutType::kNoCut;
  for (const auto &data : compute_graph->GetDirectNode()) {
    if (data->GetType() != DATA) {
      continue;
    }
    GELOGI("Get graph input %s %s", data->GetName().c_str(), data->GetType().c_str());
    const auto data_n_index = MdsUtils::GetIndexByFormat(data->GetOpDesc()->GetOutputDescPtr(kSizeZero),
                                                         CutType::kCutN);
    if (data_n_index == kIndexInvalid) {
      continue;
    }
    const auto data_n_dim = data->GetOpDesc()->GetOutputDesc(kSizeZero).GetShape().
                                GetDim(static_cast<size_t>(data_n_index));
    if ((data_n_dim % kDeployNumber) == 0) {
      selected_cut_type = is_unknown_graph ? CutType::kDynamicCutN : CutType::kCutN;
      break;
    }
    const auto data_h_index = MdsUtils::GetIndexByFormat(data->GetOpDesc()->GetOutputDescPtr(kSizeZero),
                                                         CutType::kCutH);
    if (data_h_index == kIndexInvalid) {
      continue;
    }
    const auto data_h_dim = data->GetOpDesc()->GetOutputDesc(kSizeZero).GetShape().
                                GetDim(static_cast<size_t>(data_h_index));
    if ((data_h_dim % kDeployNumber) == 0) {
      selected_cut_type = is_unknown_graph ? CutType::kDynamicCutH : CutType::kCutH;
    }
    if ((data_n_dim < 0) && (data_h_dim < 0)) {
      selected_cut_type = CutType::kDynamicCutAll;
    }
  }
  return selected_cut_type;
}
Status MdsUtils::SetDeployInfo(const ComputeGraphPtr &compute_graph,
                               const std::multimap<int64_t, GraphInputs> &deploys, const std::string &device_type) {
  GE_CHECK_NOTNULL(compute_graph);
  GELOGD("[MDS]%s SetDeployInfo start", compute_graph->GetName().c_str());

  // build deploy info
  std::vector<GeAttrValue::NAMED_ATTRS> deploy_info;
  for (const auto &pair : deploys) {
    const int64_t device_id = pair.first;
    GeAttrValue::NAMED_ATTRS thread_instance;
    thread_instance.SetName(std::to_string(device_id));
    // only first deploy instance return result
    (void)ge::AttrUtils::SetBool(thread_instance, kAttrNeedReturnResult, deploy_info.empty());
    (void)ge::AttrUtils::SetInt(thread_instance, kAttrDeviceId, device_id);
    (void)ge::AttrUtils::SetStr(thread_instance, kAttrDeviceType, device_type);
    (void)ge::AttrUtils::SetStr(thread_instance, kAttrGraphName, compute_graph->GetName());
    (void)ge::AttrUtils::SetListTensor(thread_instance, kAttrGraphInputs, pair.second);
    deploy_info.emplace_back(thread_instance);
    GELOGD("[MDS]%s SetDeployInfo on device id: %ld", compute_graph->GetName().c_str(), device_id);
  }
  // set deploy info
  REQUIRE(ge::AttrUtils::SetListNamedAttrs(*compute_graph, ATTR_NAME_DEPLOY_INFO, deploy_info),
          "Set attr failed for graph %s", compute_graph->GetName().c_str());

  return SUCCESS;
}

bool MdsUtils::IsGradNode(const NodePtr &node) {
  const auto &node_name = node->GetName();
  bool is_grad_node = false;
  if (ge::AttrUtils::GetBool(node->GetOpDesc(), ATTR_NAME_GRADIENT_NODE, is_grad_node) && is_grad_node) {
    GELOGI("node %s is grad node from attr", node_name.c_str());
    return true;
  }
  if (node_name.find(kGradPrefix) == kSizeZero) {
    GELOGI("node %s is grad node from name", node_name.c_str());
    return true;
  }
  return false;
}

Status MdsUtils::DataGather(const OutDataAnchorPtr &src, const InDataAnchorPtr &dst) {
  const auto src_node = src->GetOwnerNode();
  GE_CHECK_NOTNULL(src_node);
  const auto dst_node = dst->GetOwnerNode();
  GE_CHECK_NOTNULL(dst_node);
  const auto src_graph = src_node->GetOwnerComputeGraph();
  GE_CHECK_NOTNULL(src_graph);
  const auto src_node_output_desc = src_node->GetOpDesc()->GetOutputDesc(static_cast<uint32_t>(src->GetIdx()));
  const auto dst_node_input_desc = dst_node->GetOpDesc()->GetInputDesc(static_cast<uint32_t>(dst->GetIdx()));
  if (src_node_output_desc.GetShape().ToString() == dst_node_input_desc.GetShape().ToString()) {
    GELOGI("[MDS][DataGather] construct gather node between [%s][%d] and [%s][%d] failed, just return",
           src_node->GetName().c_str(),
           src->GetIdx(),
           dst_node->GetName().c_str(),
           dst->GetIdx());
    return SUCCESS;
  }
  const std::string node_name_suffix("_" + kPrefix + "_" + std::to_string(data_gather_count++));
  const auto hcom_allgather_node =
      AddSingleInputOutputNode(src_graph, HCOMALLGATHER + node_name_suffix, HCOMALLGATHER, src_node_output_desc);
  GE_CHECK_NOTNULL(hcom_allgather_node);
  MDS_REQUIRE_SUCCESS(GraphUtils::InsertNodeAfter(src, {dst}, hcom_allgather_node),
                      "[DataGather] failed between %s and %s", src_node->GetName().c_str(),
                      dst_node->GetName().c_str());
  MDS_REQUIRE_SUCCESS(MdsUtils::SetAttrForHcomNode(hcom_allgather_node->GetOpDesc(), kDeployNumber, kDefaultGroup),
                      "[DataGather]set attr for node for %s(%s) failed", hcom_allgather_node->GetName().c_str(),
                      hcom_allgather_node->GetType().c_str());
  REQUIRE(ge::AttrUtils::SetInt(hcom_allgather_node->GetOpDesc(), HCOM_ATTR_RANK_SIZE, kDefaultRankSize),
          "Failed to set attr reduction type %s for op:%s(%s)", kDefaultReduction.c_str(),
          hcom_allgather_node->GetName().c_str(), hcom_allgather_node->GetType().c_str());
  MDS_REQUIRE_SUCCESS(InferShapeAndType(hcom_allgather_node), "[DataGather] %s call infershape failed",
                      hcom_allgather_node->GetName().c_str());
  return SUCCESS;
}

// gradients->ApplyMomentum
// we want to reduce gradients on different device(die), so graph topo changed to
// gradients->hcomallreducemean->ApplyMomentum; Because 'mean' is not currently supported by hcomallreduce,
// topo will end up like gradients->hcomallreducesum->div->ApplyMomentum
Status MdsUtils::DataReduce(const OutDataAnchorPtr &src, const InDataAnchorPtr &dst) {
  const auto src_node = src->GetOwnerNode();
  GE_CHECK_NOTNULL(src_node);
  const auto dst_node = dst->GetOwnerNode();
  GE_CHECK_NOTNULL(dst_node);

  const auto src_graph = src_node->GetOwnerComputeGraph();
  GE_CHECK_NOTNULL(src_graph);
  NodePtr all_reduce_node = nullptr;
  if (NeedInsertHcomAllReduce(src_node, all_reduce_node)) {
    MDS_REQUIRE_SUCCESS(ConstructReduceNode(src_graph, src, dst, all_reduce_node),
                        "[DataReduce] construct allreduce node for %s failed", all_reduce_node->GetName().c_str());
    GE_CHECK_NOTNULL(all_reduce_node);
  } else {
    GE_CHECK_NOTNULL(all_reduce_node);
    MDS_REQUIRE_SUCCESS(MdsUtils::SetAttrForHcomNode(all_reduce_node->GetOpDesc(), kDeployNumber),
                        "[DataReduce][Modify] set attr for allreduce node for %s failed",
                        all_reduce_node->GetName().c_str());
  }
  return SUCCESS;
}

// tensor t with shape like [n,c,h,w], we want get [0:2/n, c, h, w] and [2/n : n, c, h, w] on different
// device; To achieve this goal, we use slice nodes.
// slice(t, [i * n/2, 0, 0, 0], [n/2, c, h, w]) i=0,1
// slice three input like : t->slice; data(0,1)->mul(n/2)->pack[i*n/2,0,0,0]->slice;  const(n,c,h,w)->slice
Status MdsUtils::DataSlice(const OutDataAnchorPtr &src, const InDataAnchorPtr &dst, const CutType cut_type,
                           NodePtr &input_node) {
  const auto src_node = src->GetOwnerNode();
  GE_CHECK_NOTNULL(src_node);
  const auto dst_node = dst->GetOwnerNode();
  GE_CHECK_NOTNULL(dst_node);
  const auto src_graph = src_node->GetOwnerComputeGraph();
  GE_CHECK_NOTNULL(src_graph);
  const auto tensor_desc = dst_node->GetOpDesc()->MutableInputDesc(static_cast<uint32_t>(dst->GetIdx()));
  if (!MdsUtils::IsDistributedDeploySupported(tensor_desc, cut_type, true)) {
    GELOGI("[MDS][DataSlice] construct slice node between [%s][%d] and [%s][%d] failed, just return",
           src_node->GetName().c_str(),
           src->GetIdx(),
           dst_node->GetName().c_str(),
           dst->GetIdx());
    return SUCCESS;
  }
  if (input_node == nullptr) {
    const std::string input_node_name = std::string(DATA) + "_" + kPrefix + "_" + std::to_string(0);
    GeTensorDesc ge_tensor_desc;
    ge_tensor_desc.SetDataType(kDefaultDataType);
    input_node = AddSingleInputOutputNode(src_graph, input_node_name, DATA, ge_tensor_desc);
  }
  // cut_dim is valid here
  const auto cut_dim = GetIndexByFormat(tensor_desc, cut_type);
  NodePtr slice_node = nullptr;
  MDS_REQUIRE_SUCCESS(ConstructSliceNode(src_graph, tensor_desc, input_node, static_cast<int8_t>(cut_dim), slice_node),
                      "[DataSlice] construct slice node for %s failed", src_node->GetName().c_str());
  GE_CHECK_NOTNULL(slice_node);
  MDS_REQUIRE_SUCCESS(GraphUtils::InsertNodeAfter(src, {dst}, slice_node), "[DataSlice] failed between %s and %s",
                      src_node->GetName().c_str(), dst_node->GetName().c_str());
  const auto slice_op_desc = slice_node->GetOpDesc();
  GE_CHECK_NOTNULL(slice_op_desc);
  (void)slice_op_desc->UpdateInputDesc(0U, src_node->GetOpDesc()->GetOutputDesc(static_cast<uint32_t>(src->GetIdx())));
  (void)slice_op_desc->UpdateOutputDesc(0U, src_node->GetOpDesc()->GetOutputDesc(static_cast<uint32_t>(src->GetIdx())));
  MDS_REQUIRE_SUCCESS(InferShapeAndType(slice_node), "[DataSlice] %s call infer shape failed",
                      slice_node->GetName().c_str());
  (void)slice_op_desc->MutableInputDesc(0U)->SetShapeRange({});
  (void)slice_op_desc->MutableOutputDesc(0U)->SetShapeRange({});
  return SUCCESS;
}

Status MdsUtils::ConstructSliceNode(const ComputeGraphPtr &src_graph, const ConstGeTensorDescPtr &tensor_desc,
                                    const NodePtr &input_node, const int8_t cut_index, NodePtr &slice_node) {
  std::vector<int64_t> slice_sizes = tensor_desc->GetShape().GetDims();
  // Express with graph structure in the future
  slice_sizes[static_cast<size_t>(cut_index)] /= kDeployNumber;
  std::vector<GeTensorPtr> ge_tensors;
  GeTensorDesc ge_tensor_desc;
  ge_tensor_desc.SetDataType(kDefaultDataType);
  MDS_REQUIRE_SUCCESS(MdsUtils::ConstructTensorDescWithData(ge_tensor_desc, slice_sizes, ge_tensors),
                      "[ConstructTensorDescWithData] failed");
  const GeTensorPtr slice_size_tensor = ge_tensors[kSizeZero];
  const auto slice_size = AddConstNodeToGraph(slice_size_tensor, src_graph);

  const std::vector<int64_t> slice_offset_other_dim{0};
  ge_tensors.clear();
  MDS_REQUIRE_SUCCESS(MdsUtils::ConstructTensorDescWithData(ge_tensor_desc, slice_offset_other_dim, ge_tensors, true),
                      "[ConstructTensorDescWithData] failed");
  const GeTensorPtr slice_offset_tensor = ge_tensors[kSizeZero];
  const auto slice_offset = AddConstNodeToGraph(slice_offset_tensor, src_graph);

  const std::vector<int64_t> slice_offset_first_dim_value{slice_sizes[static_cast<size_t>(cut_index)]};
  ge_tensors.clear();
  MDS_REQUIRE_SUCCESS(MdsUtils::ConstructTensorDescWithData(ge_tensor_desc,
                                                            slice_offset_first_dim_value,
                                                            ge_tensors,
                                                            true),
                      "[ConstructTensorDescWithData] failed");
  const GeTensorPtr slice_offset_first_dim_tensor = ge_tensors[kSizeZero];
  const auto slice_offset_first_dim = AddConstNodeToGraph(slice_offset_first_dim_tensor, src_graph);
  const std::string node_name_suffix("_" + kPrefix + "_" + std::to_string(data_slice_count++));
  const NodePtr mul_node = AddDynamicInputOutputNode(src_graph, MUL, MUL + node_name_suffix, kSizeTwo, kSizeOne);
  GE_CHECK_NOTNULL(input_node);
  MDS_REQUIRE_SUCCESS(GraphUtils::AddEdge(input_node->GetOutDataAnchor(static_cast<int32_t>(kSizeZero)),
                                          mul_node->GetInDataAnchor(static_cast<int32_t>(kSizeZero))),
                      "[ConstructSliceNode] add edge failed");
  MDS_REQUIRE_SUCCESS(
      GraphUtils::AddEdge(slice_offset_first_dim->GetOutDataAnchor(static_cast<int32_t>(kSizeZero)),
                          mul_node->GetInDataAnchor(static_cast<int32_t>(kSizeOne))),
      "[ConstructSliceNode] add edge failed");
  MDS_REQUIRE_SUCCESS(InferShapeAndType(mul_node), "[DataSlice] %s call infer shape failed",
                      mul_node->GetName().c_str());
  const NodePtr pack_node = AddDynamicInputOutputNode(src_graph, PACK, PACK + node_name_suffix, slice_sizes.size(),
                                                      kSizeOne);
  for (const auto &in_anchor : pack_node->GetAllInDataAnchors()) {
    if (in_anchor->GetIdx() == cut_index) {
      MDS_REQUIRE_SUCCESS(GraphUtils::AddEdge(mul_node->GetOutDataAnchor(static_cast<int32_t>(kSizeZero)), in_anchor),
                          "[ConstructSliceNode] add edge failed");
    } else {
      MDS_REQUIRE_SUCCESS(GraphUtils::AddEdge(slice_offset->GetOutDataAnchor(static_cast<int32_t>(kSizeZero)),
                                              in_anchor),
                          "[ConstructSliceNode] add edge failed");
    }
  }
  const auto pack_op_desc = pack_node->GetOpDesc();
  GE_CHECK_NOTNULL(pack_op_desc);
  (void) AttrUtils::SetInt(pack_op_desc, PACK_ATTR_NAME_NUM, static_cast<int64_t>(slice_sizes.size()));
  (void) AttrUtils::SetInt(pack_op_desc, ATTR_NAME_AXIS, static_cast<int64_t>(kSizeZero));
  MDS_REQUIRE_SUCCESS(InferShapeAndType(pack_node), "[DataSlice] %s call infer shape failed",
                      pack_node->GetName().c_str());
  slice_node = AddDynamicInputOutputNode(src_graph, SLICE, SLICE + node_name_suffix, kSizeThree, kSizeOne);
  MDS_REQUIRE_SUCCESS(GraphUtils::AddEdge(pack_node->GetOutDataAnchor(static_cast<int32_t>(kSizeZero)),
                                          slice_node->GetInDataAnchor(static_cast<int32_t>(kSizeOne))),
                      "[ConstructSliceNode] add edge failed");
  MDS_REQUIRE_SUCCESS(GraphUtils::AddEdge(slice_size->GetOutDataAnchor(static_cast<int32_t>(kSizeZero)),
                                          slice_node->GetInDataAnchor(static_cast<int32_t>(kSizeTwo))),
                      "[ConstructSliceNode] add edge failed");
  return SUCCESS;
}

NodePtr MdsUtils::AddSingleInputOutputNode(const ComputeGraphPtr &graph, const string &name, const string &type,
                                           const GeTensorDesc &tensor) {
  GELOGI("Begin to create op: %s", name.c_str());
  OpDescBuilder op_desc_builder(name, type);
  const OpDescPtr op_desc = op_desc_builder.AddInput("x", tensor).AddOutput("y", tensor).Build();
  if (op_desc == nullptr) {
    REPORT_CALL_ERROR("E19999", "Create op_desc:%s(%s) failed", name.c_str(), type.c_str());
    GELOGE(FAILED, "[Create][OpDesc] failed, name:%s(%s).", name.c_str(), type.c_str());
    return nullptr;
  }
  const NodePtr node = graph->AddNode(op_desc);
  if (node == nullptr) {
    REPORT_CALL_ERROR("E19999", "Add node:%s(%s) to graph:%s failed", op_desc->GetName().c_str(),
                      op_desc->GetType().c_str(), graph->GetName().c_str());
    GELOGE(FAILED, "[Add][Node] %s(%s) to graph:%s failed", op_desc->GetName().c_str(), op_desc->GetType().c_str(),
           graph->GetName().c_str());
    return nullptr;
  }
  return node;
}

NodePtr MdsUtils::AddDynamicInputOutputNode(const ComputeGraphPtr &graph, const std::string &type,
                                            const std::string &node_name, const size_t input_num,
                                            const size_t output_num) {
  GELOGI("Begin to create op: %s", node_name.c_str());
  OpDescBuilder op_desc_builder(node_name, type);
  const OpDescPtr op_desc = op_desc_builder.AddDynamicInput("x", static_cast<uint32_t>(input_num)).
                          AddDynamicOutput("y", static_cast<uint32_t>(output_num)).Build();
  if (op_desc == nullptr) {
    REPORT_CALL_ERROR("E19999", "Create op_desc:%s(%s) failed", node_name.c_str(), type.c_str());
    GELOGE(FAILED, "[Create][OpDesc] failed, name:%s(%s).", node_name.c_str(), type.c_str());
    return nullptr;
  }
  const NodePtr node = graph->AddNode(op_desc);
  if (node == nullptr) {
    REPORT_CALL_ERROR("E19999", "Add node:%s(%s) to graph:%s failed", op_desc->GetName().c_str(),
                      op_desc->GetType().c_str(), graph->GetName().c_str());
    GELOGE(FAILED, "[Add][Node] %s(%s) to graph:%s failed", op_desc->GetName().c_str(), op_desc->GetType().c_str(),
           graph->GetName().c_str());
    return nullptr;
  }
  return node;
}

NodePtr MdsUtils::AddConstNodeToGraph(const GeTensorPtr &tensor, const ComputeGraphPtr &graph) {
  const auto const_desc = OpDescUtils::CreateConstOp(tensor);
  if (const_desc == nullptr) {
    REPORT_CALL_ERROR("E19999", "Create Const op failed");
    GELOGE(OUT_OF_MEMORY, "[Create][ConstOp] failed");
    return nullptr;
  }

  if (graph == nullptr) {
    GELOGW("input param graph is null");
    return nullptr;
  }
  return graph->AddNodeFront(const_desc);
}

Status MdsUtils::ConstructReduceNode(const ComputeGraphPtr &src_graph, const OutDataAnchorPtr &src,
                                     const InDataAnchorPtr &dst, NodePtr &reduce_node) {
  const std::string node_name_suffix("_" + kPrefix + "_" + std::to_string(data_reduce_count++));
  reduce_node =
      AddDynamicInputOutputNode(src_graph, HCOMALLREDUCE, HCOMALLREDUCE + node_name_suffix, kSizeOne, kSizeOne);

  MDS_REQUIRE_SUCCESS(GraphUtils::InsertNodeAfter(src, {dst}, reduce_node),
                      "[DataReduce] failed insert %s between %s and %s", reduce_node->GetName().c_str(),
                      src->GetOwnerNode()->GetName().c_str(), dst->GetOwnerNode()->GetName().c_str());
  MDS_REQUIRE_SUCCESS(MdsUtils::SetAttrForHcomNode(reduce_node->GetOpDesc(), kDeployNumber, kDefaultGroup),
                      "[DataReduce][Create] set attr for allreduce node for %s failed", reduce_node->GetName().c_str());
  REQUIRE(ge::AttrUtils::SetStr(reduce_node->GetOpDesc(), HCOM_ATTR_REDUCE_TYPE, kDefaultReduction),
          "Failed to set attr reduction type %s for op:%s(%s)", kDefaultReduction.c_str(),
          reduce_node->GetName().c_str(), reduce_node->GetType().c_str());
  MDS_REQUIRE_SUCCESS(InferShapeAndType(reduce_node), "[DataReduce] %s call infershape failed",
                      reduce_node->GetName().c_str());
  const auto div_node = AddDynamicInputOutputNode(src_graph, REALDIV, REALDIV + node_name_suffix, kSizeTwo, kSizeOne);
  const std::vector<int64_t> slice_sizes{kDeployNumber};
  std::vector<GeTensorPtr> ge_tensors;
  GeTensorDesc ge_tensor_desc;
  ge_tensor_desc.SetDataType(kDefaultDataType);
  MDS_REQUIRE_SUCCESS(MdsUtils::ConstructTensorDescWithData(ge_tensor_desc, slice_sizes, ge_tensors),
                      "[ConstructReduceNode] failed");
  REQUIRE(!ge_tensors.empty(), "[ConstructReduceNode] failed");
  const auto div_input = AddConstNodeToGraph(ge_tensors[kSizeZero], src_graph);
  MDS_REQUIRE_SUCCESS(GraphUtils::AddEdge(div_input->GetOutDataAnchor(static_cast<int32_t>(kSizeZero)),
                                          div_node->GetInDataAnchor(static_cast<int32_t>(kSizeOne))),
                      "[ConstructSliceNode] add edge failed");
  MDS_REQUIRE_SUCCESS(GraphUtils::InsertNodeAfter(reduce_node->GetOutDataAnchor(static_cast<int32_t>(kSizeZero)), {dst},
                                                  div_node),
                      "[DataReduce] failed insert %s between %s and %s", div_node->GetName().c_str(),
                      reduce_node->GetName().c_str(), dst->GetOwnerNode()->GetName().c_str());
  MDS_REQUIRE_SUCCESS(InferShapeAndType(div_node), "[DataReduce] %s call infershape failed",
                      div_node->GetName().c_str());
  return SUCCESS;
}

bool MdsUtils::NeedInsertHcomAllReduce(const NodePtr &src_node, NodePtr &allreduce_node) {
  // recognize that the graph is originally a multi-p model, that is, there is already an allreduce node,
  // so there is no need to insert allreduce node
  GE_RT_FALSE_CHECK_NOTNULL(src_node);
  std::vector<NodePtr> nodes{src_node};
  std::set<NodePtr> nodes_visited{src_node};
  while (!nodes.empty()) {
    const auto node = nodes.back();
    nodes.pop_back();
    if (node->GetType() == HCOMALLREDUCE) {
      allreduce_node = node;
      return false;
    }
    for (const auto &in_node: node->GetInAllNodes()) {
      if (in_node->GetType() == HCOMALLREDUCE) {
        allreduce_node = node;
        return false;
      }
      if (nodes_visited.insert(in_node).second) {
        (void)nodes.insert(nodes.cbegin(), in_node);
      }
    }
    for (const auto &out_node: node->GetOutAllNodes()) {
      if (out_node->GetType() == HCOMALLREDUCE) {
        allreduce_node = node;
        return false;
      }
      if (nodes_visited.insert(out_node).second) {
        (void)nodes.insert(nodes.cbegin(), out_node);
      }
    }
  }
  return true;
}

Status MdsUtils::InferShapeAndType(const NodePtr &node) {
  GE_CHECK_NOTNULL(node);
  return ShapeRefiner::InferShapeAndType(node, false);
}

Status MdsUtils::ConstructTensorDescWithData(const GeTensorDesc &out_desc, const std::vector<int64_t> &data,
                                             std::vector<GeTensorPtr> &v_output, const bool scalar_output) {
  const auto dim_size = data.size();
  const DataType data_type = out_desc.GetDataType();
  if (data_type == DT_INT32) {
    std::unique_ptr<int32_t[]> buf = MakeUnique<int32_t[]>(dim_size);
    GE_CHECK_NOTNULL(buf);
    for (size_t i = 0U; i < dim_size; i++) {
      if (data[i] >= INT_MAX) {
        REPORT_CALL_ERROR("E19999", "Param data:%s will overflow after multi", formats::JoinToString(data).c_str());
        GELOGE(PARAM_INVALID, "[Check][Param] int32 overflow, data[%zu]:%ld", i, data[i]);
        return PARAM_INVALID;
      }
      buf[i] = static_cast<int32_t>(data[i]);
    }
    return ConstructTensorDescWithData(out_desc, buf.get(), static_cast<uint32_t>(dim_size), v_output, scalar_output);
  } else if (data_type == DT_INT64) {
    std::unique_ptr<int64_t[]> buf = MakeUnique<int64_t[]>(dim_size);
    GE_CHECK_NOTNULL(buf);
    for (size_t i = 0U; i < dim_size; i++) {
      buf[i] = data[i];
    }
    return ConstructTensorDescWithData(out_desc, buf.get(), static_cast<uint32_t>(dim_size), v_output, scalar_output);
  } else {
    GELOGW("[Check][Param] Only support DT_INT32 and DT_INT64. data_type:%s not support",
           TypeUtils::DataTypeToSerialString(data_type).c_str());
  }
  return SUCCESS;
}

template<typename T>
Status MdsUtils::ConstructTensorDescWithData(const GeTensorDesc &out_desc, T *const buf, const uint32_t len,
                                             std::vector<GeTensorPtr> &v_output, const bool scalar_output) {
  // construct TensorDesc
  const GeShape out_shape = (scalar_output ? GeShape() : GeShape({static_cast<int64_t>(len)}));
  GeTensorDesc output_tensor_desc(out_desc);
  output_tensor_desc.SetShape(out_shape);
  output_tensor_desc.SetOriginShape(out_shape);

  const GeTensorPtr output_tensor_ptr = MakeShared<GeTensor>(
      output_tensor_desc, reinterpret_cast<uint8_t *>(buf), sizeof(T) * len);
  if (output_tensor_ptr == nullptr) {
    REPORT_CALL_ERROR("E19999", "New GeTensor failed");
    GELOGE(MEMALLOC_FAILED, "[New][GeTensor] failed");
    return MEMALLOC_FAILED;
  }

  v_output.push_back(output_tensor_ptr);
  return SUCCESS;
}
}  // namespace ge
