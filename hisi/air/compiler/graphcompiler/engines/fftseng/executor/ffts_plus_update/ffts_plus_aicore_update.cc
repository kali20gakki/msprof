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
#include "ffts_plus_aicore_update.h"
#include <nlohmann/json.hpp>
#include "inc/ffts_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/op_desc_utils.h"
namespace ffts {
REGISTER_FFTS_PLUS_CTX_UPDATER(kCoreTypeAIC, FFTSPlusAICoreUpdate);
REGISTER_FFTS_PLUS_CTX_UPDATER(kCoreTypeAIV, FFTSPlusAICoreUpdate);
REGISTER_FFTS_PLUS_CTX_UPDATER(kCoreTypeMixAIC, FFTSPlusAICoreUpdate);
REGISTER_FFTS_PLUS_CTX_UPDATER(kCoreTypeMixAIV, FFTSPlusAICoreUpdate);

static const std::vector<std::string>g_cmo_key = {"prefetch_idx", "invalidate_idx", "write back_idx"};
static const int g_cmo_type[kMaxCmoType] = {RT_CTX_TYPE_FLUSH_DATA, RT_CTX_TYPE_INVALIDATE_DATA,
                                            RT_CTX_TYPE_WRITEBACK_DATA};
static const bool g_cmo_flag[kMaxCmoType] = {true, false, false};
static const size_t kMinThreadNum = 2;

FFTSPlusAICoreUpdate::FFTSPlusAICoreUpdate():input_num_(0), input_output_num_(0), task_param_offset_(0),
                                             slice_info_ptr_(nullptr), data_params_(nullptr)
{}
FFTSPlusAICoreUpdate::~FFTSPlusAICoreUpdate() {
  if (data_params_) {
    delete []data_params_;
  }
}

Status FFTSPlusAICoreUpdate::CalcAutoThreadWorkspace(const vector<optiling::utils::OpRunInfo> &op_run_info,
                                                     ge::AutoThreadParam &args_para)
{
  if (op_run_info.empty()) {
    FFTS_LOGE("Op run info is empty.");
    return FAILED;
  }
  int64_t workspace;
  auto &run_info = op_run_info[0];
  for (size_t j = 0; j < run_info.GetWorkspaceNum(); ++j) {
    (void)run_info.GetWorkspace(j, workspace);
    args_para.task_addr_offset.emplace_back(static_cast<uint64_t>(workspace));
  }
  return SUCCESS;
}

Status CalcThreadOffset(vector<ffts::DimRange> &tensor_range, int64_t &thread_offset, const uint32_t &data_type_size)
{
  thread_offset = 1;
  for (size_t i = 0; i < tensor_range.size(); ++i) {
    int64_t range = tensor_range[i].higher - tensor_range[i].lower;
    FFTS_INT64_MULCHECK(thread_offset, range);
    thread_offset *= range;
  }
  FFTS_INT64_MULCHECK(thread_offset, data_type_size);
  thread_offset = thread_offset * data_type_size;
  return SUCCESS;
}

Status FFTSPlusAICoreUpdate::CalcAutoThreadInput(const ge::NodePtr &node,
                                                 vector<vector<vector<ffts::DimRange>>> &tensor_slice,
                                                 ge::AutoThreadParam &argsPara)
{
  if (tensor_slice.empty()) {
    FE_LOGE("[Executor][FFTS_plus_update][CalcAutoThreadInput]Input tensor slice is empty.");
    return FAILED;
  }
  size_t cut_index = 0;
  int64_t thread_offset = 0;
  auto op_desc = node->GetOpDesc();
  for (auto const &anchor : node->GetAllInDataAnchors()) {
    if (ge::AnchorUtils::GetStatus(anchor) == ge::ANCHOR_SUSPEND) {
      continue;
    }
    uint32_t anchor_index = anchor->GetIdx();
    if (cut_index >= slice_info_ptr_->input_tensor_indexes.size() ||
        anchor_index < slice_info_ptr_->input_tensor_indexes[cut_index]) {
      argsPara.task_addr_offset.emplace_back(0);
      continue;
    }
    cut_index = static_cast<size_t>(anchor_index);
    ge::GeTensorDescPtr tensor_desc_ptr = op_desc->MutableInputDesc(anchor_index);
    if (tensor_desc_ptr == nullptr) {
      continue;
    }
    uint32_t data_type_size = 0;
    (void)GetDataTypeSize(tensor_desc_ptr->GetDataType(), data_type_size);
    if (anchor_index >= tensor_slice[0].size()) {
      FFTS_LOGE("[Executor][FFTS_plus_update][CalcAutoThreadInput]Anchor index:%u over slice num:%zu.",
                anchor_index, tensor_slice[0].size());
      return FAILED;
    }
    auto &tensor_range = tensor_slice[0][anchor_index];
    if (CalcThreadOffset(tensor_range, thread_offset, data_type_size) != SUCCESS) {
      FFTS_LOGE("[Executor][FFTS_plus_update][CalcAutoThreadInput]Calculate Thread Offset failed.");
      return FAILED;
    }
    FFTS_LOGD("Input anchorIndex: %u, thread_offset: %ld.", anchor_index, thread_offset);
    argsPara.task_addr_offset.emplace_back(thread_offset);
  }
  return SUCCESS;
}

Status FFTSPlusAICoreUpdate::CalcAutoThreadOutput(const ge::NodePtr &node,
                                                  vector<vector<vector<ffts::DimRange>>> &tensor_slice,
                                                  ge::AutoThreadParam &argsPara)
{
  if (tensor_slice.empty()) {
    FE_LOGE("Output tensor slice is empty.");
    return FAILED;
  }
  size_t cut_index = 0;
  if (node->GetAllOutDataAnchorsSize() > tensor_slice[0].size()) {
    FFTS_LOGE("[Executor][FFTS_plus_update][CalcAutoThreadOutput]Anchor size:%u over slice num:%zu.",
              node->GetAllOutDataAnchorsSize(), tensor_slice[0].size());
    return FAILED;
  }
  int64_t thread_offset = 0;
  auto op_desc = node->GetOpDesc();
  for (auto const &anchor : node->GetAllOutDataAnchors()) {
    uint32_t anchor_index = anchor->GetIdx();
    if (cut_index >= slice_info_ptr_->output_tensor_indexes.size() ||
      anchor_index < slice_info_ptr_->output_tensor_indexes[cut_index]) {
      argsPara.task_addr_offset.emplace_back(0);
      continue;
    }
    cut_index = static_cast<size_t>(anchor_index);
    ge::GeTensorDescPtr tensor_desc_ptr = op_desc->MutableOutputDesc(anchor_index);
    if (tensor_desc_ptr == nullptr) {
      continue;
    }
    uint32_t data_type_size = 0;
    (void)GetDataTypeSize(tensor_desc_ptr->GetDataType(), data_type_size);
    auto &tensor_range = tensor_slice[0][anchor_index];
    if (CalcThreadOffset(tensor_range, thread_offset, data_type_size) != SUCCESS) {
      FFTS_LOGE("[Executor][FFTS_plus_update][CalcAutoThreadOutput]Calculate Thread Offset failed.");
      return FAILED;
    }
    FFTS_LOGD("Output Index: %u, thread_offset: %ld.", anchor_index, thread_offset);
    argsPara.task_addr_offset.emplace_back(thread_offset);
  }
  return SUCCESS;
}

Status FFTSPlusAICoreUpdate::GetAutoThreadParam(const ge::NodePtr &node,
                                                const vector<optiling::utils::OpRunInfo> &op_run_info,
                                                ge::AutoThreadParam &auto_thread_param)
{
  FFTS_LOGD("Get node:%s auto thread param.", node->GetName().c_str());
  ffts::ThreadSliceMapPtr slice_info_ptr = nullptr;
  slice_info_ptr = node->GetOpDesc()->TryGetExtAttr(ffts::kAttrSgtStructInfo, slice_info_ptr);
  FFTS_CHECK_NOTNULL(slice_info_ptr);
  slice_info_ptr_ = slice_info_ptr;
  uint32_t thread_num = slice_info_ptr->slice_instance_num;
  if (thread_num == 0) {
    FFTS_LOGE("[Executor][FFTS_plus_update][GetAutoThreadParam]Slice num is zero.");
    return FAILED;
  }
  Status ret = CalcAutoThreadInput(node, slice_info_ptr->input_tensor_slice, auto_thread_param);
  if (ret != SUCCESS) {
    FFTS_LOGE("[Executor][FFTS_plus_update][GetAutoThreadParam]Calculate thread input failed.");
    return FAILED;
  }
  input_num_ = auto_thread_param.task_addr_offset.size();
  ret = CalcAutoThreadOutput(node, slice_info_ptr->output_tensor_slice, auto_thread_param);
  if (ret != SUCCESS) {
    FFTS_LOGE("[Executor][FFTS_plus_update][GetAutoThreadParam]Calculate thread output failed.");
    return FAILED;
  }
  input_output_num_ = auto_thread_param.task_addr_offset.size();
  CalcAutoThreadWorkspace(op_run_info, auto_thread_param);
  auto_thread_param.thread_dim = thread_num;
  auto_thread_param.input_output_num = input_output_num_;
  size_t args_num = auto_thread_param.task_addr_offset.size();
  task_param_offset_ = args_num * sizeof(uint64_t);
  FFTS_LOGD("AutoThreadParam input_output_num:%u, args_num:%zu.", input_output_num_, args_num);
  return SUCCESS;
}

Status FFTSPlusAICoreUpdate::UpdateAicAivCtx(rtFftsPlusComCtx_t *com_ctx,
                                             const ge::AutoThreadSubTaskFlush &flush_data) const
{
  FFTS_CHECK_NOTNULL(com_ctx);
  auto ctx = reinterpret_cast<rtFftsPlusAicAivCtx_t*>(com_ctx);
  if (flush_data.op_run_info.size() < kMinThreadNum) {
    FFTS_LOGE("[Executor][FFTS_plus_update][UpdateAicAivCtx]AicAivCtx run_info size(%zu) err.",
              flush_data.op_run_info.size());
    return FAILED;
  }
  ctx->nonTailBlockdim = flush_data.op_run_info[0].GetBlockDim();
  ctx->tailBlockdim = flush_data.op_run_info[1].GetBlockDim();

  FFTS_LOGD("AicAivCtx nonTail_Blkdim[%d], tail_Blkdim[%d].", ctx->nonTailBlockdim, ctx->tailBlockdim);

  ctx->threadDim = slice_info_ptr_->slice_instance_num;
  uint64_t paraBase = reinterpret_cast<uintptr_t>(flush_data.args_base);

  SetLow32FromSrc(ctx->taskParamPtrBaseL, paraBase);
  SetHigh16FromSrc(ctx->taskParamPtrBaseH, paraBase);
  SetLow32FromSrc(ctx->nonTailTaskStartPcL, flush_data.aic_non_tail_task_start_pc);
  SetHigh16FromSrc(ctx->nonTailTaskStartPcH, flush_data.aic_non_tail_task_start_pc);
  SetLow32FromSrc(ctx->tailTaskStartPcL, flush_data.aic_tail_task_start_pc);
  SetHigh16FromSrc(ctx->tailTaskStartPcH, flush_data.aic_tail_task_start_pc);
  FFTS_LOGD("Ctx Base:0x%lx, PtrBaseL:0x%x, PtrBaseH:0x%x, PcL:0x%x, PcH:0x%x, PcLTail:0x%x, PcHTail:0x%x.", paraBase,
            ctx->taskParamPtrBaseL, ctx->taskParamPtrBaseH,
            ctx->nonTailTaskStartPcL, ctx->nonTailTaskStartPcH, ctx->tailTaskStartPcL, ctx->tailTaskStartPcH);
  ctx->icachePrefetchCnt = flush_data.aic_icache_prefetch_cnt;
  ctx->taskParamPtrOffset = task_param_offset_;
  return SUCCESS;
}

Status GenerateDataCtxParam(ge::GeTensorDescPtr tensor, const std::vector<DimRange> &slice,
                            DataContextParam* data_ctx, uint32_t index)
{
  const auto &shape = tensor->GetShape().GetDims();
  FFTS_LOGD("Shape of index %d of node is %s.", index, fe::StringUtils::IntegerVecToString(shape).c_str());
  auto dtype = tensor->GetDataType();
  auto iter = DATATYPE_SIZE_MAP.find(dtype);
  if (iter == DATATYPE_SIZE_MAP.end()) {
    REPORT_FFTS_ERROR("Data type %u is not found in DATATYPE_SIZE_MAP.", dtype);
    return FAILED;
  }
  int64_t dtype_size = iter->second;
  size_t dim_count = shape.size();
  int64_t stride = 1;
  //  low to high
  for (size_t i = 0; i < dim_count - 1; i++) {
    size_t i_reverse = dim_count - 1 - i;
    FFTS_LOGD("Before calc:%zu is: high:%ld, low:%ld, dim:%ld, stride = %ld", i, slice[i_reverse].higher,
              slice[i_reverse].lower, shape[i_reverse], stride);
    stride *= shape[i_reverse];
  }
  int64_t dim_0_size = (slice[0].higher - slice[0].lower) * stride;
  int64_t block_offset = slice[0].lower * stride;
  stride *= shape[0];
  FFTS_INT64_MULCHECK(dim_0_size, dtype_size);
  dim_0_size *= dtype_size;
  data_ctx->len_inner = dim_0_size;
  FFTS_INT64_MULCHECK(block_offset, dtype_size);
  block_offset *= dtype_size;
  data_ctx->base_addr_offset = block_offset;
  data_ctx->index = index;
  data_ctx->num_inner = 1;
  FFTS_INT64_MULCHECK(stride, dtype_size);
  data_ctx->stride_inner = stride * dtype_size;
  data_ctx->num_outter = 1;
  FFTS_INT64_MULCHECK(stride, dtype_size);
  data_ctx->stride_outter = data_ctx->stride_inner;
  FFTS_LOGD("Data param: len_inner:%ld, offset %ld, stride_inner %ld.", dim_0_size, block_offset,
            data_ctx->stride_inner);
  return SUCCESS;
}

Status FFTSPlusAICoreUpdate::GenerateOutputCmoParam(const ge::NodePtr &node, int real_index, uint32_t index)
{
  ge::GeTensorDescPtr tensor = node->GetOpDesc()->MutableOutputDesc(static_cast<uint32_t>(index));
  FFTS_CHECK_NOTNULL(tensor);
  if (slice_info_ptr_->output_tensor_slice.front().size() <= static_cast<size_t>(index)) {
    FFTS_LOGE("[Executor][FFTS_plus_update][GenerateOutputCmoParam]Input thread 0 size of node %s <= %d",
              node->GetName().c_str(), index);
    return FAILED;
  }
  Status ret = GenerateDataCtxParam(tensor, slice_info_ptr_->output_tensor_slice.front()[index],
                                    &data_params_[kEachParamNum * real_index], index);
  if (ret != SUCCESS) {
    FFTS_LOGE("[Executor][FFTS_plus_update][GenerateOutputCmoParam]Generate notail failed.");
    return FAILED;
  }
  if (slice_info_ptr_->slice_instance_num < kMinThreadNum) {
    return SUCCESS;
  }
  if (slice_info_ptr_->output_tensor_slice.back().size() <= static_cast<size_t>(index)) {
    FFTS_LOGE("[Executor][FFTS_plus_update][GenerateOutputCmoParam]Input last thread size of node %s <= %d",
              node->GetName().c_str(), index);
    return FAILED;
  }
  ret = GenerateDataCtxParam(tensor, slice_info_ptr_->output_tensor_slice.back()[index],
                             &data_params_[kEachParamNum * real_index + 1], index);
  if (ret != SUCCESS) {
    FFTS_LOGE("[Executor][FFTS_plus_update][GenerateOutputCmoParam]Generate notail failed.");
    return FAILED;
  }
  return SUCCESS;
}

Status FFTSPlusAICoreUpdate::GenerateInputCmoParam(const ge::NodePtr &node, int real_index, uint32_t index)
{
  ge::GeTensorDescPtr tensor = node->GetOpDesc()->MutableInputDesc(static_cast<uint32_t>(index));
  FFTS_CHECK_NOTNULL(tensor);
  if (slice_info_ptr_->input_tensor_slice.front().size() <= static_cast<size_t>(index)) {
    FFTS_LOGE("[Executor][FFTS_plus_update][GenerateInputCmoParam]Input thread 0 size of node %s <= %d",
              node->GetName().c_str(), index);
    return FAILED;
  }
  Status ret = GenerateDataCtxParam(tensor, slice_info_ptr_->input_tensor_slice.front()[index],
                                    &data_params_[kEachParamNum * real_index], index);
  if (ret != SUCCESS) {
    FFTS_LOGE("[Executor][FFTS_plus_update][GenerateInputCmoParam]Generate notail failed.");
    return FAILED;
  }
  if (slice_info_ptr_->slice_instance_num < kMinThreadNum) {
    return SUCCESS;
  }
  if (slice_info_ptr_->input_tensor_slice.back().size() <= static_cast<size_t>(index)) {
    FFTS_LOGE("[Executor][FFTS_plus_update][GenerateInputCmoParam]Input last thread size of node %s <= %d",
              node->GetName().c_str(), index);
    return FAILED;
  }
  ret = GenerateDataCtxParam(tensor, slice_info_ptr_->input_tensor_slice.back()[index],
                             &data_params_[kEachParamNum * real_index + 1], index);
  if (ret != SUCCESS) {
    FFTS_LOGE("[Executor][FFTS_plus_update][GenerateInputCmoParam]Generate notail failed.");
    return FAILED;
  }
  return SUCCESS;
}

inline void FillDataCtxDmu(rtFftsPlusDataCtx_t* ctx, const DataContextParam& data_para,
                           const DataContextParam& tail_data_para)
{
  ctx->nonTailLengthInner = data_para.len_inner;
  ctx->nonTailNumInner = data_para.num_inner;
  ctx->nonTailNumOutter = data_para.num_outter;
  ctx->nonTailStrideInner = data_para.stride_inner;
  ctx->nonTailStrideOutter = data_para.stride_outter;
  ctx->tailLengthInner = tail_data_para.len_inner;
  ctx->tailNumInner = tail_data_para.num_inner;
  ctx->tailNumOutter = tail_data_para.num_outter;
  ctx->tailStrideInner = tail_data_para.stride_inner;
  ctx->tailStrideOutter = tail_data_para.stride_outter;
  return;
}

Status FFTSPlusAICoreUpdate::FillPrefetchCtxParam(rtFftsPlusDataCtx_t* ctx,
                                                  const ge::AutoThreadSubTaskFlush &flush_data, size_t index) const
{
  const auto &data_para = data_params_[kEachParamNum * index];
  const auto &tail_data_para = data_params_[kEachParamNum * index + 1];
  FillDataCtxDmu(ctx, data_para, tail_data_para);
  if (data_para.index >= input_num_) {
    FFTS_LOGE("[Executor][FFTS_plus_update][FillPrefetchCtxParam]Index(%u) over input_num(%u).",
              data_para.index, input_num_);
    return FAILED;
  }
  if (data_para.index >= flush_data.input_addr_base.size()) {
    FFTS_LOGE("[Executor][FFTS_plus_update][FillPrefetchCtxParam]Index(%u) over addrs(%zu).",
              data_para.index, flush_data.input_addr_base.size());
    return FAILED;
  }
  uint64_t para_base = flush_data.input_addr_base[data_para.index];
  SetLow32FromSrc(ctx->addressBaseL, para_base);
  SetHigh32FromSrc(ctx->addressBaseH, para_base);
  uint32_t offset = tail_data_para.base_addr_offset;
  if (slice_info_ptr_->slice_instance_num > 1) {
    offset = tail_data_para.base_addr_offset / (slice_info_ptr_->slice_instance_num - 1);
  }
  ctx->addressOffset = offset;
  ctx->threadDim = slice_info_ptr_->slice_instance_num;
  FFTS_LOGD("Prefetch DataCtx index[%u], base(0x%lx), Offset[%u], oriOffset:%ld.", data_para.index, para_base,
            offset, tail_data_para.base_addr_offset);
  return SUCCESS;
}

Status FFTSPlusAICoreUpdate::FillInvAndWriCtxParam(rtFftsPlusDataCtx_t* ctx,
                                                   const ge::AutoThreadSubTaskFlush &flush_data, size_t index) const
{
  const auto &data_para = data_params_[kEachParamNum * index];
  const auto &tail_data_para = data_params_[kEachParamNum * index + 1];
  FillDataCtxDmu(ctx, data_para, tail_data_para);
  // output_offset = input_size + index
  uint32_t out_num = input_output_num_ - input_num_;
  FFTS_LOGI("out_num(%u), outIndex(%u), total_num(%u).", out_num, data_para.index, input_output_num_);
  if (data_para.index >= out_num) {
    FFTS_LOGE("[Executor][FFTS_plus_update][FillInvAndWriCtxParam]Output index over.");
    return FAILED;
  }
  if (data_para.index >= flush_data.output_addr_base.size()) {
    FFTS_LOGE("[Executor][FFTS_plus_update][FillInvAndWriCtxParam]Index(%u) over addrs(%zu).",
              data_para.index, flush_data.output_addr_base.size());
    return FAILED;
  }
  uint64_t para_base = flush_data.output_addr_base[data_para.index];
  SetLow32FromSrc(ctx->addressBaseL, para_base);
  SetHigh32FromSrc(ctx->addressBaseH, para_base);
  ctx->addressOffset = data_para.base_addr_offset;
  ctx->threadDim = slice_info_ptr_->slice_instance_num;
  FFTS_LOGD("Invalid DataCtx addressOffset[%ld], paraBase(0x%lx).", data_para.base_addr_offset, para_base);
  return SUCCESS;
}

void GetNodeCMOCtxList(ge::OpDescPtr op_desc, int type, std::vector<std::vector<int64_t>> &ctx_id_vec)
{
	if (op_desc == nullptr) {
	  return;
	}
	if (type == RT_CTX_TYPE_FLUSH_DATA) {
	  (void)ge::AttrUtils::GetListListInt(op_desc, "_data_prefetch_ctx_id_list", ctx_id_vec);
	} else if (type == RT_CTX_TYPE_INVALIDATE_DATA) {
	  (void)ge::AttrUtils::GetListListInt(op_desc, "_invalid_ctx_id_list", ctx_id_vec);
	} else if (type == RT_CTX_TYPE_WRITEBACK_DATA) {
	  (void)ge::AttrUtils::GetListListInt(op_desc, "_write_back_ctx_id_list", ctx_id_vec);
	}
	FFTS_LOGD("Get CMO type(%d) context list num(%zu).", type, ctx_id_vec.size());
	return;
}

/*
 * |                thread0              |               thread1               |              thread2                |
 * |                OP_Ctx_0             |               OP_Ctx_1              |              OP_Ctx_2               |
 * | Data_Ctx_00,Data_Ctx_10,Data_Ctx_20 | Data_Ctx_01,Data_Ctx_11,Data_Ctx_21 |  Data_Ctx_02,Data_Ctx_12,Data_Ctx_22|
 * */
Status FFTSPlusAICoreUpdate::UpdateCmoCtxProc(const rtFftsPlusTaskInfo_t &task_info, const ge::NodePtr &node,
                                              const ge::AutoThreadSubTaskFlush &flush_data, int type, int data_num)
{
  auto op_desc = node->GetOpDesc();
  uint16_t window_size = slice_info_ptr_->parallel_window_size;
  std::vector<std::vector<int64_t>> ctx_id_vec;
  GetNodeCMOCtxList(op_desc, type, ctx_id_vec);
  FFTS_LOGD("Context num(%zu), param_size(%d).", ctx_id_vec.size(), data_num);
  if (ctx_id_vec.empty()) {
    return SUCCESS;
  }
  rtFftsPlusComCtx_t *context_head = (rtFftsPlusComCtx_t*)(task_info.descBuf);
  rtFftsPlusComCtx_t *com_ctx = nullptr;
  uint16_t ctx_Id;
  Status ret = FAILED;
  uint16_t real_size = window_size <= slice_info_ptr_->slice_instance_num ?
                       window_size : slice_info_ptr_->slice_instance_num;
  if (ctx_id_vec.size() < real_size) {
    FFTS_LOGE("[Executor][FFTS_plus_update][UpdateCmoCtxProc]Data context num less than windowsize.");
    return FAILED;
  }
  for (size_t i = 0; i < static_cast<size_t>(data_num); ++i) {
    for (uint16_t j = 0; j < real_size; ++j) {
      ctx_Id = ctx_id_vec[j][i];
      FFTS_LOGD("Data ctxId(%u).", ctx_Id);
      if (ctx_Id >= task_info.fftsPlusSqe->totalContextNum) {
        FFTS_LOGE("[Executor][FFTS_plus_update][UpdateCmoCtxProc]Data(%u) ctx Id err.", ctx_Id);
        return FAILED;
      }
      com_ctx = context_head + ctx_Id;
      auto data_ctx = reinterpret_cast<rtFftsPlusDataCtx_t*>(com_ctx);
      if (data_ctx->contextType == RT_CTX_TYPE_FLUSH_DATA) {
        ret = FillPrefetchCtxParam(data_ctx, flush_data, i);
      } else if (data_ctx->contextType == RT_CTX_TYPE_INVALIDATE_DATA ||
             data_ctx->contextType == RT_CTX_TYPE_WRITEBACK_DATA) {
        ret = FillInvAndWriCtxParam(data_ctx, flush_data, i);
      }
      if (ret != SUCCESS) {
        FFTS_LOGE("[Executor][FFTS_plus_update][UpdateCmoCtxProc]Fill CTX type(%d) Index(%zu) thread(%u) err.",
                  data_ctx->contextType, i, j);
        return FAILED;
      }
    }
  }
  return SUCCESS;
}

Status FFTSPlusAICoreUpdate::UpdateCmoProc(const rtFftsPlusTaskInfo_t &task_info, const ge::NodePtr &node,
                                           const ge::AutoThreadSubTaskFlush &flush_data)
{
  std::vector<int32_t> cmo_idx;
  if (data_params_ == nullptr) {
    data_params_ = new DataContextParam[kEachParamNum * kMaxIndexNum];
    FFTS_CHECK_NOTNULL(data_params_);
  }
  for (size_t i = 0; i < kMaxCmoType; ++i) {
    rtFftsPlusContextType_t hw_type = static_cast<rtFftsPlusContextType_t>(g_cmo_type[i]);
    bool is_input = g_cmo_flag[i];
    (void)ge::AttrUtils::GetListInt(node->GetOpDesc(), g_cmo_key[i], cmo_idx);
    FFTS_LOGD("Get %s's index num %zu.", is_input == true ? "input" : "output", cmo_idx.size());
    int real_index = 0;
    for (auto index : cmo_idx) {
      if (real_index >= kMaxIndexNum) {
        break;
      }
      // one to one
      Status ret;
      if (is_input) {
        ret = GenerateInputCmoParam(node, real_index, index);
      } else {
        ret = GenerateOutputCmoParam(node, real_index, index);
      }
      if (ret != SUCCESS) {
        FFTS_LOGW("[Executor][FFTS_plus_update][UpdateCmoProc]Calculate data param err.");
        return SUCCESS;
      }
      real_index++;
      // get prefetch max num
      if (is_input && real_index == kMaxPrefetchNum) {
        break;
      }
    }
    if (UpdateCmoCtxProc(task_info, node, flush_data, hw_type, real_index) != SUCCESS) {
      FFTS_LOGW("[Executor][FFTS_plus_update][UpdateCmoProc]Update data context err.");
      return SUCCESS;
    }
  }
  return SUCCESS;
}

Status FFTSPlusAICoreUpdate::UpdateContextByType(const ge::NodePtr node, const ge::AutoThreadSubTaskFlush &flush_data,
                                                 const rtFftsPlusTaskInfo_t &task_info,
                                                 const vector<int32_t> &ctx_id_vec)
{
  void *ctx_buf = const_cast<void*>(task_info.descBuf);
  rtFftsPlusComCtx_t *context_head = reinterpret_cast<rtFftsPlusComCtx_t*>(ctx_buf);
  rtFftsPlusComCtx_t *context = nullptr;
  for (auto idx = ctx_id_vec.begin(); idx != ctx_id_vec.end(); ++idx) {
    if ((*idx) > task_info.fftsPlusSqe->totalContextNum) {
      FFTS_LOGE("[Executor][FFTS_plus_update][UpdateSubtaskAndCache]Context Id(%d) over err.", (*idx));
      return FAILED;
    }
    context = context_head + (*idx);
    FFTS_LOGD("Context Id[%d] , type[0x%x].", *idx, context->contextType);
    if (UpdateAicAivCtx(context, flush_data) != SUCCESS) {
      FFTS_LOGE("[Executor][FFTS_plus_update][UpdateSubtaskAndCache]Update context failed.");
      return FAILED;
    }
  }
  return SUCCESS;
}

Status FFTSPlusAICoreUpdate::UpdateMixL2AicAivCtx(const rtFftsPlusTaskInfo_t &task_info,
                                                  const ge::AutoThreadSubTaskFlush &flush_data,
                                                  const ge::NodePtr node) const
{
  void *ctx_buf = const_cast<void*>(task_info.descBuf);
  rtFftsPlusComCtx_t *context_head = reinterpret_cast<rtFftsPlusComCtx_t*>(ctx_buf);
  rtFftsPlusMixAicAivCtx_t *ctx = nullptr;
  uint32_t contextId;
  (void)ge::AttrUtils::GetInt(node->GetOpDesc(), kContextId, contextId);
  if (contextId > task_info.fftsPlusSqe->totalContextNum) {
    FFTS_LOGE("[Executor][FFTS_plus_update][UpdateMixL2AicAivCtx]Context Id(%d) over err.", contextId);
    return FAILED;
  }
  ctx = reinterpret_cast<rtFftsPlusMixAicAivCtx_t*>(context_head + contextId);
  FFTS_CHECK_NOTNULL(ctx);
  uint64_t paraBase = reinterpret_cast<uintptr_t>(flush_data.args_base);
  SetLow32FromSrc(ctx->aicTaskParamPtrL, paraBase);
  SetHigh16FromSrc(ctx->aicTaskParamPtrH, paraBase);
  ctx->aicTaskParamPtrOffset = 0;
  ctx->aivTaskParamPtrH = ctx->aicTaskParamPtrH;
  ctx->aivTaskParamPtrL = ctx->aicTaskParamPtrL;
  ctx->aivTaskParamPtrOffset = 0;
  if (node->GetOpDesc()->HasAttr("_mix_aic" + ATTR_NAME_KERNEL_LIST_FIRST_NAME) ||
      node->GetOpDesc()->HasAttr("_mix_aiv" + ATTR_NAME_KERNEL_LIST_FIRST_NAME)) {
    if (flush_data.op_run_info.empty()) {
      FFTS_LOGE("[Executor][FFTS_plus_update][UpdateMixL2AicAivCtx]MixAicAivCtx runinfo size(%zu) err.",
                flush_data.op_run_info.size());
      return FAILED;
    }
    ctx->nonTailBlockdim = flush_data.op_run_info[0].GetBlockDim();
    ctx->tailBlockdim = flush_data.op_run_info[0].GetBlockDim();
    FFTS_LOGD("MixL2 nonTail_Block_dim:%d, tail_Block_dim:%d", ctx->nonTailBlockdim, ctx->tailBlockdim);
    ctx->aicIcachePrefetchCnt = flush_data.aic_icache_prefetch_cnt;
    SetLow32FromSrc(ctx->nonTailAicTaskStartPcL, flush_data.aic_non_tail_task_start_pc);
    SetHigh16FromSrc(ctx->nonTailAicTaskStartPcH, flush_data.aic_non_tail_task_start_pc);
    SetLow32FromSrc(ctx->tailAicTaskStartPcL, flush_data.aic_non_tail_task_start_pc);
    SetHigh16FromSrc(ctx->tailAicTaskStartPcH, flush_data.aic_non_tail_task_start_pc);
    ctx->aivIcachePrefetchCnt = flush_data.aiv_icache_prefetch_cnt;
    SetLow32FromSrc(ctx->nonTailAivTaskStartPcL, flush_data.aiv_non_tail_task_start_pc);
    SetHigh16FromSrc(ctx->nonTailAivTaskStartPcH, flush_data.aiv_non_tail_task_start_pc);
    SetLow32FromSrc(ctx->tailAivTaskStartPcL, flush_data.aiv_non_tail_task_start_pc);
    SetHigh16FromSrc(ctx->tailAivTaskStartPcH, flush_data.aiv_non_tail_task_start_pc);
  }
  FFTS_LOGD("MixL2 context update success");
  return SUCCESS;
}

Status FFTSPlusAICoreUpdate::UpdateSubTaskAndCache(const ge::NodePtr &node,
                                                   const ge::AutoThreadSubTaskFlush &flush_data,
                                                   rtFftsPlusTaskInfo_t &task_info)
{
  FFTS_CHECK_NOTNULL(node);
  ge::OpDescPtr op_desc = node->GetOpDesc();
  if (op_desc->HasAttr(ATTR_NAME_ALIAS_ENGINE_NAME)) {
    return UpdateMixL2AicAivCtx(task_info, flush_data, node);
  }
  FFTS_CHECK_NOTNULL(task_info.descBuf);
  FFTS_CHECK_NOTNULL(slice_info_ptr_);
  FFTS_LOGI("FFTS plus update, node name(%s), ctxNum(%u), mode(%u), threadDim(%u), windowSize(%u).",
            node->GetName().c_str(), task_info.fftsPlusSqe->totalContextNum, slice_info_ptr_->thread_mode,
            slice_info_ptr_->slice_instance_num, slice_info_ptr_->parallel_window_size);

  // get node contextId list
  std::vector<int32_t> ctx_id_vec;
  (void)ge::AttrUtils::GetListInt(op_desc, kAutoCtxIdList, ctx_id_vec);
  if (ctx_id_vec.empty()) {
    FFTS_LOGE("[Executor][FFTS_plus_update][UpdateSubtaskAndCache]Node context list empty.");
    return FAILED;
  }
  if (UpdateCmoProc(task_info, node, flush_data) != SUCCESS) {
    FFTS_LOGW("[Executor][FFTS_plus_update][UpdateSubtaskAndCache]Update CMO fail.");
  }
  if (UpdateContextByType(node, flush_data, task_info, ctx_id_vec) != SUCCESS) {
    return FAILED;
  }
  FFTS_LOGD("FFTS+ node context update success.");
  return SUCCESS;
}
}