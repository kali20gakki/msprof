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

#ifndef AIR_RUNTIME_DEPLOY_EXECUTOR_INCREMENTAL_MODEL_PARSER_H_
#define AIR_RUNTIME_DEPLOY_EXECUTOR_INCREMENTAL_MODEL_PARSER_H_

#include <vector>
#include <map>
#include <memory>
#include "ge/ge_api_error_codes.h"
#include "common/model/ge_model.h"
#include "graph/model.h"
#include "framework/common/types.h"
#include "framework/common/helper/om_file_helper.h"
#include "common/model/ge_root_model.h"

namespace ge {
class IncrementalModelParser {
 public:
  explicit IncrementalModelParser(uint64_t model_size);
  ~IncrementalModelParser() = default;
  Status Init(const void *buffer, size_t size);
  Status ParseAndDeserialize(uint64_t offset, const void *model_buffer, uint64_t buffer_size);
  Status GetModel(GeRootModelPtr &model) const;

 private:
  enum class ParsePartitionType {
    kModelDef,
    kWeightData,
    kTaskInfo,
    kTbeKernels,
    kCustAiCpuKernels,
    kPartitionTableHeader = 100,
    kPartitionTable,
    kInvalid,
  };

  Status CheckParams(uint64_t offset, const void *model_buffer, uint64_t buffer_size) const;
  Status SetCurrentPartition(size_t index);
  Status ParseCurrentPartition();
  Status ResetParseState();
  Status ParsePartitionTableNum();
  Status ParsePartitionTable();
  Status OnSubmodelParsed(bool &has_more);
  static ParsePartitionType ToParsePartitionType(ModelPartitionType type);

  struct ModelPartitionDesc {
    ParsePartitionType type;
    uint64_t offset;
    uint64_t size;
  };

  struct PartitionParseState {
    uint64_t current_offset = 0U;
    uint64_t partition_size = 0U;
    size_t index = 0U;
    ParsePartitionType type = ParsePartitionType::kInvalid;
    std::unique_ptr<uint8_t[]> recv_buffer;
    uint8_t *buf = nullptr;
  };

  class BufferReader {
   public:
    BufferReader(const void *buffer, uint64_t size) : buffer_(static_cast<const uint8_t *>(buffer)), size_(size) {
    }

    bool IsEmpty() const {
      return size_ == 0U;
    }

    uint64_t GetSize() const {
      return size_;
    }

    Status DrainTo(PartitionParseState &state, uint64_t size);

    void AdvanceBy(uint64_t delta);

   private:
    const uint8_t *buffer_;
    uint64_t size_;
  };

  std::vector<ModelPartitionDesc> partition_descs_;
  uint64_t model_size_ = 0U;
  uint64_t expected_offset_ = 0U;
  PartitionParseState current_parse_state_;
  bool completed_ = false;
  uint32_t num_models_ = 1U;
  GeModelPtr model_;
  GeRootModelPtr root_model_;
  uint32_t num_partitions_ = 0U;
};
}  // namespace ge

#endif  // AIR_RUNTIME_DEPLOY_EXECUTOR_INCREMENTAL_MODEL_PARSER_H_
