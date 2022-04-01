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

#include "hybrid/executor/hybrid_data_flow.h"

namespace ge {
namespace hybrid {
DataFlowResource::DataFlowResource(const int64_t max_size) : max_size_(max_size) {}

bool DataFlowResource::IsMaxSizeConst() const {
  const std::lock_guard<std::mutex> lock(data_flow_res_mutex_);
  return max_size_const_;
}

bool DataFlowResource::IsClosed() const {
  const std::lock_guard<std::mutex> lock(data_flow_res_mutex_);
  return is_closed_;
}

bool DataFlowResource::Empty() const {
  const std::lock_guard<std::mutex> lock(data_flow_res_mutex_);
  return data_.empty();
}

void DataFlowResource::SetMaxSize(const int64_t max_size) {
  const std::lock_guard<std::mutex> lock(data_flow_res_mutex_);
  max_size_ = max_size;
}

void DataFlowResource::SetMaxSizeConst(const bool max_size_const) {
  const std::lock_guard<std::mutex> lock(data_flow_res_mutex_);
  max_size_const_ = max_size_const;
}

void DataFlowResource::SetClosed(const bool is_closed) {
  const std::lock_guard<std::mutex> lock(data_flow_res_mutex_);
  is_closed_ = is_closed;
}

Status DataFlowResource::EmplaceBack(DataFlowTensor &&value) {
  const std::lock_guard<std::mutex> lock(data_flow_res_mutex_);
  if (static_cast<int64_t>(data_.size()) >= max_size_) {
    GELOGE(INTERNAL_ERROR, "The capacity has reached the maximum, max_size:%ld.", max_size_);
    return INTERNAL_ERROR;
  }
  data_.emplace_back(value);
  return SUCCESS;
}

void DataFlowResource::PopBack() {
  const std::lock_guard<std::mutex> lock(data_flow_res_mutex_);
  data_.pop_back();
}

const DataFlowTensor &DataFlowResource::Back() {
  const std::lock_guard<std::mutex> lock(data_flow_res_mutex_);
  return data_.back();
}

void DataFlowResource::Clear() {
  const std::lock_guard<std::mutex> lock(data_flow_res_mutex_);
  data_.clear();
}
}  // namespace hybrid
}  // namespace ge
