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

#ifndef GE_GRAPH_LOAD_NEW_MODEL_MANAGER_AIPP_UTILS_H_
#define GE_GRAPH_LOAD_NEW_MODEL_MANAGER_AIPP_UTILS_H_

#include <map>

#include "framework/common/ge_inner_error_codes.h"
#include "framework/common/ge_types.h"
#include "graph/op_desc.h"
#include "graph/utils/attr_utils.h"
#include "proto/insert_op.pb.h"

namespace ge {
constexpr uint32_t kAippOriginInputIndex = 0U;
constexpr uint32_t kAippInfoNum = 6U;
constexpr uint32_t kAippInfoFormat = 0U;
constexpr uint32_t kAippInfoDataType = 1U;
constexpr uint32_t kAippInfoTensorName = 2U;
constexpr uint32_t kAippInfoTensorSize = 3U;
constexpr uint32_t kAippInfoDimNum = 4U;
constexpr uint32_t kAippInfoShape = 5U;

class AippUtils {
 public:
  AippUtils() = default;
  ~AippUtils() = default;

  static Status ConvertAippParams2AippInfo(const domi::AippOpParams &aipp_params, AippConfigInfo &aipp_info);
  static Status SetAippInfoAndTypeFromOpDesc(const std::map<std::string, uint32_t> &data_index_map,
                                             const OpDescPtr &op_desc, const uint32_t index,
                                             std::map<uint32_t, AippConfigInfo> &aipp_infos,
                                             std::map<uint32_t, std::pair<InputAippType, size_t>> &aipp_types);
  static Status GetAippInfo(const std::map<uint32_t, AippConfigInfo> &aipp_infos, const uint32_t index,
                            AippConfigInfo &aipp_info);
  static Status GetAippType(const std::map<uint32_t, std::pair<InputAippType, size_t>> &aipp_types,
                            const uint32_t index, InputAippType &aipp_type, size_t &aipp_data_index);

private:
  static Status SetAippInfoImpl(const NamedAttrs &aipp_attr,
                                const OpDescPtr &op_desc, const uint32_t index,
                                std::map<uint32_t, AippConfigInfo> &aipp_infos);
  static Status SetAippTypeImpl(const std::map<std::string, uint32_t> &data_index_map,
                                const std::string &data_mode, const OpDescPtr &op_desc, const uint32_t index,
                                std::map<uint32_t, std::pair<InputAippType, size_t>> &aipp_types);
  static void SetMatrixInfo(const domi::AippOpParams &aipp_params, AippConfigInfo &aipp_info);
  static void SetBiasInfo(const domi::AippOpParams &aipp_params, AippConfigInfo &aipp_info);
  static void SetChnInfo(const domi::AippOpParams &aipp_params, AippConfigInfo &aipp_info);
};
}  // namespace ge

#endif  // GE_GRAPH_LOAD_NEW_MODEL_MANAGER_AIPP_UTILS_H_
