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

#include "incremental_model_parser.h"
#include "framework/common/helper/model_helper.h"
#include "graph/utils/graph_utils.h"
#include "proto/task.pb.h"

namespace ge {
IncrementalModelParser::IncrementalModelParser(uint64_t model_size) : model_size_(model_size) {
}

Status IncrementalModelParser::Init(const void *buffer, size_t size) {
  if (root_model_ != nullptr) {
    GELOGE(FAILED, "repeat init");
    return FAILED;
  }

  // parse header
  GE_CHECK_GE(size, sizeof(ModelFileHeader));
  root_model_ = MakeShared<GeRootModel>();
  GE_CHECK_NOTNULL(root_model_);
  const auto *header = static_cast<const ModelFileHeader *>(buffer);
  GELOGD("[Parse][Header] model version = %u", header->version);
  if (header->version >= ge::MODEL_VERSION) {
    num_models_ = header->model_num;
    GELOGD("number of submodels = %u", num_models_);
  }
  if (header->length != model_size_ - sizeof(ModelFileHeader)) {
    GELOGE(FAILED,
           "header->length = %u, model_size = %lu, sizeof(ModelFileHeader) = %zu",
           header->length,
           model_size_,
           sizeof(ModelFileHeader));
    return FAILED;
  }

  return SUCCESS;
}

Status IncrementalModelParser::ParsePartitionTable() {
  auto *const partitions = reinterpret_cast<ModelPartitionMemInfo *>(current_parse_state_.buf);
  for (uint32_t i = 0U; i < num_partitions_; ++i) {
    ModelPartitionDesc partition{};
    partition.size = partitions[i].mem_size;
    partition.type = ToParsePartitionType(partitions[i].type);
    partition_descs_.emplace_back(partition);
    GELOGD("Partition[%u], type:%d, size:%lu", i, static_cast<int32_t>(partition.type), partition.size);
  }
  return SUCCESS;
}

Status IncrementalModelParser::CheckParams(uint64_t offset, const void *model_buffer, uint64_t buffer_size) const {
  (void) buffer_size;
  GE_CHECK_NOTNULL(model_buffer);
  if (completed_) {
    GELOGE(FAILED, "Already completed");
    return FAILED;
  }

  if (offset != expected_offset_) {
    GELOGE(PARAM_INVALID, "[Check][Offset] failed, expected = %lu, actual = %lu", expected_offset_, offset);
    return PARAM_INVALID;
  }

  return SUCCESS;
}

Status IncrementalModelParser::ParseAndDeserialize(uint64_t offset, const void *model_buffer, uint64_t buffer_size) {
  GE_CHK_STATUS_RET_NOLOG(CheckParams(offset, model_buffer, buffer_size));
  expected_offset_ += buffer_size;
  GELOGD("[Recv][ModelData] offset = %lu, size = %lu, progress: %lu/%lu",
         offset,
         buffer_size,
         expected_offset_,
         model_size_);
  BufferReader reader(model_buffer, buffer_size);
  if (offset == 0) {
    GE_CHK_STATUS_RET(Init(model_buffer, buffer_size), "Failed to init parser");
    reader.AdvanceBy(sizeof(ModelFileHeader));
    GE_CHK_STATUS_RET(ResetParseState(), "Failed to reset parse state");
  }

  while (true) {
    auto remaining_size = reader.GetSize();
    auto need_by_current_partition = current_parse_state_.partition_size - current_parse_state_.current_offset;
    GELOGD("[Parse][Model] current partition need additional %lu bytes, incoming size = %lu",
           need_by_current_partition, remaining_size);

    if (remaining_size < need_by_current_partition) {
      GE_CHK_STATUS_RET_NOLOG(reader.DrainTo(current_parse_state_, remaining_size));
      return SUCCESS;
    }

    GE_CHK_STATUS_RET_NOLOG(reader.DrainTo(current_parse_state_, need_by_current_partition));
    GE_CHK_STATUS_RET(ParseCurrentPartition());
    if (current_parse_state_.index == partition_descs_.size() - 1U) {
      bool has_next = false;
      GE_CHK_STATUS_RET(OnSubmodelParsed(has_next));
      if (has_next) {
        continue;
      }

      // all partitions were parsed
      if ((expected_offset_ != model_size_) || (!reader.IsEmpty())) {
        GELOGE(FAILED, "[Parse][Model] failed, not all model_data was consumed, received_size = %lu, "
                       "model_size = %lu, buffer_remaining_size = %lu",
               expected_offset_, model_size_, reader.GetSize());
        return FAILED;
      }

      GELOGD("[Parse][Model] success");
      completed_ = true;
      return SUCCESS;
    }
    GE_CHK_STATUS_RET(SetCurrentPartition(current_parse_state_.index + 1),
                      "Failed to advance to next partition");
  }
}

Status IncrementalModelParser::OnSubmodelParsed(bool &has_next) {
  GELOGI("Parsed model num = %zu, all model num = %u",
         root_model_->GetSubgraphInstanceNameToModel().size(),
         num_models_);
  root_model_->SetSubgraphInstanceNameToModel(model_->GetName(), model_);
  // first submodel is root_model
  if (root_model_->GetSubgraphInstanceNameToModel().size() == 1U) {
    root_model_->SetRootGraph(GraphUtils::GetComputeGraph(model_->GetGraph()));
  }
  if (root_model_->GetSubgraphInstanceNameToModel().size() < num_models_) {
    // more submodel to parse
    GE_CHK_STATUS_RET(ResetParseState(), "Failed to reset parse state");
    has_next = true;
  } else {
    has_next = false;
  }
  return SUCCESS;
}

Status IncrementalModelParser::SetCurrentPartition(size_t index) {
  const auto &partition_desc = partition_descs_[index];
  current_parse_state_.current_offset = 0U;
  current_parse_state_.partition_size = partition_desc.size;
  current_parse_state_.type = partition_desc.type;
  current_parse_state_.index = index;
  if (current_parse_state_.type == ParsePartitionType::kWeightData) {
    model_->SetWeight(Buffer(current_parse_state_.partition_size));
    current_parse_state_.buf = model_->GetWeight().GetData();
  } else {
    current_parse_state_.recv_buffer.reset(new(std::nothrow)uint8_t[current_parse_state_.partition_size]);
    current_parse_state_.buf = current_parse_state_.recv_buffer.get();
  }
  GELOGD("[Set][CurrentPartition] Set working partition, index = %zu, type = %d",
         index, static_cast<int32_t>(current_parse_state_.type));
  return SUCCESS;
}

Status IncrementalModelParser::ResetParseState() {
  partition_descs_.clear();
  ModelPartitionDesc partition{};
  partition.size = sizeof(ModelPartitionTable);
  partition.type = ParsePartitionType::kPartitionTableHeader;
  partition_descs_.emplace_back(partition);
  GE_CHK_STATUS_RET(SetCurrentPartition(0), "Failed to set current partition");
  model_ = MakeShared<GeModel>();
  GE_CHECK_NOTNULL(model_);
  return SUCCESS;
}

Status IncrementalModelParser::ParsePartitionTableNum() {
  const auto *partition_table = reinterpret_cast<const ModelPartitionTable *>(current_parse_state_.buf);
  ModelPartitionDesc partition{};
  partition.size = SizeOfModelPartitionTable(*partition_table) - sizeof(partition_table->num);
  partition.type = ParsePartitionType::kPartitionTable;
  partition_descs_.emplace_back(partition);
  num_partitions_ = partition_table->num;

  const uint32_t optional_num = 2U;
  if ((num_partitions_ != PARTITION_SIZE) &&
      (num_partitions_ != (PARTITION_SIZE - 1U)) &&
      (num_partitions_ != (PARTITION_SIZE - optional_num)) &&
      (num_partitions_ != 1U)) {
    GELOGE(PARAM_INVALID, "Invalid partition_table->num:%u", num_partitions_);
    return PARAM_INVALID;
  }

  GELOGD("Partition num = %u, size = %lu", partition_table->num, partition.size);
  return SUCCESS;
}

Status IncrementalModelParser::ParseCurrentPartition() {
  uint8_t *buf = current_parse_state_.buf;
  size_t size = current_parse_state_.partition_size;
  Status ret = SUCCESS;
  switch (current_parse_state_.type) {
    case ParsePartitionType::kPartitionTableHeader: {
      ret = ParsePartitionTableNum();
      break;
    }
    case ParsePartitionType::kPartitionTable: {
      ret = ParsePartitionTable();
      break;
    }
    case ParsePartitionType::kModelDef: {
      ge::Model model;
      GE_CHK_STATUS_RET(Model::Load(buf, size, model), "[Load][ModelDef] failed");
      ModelHelper::SetModelToGeModel(model_, model);
      GELOGD("[Load][ModelDef] success");
      break;
    }
    case ParsePartitionType::kWeightData: {
      GELOGD("[Load][Weight] success");  // need do nothing
      break;
    }
    case ParsePartitionType::kTaskInfo: {
      auto model_task_def = MakeShared<domi::ModelTaskDef>();
      GE_CHECK_NOTNULL(model_task_def);
      if (ReadProtoFromArray(buf, static_cast<int32_t>(size), model_task_def.get())) {
        model_->SetModelTaskDef(model_task_def);
        GELOGD("[Load][TaskInfo] success");
      } else {
        ret = FAILED;
        GELOGE(FAILED, "[Load][TaskInfo] failed");
      }
      break;
    }
    case ParsePartitionType::kTbeKernels: {
      TBEKernelStore tbe_kernel_store;
      if (tbe_kernel_store.Load(buf, size)) {
        model_->SetTBEKernelStore(tbe_kernel_store);
        GELOGD("[Load][TBEKernels] success");
      } else {
        ret = FAILED;
        GELOGE(FAILED, "[Load][TBEKernels] failed");
      }
      break;
    }
    case ParsePartitionType::kCustAiCpuKernels: {
      CustAICPUKernelStore aicpu_kernel_store;
      if (aicpu_kernel_store.Load(buf, size)) {
        model_->SetCustAICPUKernelStore(aicpu_kernel_store);
        GELOGD("[Load][CustomAiCpuKernels] success");
      } else {
        ret = FAILED;
        GELOGE(FAILED, "[Load][CustomAiCpuKernels] failed");
      }
      break;
    }
    default: {
      ret = UNSUPPORTED;
      GELOGE(UNSUPPORTED, "[Load][Partition] failed, unexpected partition type: %d",
             static_cast<int32_t>(current_parse_state_.type));
      break;
    }
  }
  return ret;
}

Status IncrementalModelParser::GetModel(GeRootModelPtr &model) const {
  if (!completed_) {
    GELOGE(FAILED, "Parse not completed, progress: %lu/%lu", expected_offset_, model_size_);
    return FAILED;
  }
  GE_CHECK_NOTNULL(root_model_);
  model = root_model_;
  return SUCCESS;
}

IncrementalModelParser::ParsePartitionType IncrementalModelParser::ToParsePartitionType(ModelPartitionType type) {
  ParsePartitionType ret;
  switch (type) {
    case ModelPartitionType::MODEL_DEF:
      ret = ParsePartitionType::kModelDef;
      break;
    case ModelPartitionType::WEIGHTS_DATA:
      ret = ParsePartitionType::kWeightData;
      break;
    case ModelPartitionType::TASK_INFO:
      ret = ParsePartitionType::kTaskInfo;
      break;
    case ModelPartitionType::TBE_KERNELS:
      ret = ParsePartitionType::kTbeKernels;
      break;
    case ModelPartitionType::CUST_AICPU_KERNELS:
      ret = ParsePartitionType::kCustAiCpuKernels;
      break;
    default:
      ret = ParsePartitionType::kInvalid;
      break;
  }
  return ret;
}

Status IncrementalModelParser::BufferReader::DrainTo(IncrementalModelParser::PartitionParseState &state,
                                                     uint64_t size) {
  GE_CHECK_LE(size, size_);
  void *dst_buffer = state.buf + state.current_offset;
  uint64_t dst_max = state.partition_size - state.current_offset;
  if (memcpy_s(dst_buffer, dst_max, buffer_, size) != EOK) {
    GELOGE(FAILED, "[Copy][Buffer] failed");
    return FAILED;
  }

  state.current_offset += size;
  AdvanceBy(size);
  return SUCCESS;
}

void IncrementalModelParser::BufferReader::AdvanceBy(uint64_t delta) {
  // checked by invoker
  buffer_ += delta;
  size_ -= delta;
}
}  // namespace ge
