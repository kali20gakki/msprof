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

#include "common/memory_slice.h"

namespace fe {
Status Block2DataCtxParam(const Block &block, int64_t dtype_size, int64_t burst_len,
                          std::vector<DataContextParam> &data_ctx, uint32_t index) {
  if (burst_len <= 0) {
    REPORT_FE_ERROR("Burst len %ld is less than zero.", burst_len);
    return FAILED;
  }
  FE_INT64_MULCHECK(block.dim[0], dtype_size);
  int64_t dim_0_size = block.dim[0] * dtype_size;

  int64_t burst_count = dim_0_size / burst_len;
  int64_t burst_remain = dim_0_size % burst_len;
  /* Li may larger than the burst_len(which is the maximum number of
   * data in the inner dimension). We will separate the inner block
   * and give each param a additional offset(burst_len * index after seperated).
   */
  int64_t inner_block_count = burst_count + (burst_remain > 0 ? 1 : 0);

  for (int64_t i = 0; i < block.count; i++) {
    int64_t block_offset = block.offset + block.stride * i;
    FE_INT64_MULCHECK(block_offset, dtype_size);
    int64_t block_offset_size = block_offset * dtype_size;

    for (int64_t j = 0; j < inner_block_count; j++) {
      DataContextParam ctx = {};
      int64_t curr_len = (j < burst_count ? burst_len : burst_remain);
      ctx.index = index;
      ctx.len_inner = curr_len;
      ctx.num_inner = block.dim[1];
      FE_INT64_MULCHECK(block.dim_stride[1], dtype_size);
      ctx.stride_inner = block.dim_stride[1] * dtype_size;
      ctx.num_outter = block.dim[2];
      FE_INT64_MULCHECK(block.dim_stride[2], dtype_size);
      ctx.stride_outter = block.dim_stride[2] * dtype_size;
      ctx.base_addr_offset = block_offset_size + j * burst_len;
      data_ctx.push_back(ctx);
    }
  }
  return SUCCESS;
}

Status DimensionMerge(const std::vector<int64_t> &shape,
                      const std::vector<ffts::DimRange> &slice, Block &block) {
  size_t dim_count = shape.size();
  if (dim_count != slice.size()) {
    REPORT_FE_ERROR("[GenTsk][DataTsk][DimMerge] the dim count %zu is not same as slice size %zu.",
                    dim_count, slice.size());
    return FAILED;
  }

  int64_t stride = 1;
  int64_t pre_stride = 1;
  int64_t accumulation = 1;
  int curr_dim = 0;
  Block blk = {};
  for (size_t i = 0; i < dim_count; i++) {
    size_t i_reverse = dim_count - 1 - i;
    int64_t dim = shape[i_reverse];
    int64_t slice_low = slice[i_reverse].lower;
    int64_t slice_high = slice[i_reverse].higher;
    if (slice_low < 0 || slice_high <= slice_low) {
      REPORT_FE_ERROR("[GenTsk][DataTsk][DimMerge] slice_low %ld is < 0.", slice_low);
      return FAILED;
    }

    FE_LOGD("before block is: dim {%ld, %ld, %ld}, dim_stride {%ld, %ld, %ld}."
            "slice low = %ld"
            "ori dim = %ld"
            "blk.offset = %ld."
            "stride = %ld",
            blk.dim[0], blk.dim[1], blk.dim[2],
            blk.dim_stride[0], blk.dim_stride[1], blk.dim_stride[2],
            slice_low, dim, blk.offset, stride);
    if (slice_high - slice_low != dim) {
      /* If current dim is equal to 3 and slice number is 4, we cannot describe the slicing.
       * Because using inner len + outter len can not describe the 4th
       * axis slicing. */
      if (curr_dim >= kBlockDim) {
        REPORT_FE_ERROR("[GenTsk][DataTsk][DimMerge] curr_dim %d is larger than max block dim(3).",
                        curr_dim);
        return FAILED;
      }
      blk.dim[curr_dim] = (slice_high - slice_low) * accumulation;
      accumulation = 1;
      blk.offset += (slice_low * stride);
      blk.dim_stride[curr_dim] = pre_stride;
      curr_dim++;
      stride *= dim;
      pre_stride = stride;
    } else {
      // no slicing on this dim, fuse it
      stride *= dim;
      accumulation *= dim;
    }
    FE_LOGD("after block is: dim {%ld, %ld, %ld}, dim_stride {%ld, %ld, %ld}."
            "slice low = %ld"
            "ori dim = %ld"
            "blk.offset = %ld."
            "stride = %ld",
            blk.dim[0], blk.dim[1], blk.dim[2],
            blk.dim_stride[0], blk.dim_stride[1], blk.dim_stride[2],
            slice_low, dim, blk.offset, stride);
  }

  /* block dim[0] is equal to zero means we do not split any of the axes.
   * So the block dim[0] == stride of dim[0].
   * And we have only one whole inner block.
   * dim[1] = dim[2] = 1, and stride is the same as stride of
   * dim[0]. */
  for (int64_t i = 0; i < kBlockDim; i++) {
    if (blk.dim[i] == 0) {
      blk.dim[i] = accumulation;
      // in case of less than 3 split
      blk.dim_stride[i] = pre_stride;
      pre_stride = blk.dim[i] * blk.dim_stride[i];
      accumulation = 1;
    }
  }
  /* If the accumulation is not 1, we will seperate the total
   * stride into several block and for each the stride is
   * total stride divided by accumulation. */
  blk.stride = stride / accumulation;
  blk.count = accumulation;

  block = blk;
  FE_LOGD("block is: dim {%ld, %ld, %ld}, dim_stride {%ld, %ld, %ld}.",
          block.dim[0], block.dim[1], block.dim[2],
          block.dim_stride[0], block.dim_stride[1], block.dim_stride[2]);
  FE_LOGD("offset: %ld, count %ld, stride %ld.", block.offset, block.count, block.stride);
  return SUCCESS;
}

Status MemorySlice::GenerateDataCtxParam(ge::GeTensorDescPtr tensor, const std::vector<ffts::DimRange> &slice,
                                         int64_t burst_len, std::vector<DataContextParam> &data_ctx, uint32_t index) {
  auto shape = tensor->GetShape();
  FE_LOGD("Shape of index %d of node is %s.", index, StringUtils::IntegerVecToString(shape.GetDims()).c_str());
  FE_LOGD("Range is: {");
  for (auto ele : slice) {
    FE_LOGD("[%ld, %ld]", ele.lower, ele.higher);
  }
  FE_LOGD("}");
  auto dtype = tensor->GetDataType();
  Block block;
  if (DimensionMerge(shape.GetDims(), slice, block) != SUCCESS) {
    return FAILED;
  } else {
    auto iter = DATATYPE_SIZE_MAP.find(dtype);
    if (iter == DATATYPE_SIZE_MAP.end()) {
      REPORT_FE_ERROR("Data type %u is not found in DATATYPE_SIZE_MAP.", dtype);
      return FAILED;
    }
    int64_t dtype_size = iter->second;
    FE_LOGD("Burst len and data type size is %ld and %ld.", burst_len, dtype_size);
    Block2DataCtxParam(block, dtype_size, burst_len, data_ctx, index);
  }
  return SUCCESS;
}

Status GetPeerNodeAndOutIdx(const ge::NodePtr &ori_node, int ori_index,
                            ge::NodePtr &peer_node, int &peer_index) {
  auto in_anchor = ori_node->GetInDataAnchor(ori_index);
  FE_CHECK_NOTNULL(in_anchor);
  auto peer_out_anchor = in_anchor->GetPeerOutAnchor();
  FE_CHECK_NOTNULL(peer_out_anchor);

  peer_node = peer_out_anchor->GetOwnerNode();
  FE_CHECK_NOTNULL(peer_node);
  peer_index = peer_out_anchor->GetIdx();
  return SUCCESS;
}

Status GetSliceList(const ge::NodePtr &node, int index, bool is_input, ge::GeTensorDescPtr &tensor,
                    vector<std::vector<ffts::DimRange>> &slice_list) {
  ffts::ThreadSliceMapPtr slice_info_ptr = nullptr;
  slice_info_ptr = node->GetOpDesc()->TryGetExtAttr(ffts::kAttrSgtStructInfo, slice_info_ptr);
  FE_CHECK_NOTNULL(slice_info_ptr);
  uint32_t thread_id = 0;
  if (is_input) {
    ge::NodePtr peer_out_node;
    int peer_index = 0;
    if (GetPeerNodeAndOutIdx(node, index, peer_out_node, peer_index) != SUCCESS) {
      return FAILED;
    }

    if (slice_info_ptr->input_tensor_slice.size() <= thread_id) {
      REPORT_FE_ERROR("[GenTsk][DataTsk][MemSlice]input_tensor_slice of node %s is empty.",
                      node->GetName().c_str());
      return FAILED;
    }

    if (slice_info_ptr->input_tensor_slice[thread_id].size() <= static_cast<size_t>(index)) {
      REPORT_FE_ERROR("[GenTsk][DataTsk][MemSlice]size of input thread %u of node %s is %zu, which is <= %d.",
                      thread_id, node->GetName().c_str(), slice_info_ptr->input_tensor_slice[thread_id].size(), index);
      return FAILED;
    }
    tensor = peer_out_node->GetOpDesc()->MutableOutputDesc(static_cast<uint32_t>(peer_index));
    slice_list.push_back(slice_info_ptr->input_tensor_slice[thread_id][index]);
    if (slice_info_ptr->thread_mode) {
      auto last_index = slice_info_ptr->input_tensor_slice.size() - 1;
      if (slice_info_ptr->input_tensor_slice[last_index].size() <= static_cast<size_t>(index)) {
        REPORT_FE_ERROR("[GenTsk][DataTsk][MemSlice] Size of input last_index[%zu] of node[%s] is %zu, which is <= %d.",
                        last_index, node->GetName().c_str(), slice_info_ptr->input_tensor_slice[last_index].size(),
                        index);
        return FAILED;
      }
      slice_list.push_back(slice_info_ptr->input_tensor_slice[last_index][index]);
    }
  } else {
    if (slice_info_ptr->output_tensor_slice.size() <= thread_id) {
      REPORT_FE_ERROR("[GenTsk][DataTsk][MemSlice]input_tensor_slice of node %s is empty.",
                      node->GetName().c_str());
      return FAILED;
    }
    if (slice_info_ptr->output_tensor_slice[thread_id].size() <= static_cast<size_t>(index)) {
      REPORT_FE_ERROR("[GenTsk][DataTsk][MemSlice]size of output thread %u of node %s is %zu, which is <= %d.",
                      thread_id, node->GetName().c_str(), slice_info_ptr->output_tensor_slice[thread_id].size(), index);
      return FAILED;
    }
    tensor = node->GetOpDesc()->MutableOutputDesc(static_cast<uint32_t>(index));
    slice_list.push_back(slice_info_ptr->output_tensor_slice[thread_id][index]);
    if (slice_info_ptr->thread_mode) {
      auto last_index = slice_info_ptr->output_tensor_slice.size() - 1;
      if (slice_info_ptr->output_tensor_slice[last_index].size() <= static_cast<size_t>(index)) {
        REPORT_FE_ERROR("[GenTsk][DataTsk][MemSlice] Size of output last_index[%zu] of node[%s] is %zu, which is <= %d",
                        last_index, node->GetName().c_str(), slice_info_ptr->output_tensor_slice[last_index].size(),
                        index);
        return FAILED;
      }
      slice_list.push_back(slice_info_ptr->output_tensor_slice[last_index][index]);
    }
  }
  return SUCCESS;
}

Status MemorySlice::GenerateManualDataCtxParam(const ge::NodePtr &node, int index, bool is_input, int64_t burst_len,
                                               std::vector<DataContextParam> &data_ctx) {
  ge::GeTensorDescPtr tensor = nullptr;
  vector<std::vector<ffts::DimRange>> slice_list;
  FE_LOGD("Generate Manual param of node %s.", node->GetName().c_str());
  if (GetSliceList(node, index, is_input, tensor, slice_list) != SUCCESS) {
    REPORT_FE_ERROR("[GenTsk][DataTsk][GenManualDataCtxParam] Index[%d] of node[%s] Get slice list failed.",
                    index, node->GetName().c_str());
    return FAILED;
  }
  if (slice_list.size() != 1) {
    REPORT_FE_ERROR("[GenTsk][DataTsk][GenManualDataCtxParam] Index[%d] of node[%s] slice list size is %zu.",
                    index, node->GetName().c_str(), slice_list.size());
    return FAILED;
  }
  FE_CHECK_NOTNULL(tensor);
  return GenerateDataCtxParam(tensor, slice_list[0], burst_len, data_ctx, index);
}

Status MemorySlice::GenerateAutoDataCtxParam(const ge::NodePtr &node, int index, bool is_input, int64_t burst_len,
                                             std::vector<DataContextParam> &param_nontail_tail) {
  ge::GeTensorDescPtr tensor = nullptr;
  vector<std::vector<ffts::DimRange>> slice_list;
  FE_LOGD("Generate Auto param of node %s.", node->GetName().c_str());
  if (GetSliceList(node, index, is_input, tensor, slice_list) != SUCCESS) {
    REPORT_FE_ERROR("[GenTsk][DataTsk][GenAutoDataCtxParam] Index[%d] of node[%s] Get slice list failed.",
                    index, node->GetName().c_str());
    return FAILED;
  }
  if (slice_list.size() <= 1) {
    REPORT_FE_ERROR("[GenTsk][DataTsk][GenAutoDataCtxParam] Index[%d] of node[%s] slice list size is %zu.",
                    index, node->GetName().c_str(), slice_list.size());
    return FAILED;
  }
  FE_CHECK_NOTNULL(tensor);
  auto shape = tensor->MutableShape();
  FE_LOGD("[GenAutoDataCtxParam] slice size is: %zu, Shape of index %d of node %s is %s.", slice_list.size(), index,
          node->GetName().c_str(), StringUtils::IntegerVecToString(shape.GetDims()).c_str());
  FE_LOGD("Range is: {");
  for (auto eles : slice_list) {
    for (auto ele : eles) {
      FE_LOGD("[%ld, %ld]", ele.lower, ele.higher);
    }
  }
  FE_LOGD("}");

  // single data context
  for (const auto &slice : slice_list) {
     std::vector<DataContextParam> data_ctx;
     GenerateDataCtxParam(tensor, slice, burst_len, data_ctx, index);
     if (data_ctx.size() != 1) {
       FE_LOGW("[GenTsk][DataTsk][GenAutoDataCtxParam] Index[%d] of node[%s] data_ctx size:[%zu] != 1.", index,
               node->GetName().c_str(), data_ctx.size());
       return FAILED;
     }
     param_nontail_tail.push_back(data_ctx[0]);
  }
  return SUCCESS;
}
}