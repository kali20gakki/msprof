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

#include "framework/common/helper/om_file_helper.h"

#include <string>
#include <vector>

#include "common/auth/file_saver.h"
#include "common/math/math_util.h"
#include "framework/common/debug/ge_log.h"
#include "framework/common/debug/log.h"
#include "framework/common/ge_inner_error_codes.h"
#include "framework/common/util.h"
#include "graph/def_types.h"

namespace {
const uint32_t kOptionalNum = 2U;
}
namespace ge {
// For Load
Status OmFileLoadHelper::Init(const ge::ModelData &model) {
  if (CheckModelValid(model) != SUCCESS) {
    return FAILED;
  }
  const uint32_t model_data_size = model.model_len - sizeof(ModelFileHeader);
  uint8_t
      *const model_data = PtrToPtr<void, uint8_t>(ValueToPtr(PtrToValue(model.model_data) + sizeof(ModelFileHeader)));
  const Status ret = Init(model_data, model_data_size);
  return ret;
}

Status OmFileLoadHelper::Init(uint8_t* const model_data, const uint32_t model_data_size) {
  const Status status = LoadModelPartitionTable(model_data, model_data_size);
  if (status != SUCCESS) {
    return status;
  }
  is_inited_ = true;
  return SUCCESS;
}

Status OmFileLoadHelper::Init(uint8_t* const model_data, const uint32_t model_data_size, const uint32_t model_num) {
  const Status status = LoadModelPartitionTable(model_data, model_data_size, model_num);
  if (status != SUCCESS) {
    return status;
  }
  is_inited_ = true;
  return SUCCESS;
}

// Use both
Status OmFileLoadHelper::GetModelPartition(const ModelPartitionType type, ModelPartition &partition) {
  if (!is_inited_) {
    GELOGE(PARAM_INVALID, "OmFileLoadHelper has not been initialized!");
    return PARAM_INVALID;
  }

  bool found = false;
  for (ModelPartition &part : context_.partition_datas_) {
    if (part.type == type) {
      partition = part;
      found = true;
      break;
    }
  }

  if (!found) {
    if ((type != ModelPartitionType::TBE_KERNELS) && (type != ModelPartitionType::WEIGHTS_DATA) &&
        (type != ModelPartitionType::CUST_AICPU_KERNELS)) {
      GELOGE(FAILED, "GetModelPartition:type:%d is not in partition_datas!", static_cast<int32_t>(type));
      return FAILED;
    }
  }
  return SUCCESS;
}

Status OmFileLoadHelper::GetModelPartition(const ModelPartitionType type,
                                           ModelPartition &partition, const size_t model_index) {
  if (!is_inited_) {
    GELOGE(PARAM_INVALID, "OmFileLoadHelper has not been initialized!");
    return PARAM_INVALID;
  }
  if (model_index >= model_contexts_.size()) {
    GELOGE(PARAM_INVALID, "cur index : %zu, model_contexts size:%zu", model_index, model_contexts_.size());
    return PARAM_INVALID;
  }
  auto &cur_ctx = model_contexts_[model_index];
  bool found = false;
  for (ModelPartition &part : cur_ctx.partition_datas_) {
    if (part.type == type) {
      partition = part;
      found = true;
      break;
    }
  }

  if (!found) {
    if ((type != ModelPartitionType::TBE_KERNELS) && (type != ModelPartitionType::WEIGHTS_DATA) &&
        (type != ModelPartitionType::CUST_AICPU_KERNELS)) {
      GELOGE(FAILED, "GetModelPartition:type:%d is not in partition_datas!", static_cast<int32_t>(type));
      return FAILED;
    }
  }
  return SUCCESS;
}

Status OmFileLoadHelper::CheckModelValid(const ge::ModelData &model) const {
  // Parameter validity check
  if (model.model_data == nullptr) {
    GELOGE(PARAM_INVALID, "Model_data must not be null!");
    return PARAM_INVALID;
  }

  // Model length too small
  if (model.model_len < (sizeof(ModelFileHeader) + sizeof(ModelPartitionTable))) {
    GELOGE(PARAM_INVALID,
        "Invalid model. length[%u] < sizeof(ModelFileHeader)[%zu] + sizeof(ModelPartitionTable)[%zu].",
        model.model_len, sizeof(ModelFileHeader), sizeof(ModelPartitionTable));
    return PARAM_INVALID;
  }

  // Get file header
  auto const model_header = reinterpret_cast<ModelFileHeader *>(model.model_data);
  // Determine whether the file length and magic number match
  if ((model_header->length != (model.model_len - sizeof(ModelFileHeader))) ||
      (MODEL_FILE_MAGIC_NUM != model_header->magic)) {
    GELOGE(PARAM_INVALID,
           "Invalid model. file_header->length[%u] + sizeof(ModelFileHeader)[%zu] != model->model_len[%u] || "
            "MODEL_FILE_MAGIC_NUM[%u] != file_header->magic[%u]",
           model_header->length, sizeof(ModelFileHeader), model.model_len, MODEL_FILE_MAGIC_NUM, model_header->magic);
    return PARAM_INVALID;
  }
  return SUCCESS;
}

Status OmFileLoadHelper::LoadModelPartitionTable(uint8_t* const model_data, const uint32_t model_data_size) {
  if (model_data == nullptr) {
    GELOGE(ACL_ERROR_GE_EXEC_MODEL_ADDR_INVALID, "Param model_data must not be null!");
    return ACL_ERROR_GE_EXEC_MODEL_ADDR_INVALID;
  }
  // Init partition table
  auto const partition_table = reinterpret_cast<ModelPartitionTable *>(model_data);
  // Davinici model partition include graph-info  weight-info  task-info  tbe-kernel :
  // Original model partition include graph-info
  if ((partition_table->num != PARTITION_SIZE) && (partition_table->num != (PARTITION_SIZE - 1U)) &&
      (partition_table->num != (PARTITION_SIZE - kOptionalNum)) && (partition_table->num != 1U)) {
    GELOGE(ACL_ERROR_GE_PARAM_INVALID, "Invalid partition_table->num:%u", partition_table->num);
    return ACL_ERROR_GE_PARAM_INVALID;
  }
  size_t mem_offset = (SizeOfModelPartitionTable(*partition_table));
  GELOGD("ModelPartitionTable num:%u, ModelFileHeader length:%zu, ModelPartitionTable length:%zu",
         partition_table->num, sizeof(ModelFileHeader), mem_offset);
  if (model_data_size <= mem_offset) {
    GELOGE(ACL_ERROR_GE_EXEC_MODEL_DATA_SIZE_INVALID, "invalid model data, partition_table->num:%u, model data size %u",
           partition_table->num, model_data_size);
    return ACL_ERROR_GE_EXEC_MODEL_DATA_SIZE_INVALID;
  }
  for (uint32_t i = 0U; i < partition_table->num; i++) {
    ModelPartition partition;
    partition.size = static_cast<uint32_t>(partition_table->partition[i].mem_size);
    partition.data = PtrToPtr<void, uint8_t>(ValueToPtr(PtrToValue(model_data) + mem_offset));
    partition.type = partition_table->partition[i].type;
    context_.partition_datas_.push_back(partition);

    if ((partition.size > model_data_size) || (mem_offset > static_cast<size_t>(model_data_size - partition.size))) {
      GELOGE(ACL_ERROR_GE_EXEC_MODEL_DATA_SIZE_INVALID,
             "The partition size %zu is greater than the model data size %u.",
             static_cast<size_t>(partition.size + mem_offset), model_data_size);
      return ACL_ERROR_GE_EXEC_MODEL_DATA_SIZE_INVALID;
    }
    mem_offset += partition.size;
    GELOGD("Partition, type:%d, size:%u", static_cast<int32_t>(partition.type), partition.size);
  }
  return SUCCESS;
}

Status OmFileLoadHelper::LoadModelPartitionTable(uint8_t* const model_data, const uint32_t model_data_size,
                                                 const uint32_t model_num) {
  if (model_data == nullptr) {
    GELOGE(PARAM_INVALID, "Param model_data must not be null!");
    return PARAM_INVALID;
  }

  uint32_t cur_offset = 0U;
  for (size_t index = 0U; index < static_cast<size_t>(model_num); ++index) {
    // Init partition table
    const auto partition_table = reinterpret_cast<ModelPartitionTable *>(PtrToPtr<void, uint8_t>(ValueToPtr(
        PtrToValue(model_data) + static_cast<size_t>(cur_offset))));
    const size_t partition_table_size = (SizeOfModelPartitionTable(*partition_table));
    cur_offset += partition_table_size;
    GELOGD("Cur model index %zu: ModelPartitionTable num :%u, "
           "ModelFileHeader length :%zu, ModelPartitionTable length :%zu",
           index, partition_table->num, sizeof(ModelFileHeader), partition_table_size);
    if (model_data_size <= cur_offset) {
      GELOGE(ACL_ERROR_GE_EXEC_MODEL_DATA_SIZE_INVALID,
             "invalid model data, partition_table->num:%u, model data size %u",
             partition_table->num, model_data_size);
      return ACL_ERROR_GE_EXEC_MODEL_DATA_SIZE_INVALID;
    }

    for (uint32_t i = 0U; i < partition_table->num; i++) {
      ModelPartition partition;
      partition.size = partition_table->partition[i].mem_size;
      partition.data = PtrToPtr<void, uint8_t>(ValueToPtr(PtrToValue(model_data) + cur_offset));
      partition.type = partition_table->partition[i].type;
      if (index >= model_contexts_.size()) {
        if (index != model_contexts_.size()) {
          GELOGE(FAILED, "cur index is %zu make model_contexts_ overflow", index);
          return FAILED;
        }

        OmFileContext tmp_ctx;
        tmp_ctx.partition_datas_.push_back(partition);
        model_contexts_.push_back(tmp_ctx);
      } else {
        model_contexts_[index].partition_datas_.push_back(partition);
      }

      if ((partition.size > model_data_size) || (cur_offset > (model_data_size - partition.size))) {
        GELOGE(ACL_ERROR_GE_EXEC_MODEL_DATA_SIZE_INVALID,
               "The partition size %u is greater than the model data size %u.",
               partition.size + cur_offset, model_data_size);
        return ACL_ERROR_GE_EXEC_MODEL_DATA_SIZE_INVALID;
      }
      cur_offset += partition.size;
      GELOGD("Partition, type:%d, size:%u, model_index:%zu",
             static_cast<int32_t>(partition.type), partition.size, index);
    }
  }
  if (cur_offset != model_data_size) {
    GELOGE(FAILED, "do not get the complete model, read end offset:%u, all size:%u", cur_offset, model_data_size);
    return FAILED;
  }
  return SUCCESS;
}

ModelPartitionTable *OmFileSaveHelper::GetPartitionTable() {
  const auto partition_size = static_cast<uint32_t>(context_.partition_datas_.size());
  // Build ModelPartitionTable, flex array
  context_.partition_table_.clear();
  context_.partition_table_.resize(sizeof(ModelPartitionTable) + (sizeof(ModelPartitionMemInfo) * partition_size),
                                   static_cast<char_t>(0));

  auto const partition_table = PtrToPtr<char_t, ModelPartitionTable>(context_.partition_table_.data());
  partition_table->num = partition_size;

  uint32_t mem_offset = 0U;
  for (size_t i = 0U; i < static_cast<size_t>(partition_size); i++) {
    const ModelPartition partition = context_.partition_datas_[i];
    partition_table->partition[i] = {partition.type, mem_offset, partition.size};
    mem_offset += partition.size;
    GELOGD("Partition, type:%d, size:%u", static_cast<int32_t>(partition.type), partition.size);
  }
  return partition_table;
}

ModelPartitionTable *OmFileSaveHelper::GetPartitionTable(const size_t cur_ctx_index) {
  auto &cur_ctx = model_contexts_[cur_ctx_index];
  const auto partition_size = static_cast<uint32_t>(cur_ctx.partition_datas_.size());
  // Build ModelPartitionTable, flex array
  cur_ctx.partition_table_.clear();
  cur_ctx.partition_table_.resize(sizeof(ModelPartitionTable) + (sizeof(ModelPartitionMemInfo) * partition_size),
                                  static_cast<char_t>(0));

  auto const partition_table = PtrToPtr<char_t, ModelPartitionTable>(cur_ctx.partition_table_.data());
  partition_table->num = partition_size;

  uint32_t mem_offset = 0U;
  for (size_t i = 0U; i < static_cast<size_t>(partition_size); i++) {
    const ModelPartition partition = cur_ctx.partition_datas_[i];
    partition_table->partition[i] = {partition.type, mem_offset, partition.size};
    mem_offset += partition.size;
    GELOGD("Partition, type:%d, size:%u", static_cast<int32_t>(partition.type), partition.size);
  }
  return partition_table;
}

Status OmFileSaveHelper::AddPartition(const ModelPartition &partition) {
  if (ge::CheckUint32AddOverflow(context_.model_data_len_, partition.size) != SUCCESS) {
    GELOGE(FAILED, "UINT32 %u and %u addition can result in overflow!", context_.model_data_len_, partition.size);
    return FAILED;
  }
  context_.partition_datas_.push_back(partition);
  context_.model_data_len_ += partition.size;
  return SUCCESS;
}

Status OmFileSaveHelper::AddPartition(const ModelPartition &partition, const size_t cur_index) {
  if (ge::CheckUint32AddOverflow(context_.model_data_len_, partition.size) != SUCCESS) {
    GELOGE(FAILED, "UINT32 %u and %u addition can result in overflow!", context_.model_data_len_, partition.size);
    return FAILED;
  }
  if (cur_index >= model_contexts_.size()) {
    if (cur_index != model_contexts_.size()) {
      GELOGE(FAILED, "cur index is %zu make model_contexts_ overflow", cur_index);
      return FAILED;
    }
    OmFileContext tmp_ctx;
    tmp_ctx.model_data_len_ += partition.size;
    tmp_ctx.partition_datas_.push_back(partition);
    model_contexts_.push_back(tmp_ctx);
  } else {
    model_contexts_[cur_index].model_data_len_ += partition.size;
    model_contexts_[cur_index].partition_datas_.push_back(partition);
  }
  return SUCCESS;
}

Status OmFileSaveHelper::SaveModel(const SaveParam &save_param, const char_t* const output_file, ModelBufferData &model,
                                   const bool is_offline) {
  (void)save_param.cert_file;
  (void)save_param.ek_file;
  (void)save_param.encode_mode;
  (void)save_param.hw_key_file;
  (void)save_param.pri_key_file;
  const Status ret = SaveModelToFile(output_file, model, is_offline);
  if (ret == SUCCESS) {
    GELOGD("Generate model with encrypt.");
  }
  return ret;
}

Status OmFileSaveHelper::SaveModelToFile(const char_t* const output_file, ModelBufferData &model,
                                         const bool is_offline) {
  if (context_.partition_datas_.empty()) {
    GE_CHK_BOOL_EXEC(!model_contexts_.empty(), return FAILED, "mode contexts empty");
    context_ = model_contexts_.front();
  }
  const uint32_t model_data_len = context_.model_data_len_;
  if (model_data_len == 0U) {
    GELOGE(domi::PARAM_INVALID, "Model data len error! should not be 0");
    return domi::PARAM_INVALID;
  }

  ModelPartitionTable * const partition_table = GetPartitionTable();
  if (partition_table == nullptr) {
    GELOGE(ge::GE_GRAPH_SAVE_FAILED, "SaveModelToFile execute failed: partition_table is NULL.");
    return ge::GE_GRAPH_SAVE_FAILED;
  }
  const uint32_t size_of_table = (SizeOfModelPartitionTable(*partition_table));
  FMK_UINT32_ADDCHECK(size_of_table, model_data_len)
  model_header_.length = size_of_table + model_data_len;

  GELOGD("Sizeof(ModelFileHeader):%zu,sizeof(ModelPartitionTable):%u, model_data_len:%u, model_total_len:%zu",
         sizeof(ModelFileHeader), size_of_table, model_data_len,
         static_cast<size_t>(model_header_.length + sizeof(ModelFileHeader)));

  const std::vector<ModelPartition> partition_datas = context_.partition_datas_;
  Status ret;
  if (is_offline) {
    ret = FileSaver::SaveToFile(output_file, model_header_, *partition_table, partition_datas);
  } else {
    ret = FileSaver::SaveToBuffWithFileHeader(model_header_, *partition_table, partition_datas, model);
  }
  if (ret == SUCCESS) {
    GELOGD("Save model success without encrypt.");
  }
  return ret;
}

Status OmFileSaveHelper::SaveRootModel(const SaveParam &save_param, const char_t *const output_file,
                                       ModelBufferData &model, const bool is_offline) {
  (void)save_param.cert_file;
  (void)save_param.ek_file;
  (void)save_param.encode_mode;
  (void)save_param.hw_key_file;
  (void)save_param.pri_key_file;

  std::vector<ModelPartitionTable *> model_partition_tabels;
  std::vector<std::vector<ModelPartition>> all_model_partitions;
  for (size_t ctx_index = 0U; ctx_index < model_contexts_.size(); ++ctx_index) {
    auto &cur_ctx = model_contexts_[ctx_index];
    const uint32_t cur_model_data_len = cur_ctx.model_data_len_;
    if (cur_model_data_len == 0U) {
      GELOGE(domi::PARAM_INVALID, "Model data len error! should not be 0");
      return domi::PARAM_INVALID;
    }

    ModelPartitionTable * const tmp_table = GetPartitionTable(ctx_index);
    if (tmp_table == nullptr) {
      GELOGE(ge::GE_GRAPH_SAVE_FAILED, "SaveModelToFile execute failed: partition_table is NULL.");
      return ge::GE_GRAPH_SAVE_FAILED;
    }
    const uint32_t size_of_table = (SizeOfModelPartitionTable(*tmp_table));
    FMK_UINT32_ADDCHECK(size_of_table, cur_model_data_len)
    FMK_UINT32_ADDCHECK(size_of_table + cur_model_data_len, model_header_.length)
    model_header_.length += size_of_table + cur_model_data_len;
    model_partition_tabels.push_back(tmp_table);
    all_model_partitions.push_back(cur_ctx.partition_datas_);
    GELOGD("sizeof(ModelPartitionTable):%u, cur_model_data_len:%u, cur_context_index:%zu", size_of_table,
           cur_model_data_len, ctx_index);
  }
  Status ret;
  if (is_offline) {
    ret = FileSaver::SaveToFile(output_file, model_header_, model_partition_tabels, all_model_partitions);
  } else {
    ret = FileSaver::SaveToBuffWithFileHeader(model_header_, model_partition_tabels, all_model_partitions, model);
  }
  if (ret == SUCCESS) {
    GELOGD("Save model success without encrypt.");
  }
  return ret;
}
}  // namespace ge
