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

#include "graph_stage_cache.h"
#include "graph/ge_tensor.h"
#include "graph/utils/tensor_utils.h"
#include "common/debug/ge_log.h"
#include "hybrid/model/node_item.h"
#include "hybrid/model/infer/tensor_desc_holder.h"
#include "hybrid/model/infer/shape_utils.h"
#include "hybrid/model/infer/node_shape_infer.h"
#include "common/plugin/ge_util.h"

namespace ge {
namespace hybrid {

struct CacheTensorDescObserver : ChangeObserver, private GeTensorDesc {
  explicit CacheTensorDescObserver(const TensorDescHolder &dest)
      : ChangeObserver(), GeTensorDesc(), dest_(dest), is_changed_(false) {}
  Status DoPropagate() {
    if (is_changed_.load()) {
      GE_CHK_STATUS_RET_NOLOG(ShapeUtils::CopyShapeAndTensorSize(*this, dest_.Input()));
      is_changed_.store(false);
    }
    return SUCCESS;
  }

  GeTensorDesc &GetCacheTensorDesc() {
    return *this;
  }

private:
  virtual void Changed() override {
    is_changed_.store(true);
  };

private:
  TensorDescHolder dest_;
  std::atomic<bool> is_changed_;
};

namespace {
bool DoNotNeedShapePropagate(const NodeItem &next_node, const int32_t input_index) {
  if (next_node.IsInputShapeStatic(input_index)) {
    return true;
  }
  const auto &input_desc = next_node.MutableInputDesc(input_index);
  return input_desc == nullptr;
}
} // namespace

Status GraphStageCache::CreatePropagator(NodeItem &cur_node,
                                         const int32_t output_index,
                                         NodeItem &next_node,
                                         const int32_t input_index) {
  if (DoNotNeedShapePropagate(next_node, input_index)) {
    return SUCCESS;
  }

  const TensorDescHolder src_tensor_desc(cur_node, output_index);
  const TensorDescHolder dest_tensor_desc(next_node, input_index);
  if ((cur_node.group == next_node.group) || (next_node.group < 0)) {
    cur_node.CreatePropagator(src_tensor_desc, TensorDescObserver(dest_tensor_desc));
  } else {
    const auto stage_id = next_node.group;
    const auto cache_observer = ge::MakeShared<CacheTensorDescObserver>(dest_tensor_desc);
    GE_CHECK_NOTNULL(cache_observer);
    const auto observer = GetOrCreate(stage_id).CreateTensorDescObserver(cache_observer);
    cur_node.CreatePropagator(src_tensor_desc, observer);
    GELOGD("Create stage propagator for [%s] stage id[%d] ", next_node.NodeName().c_str(), stage_id);
  }
  return SUCCESS;
}

GraphStageCache::StageCache::~StageCache() {}

GraphStageCache::StageCache &GraphStageCache::GetOrCreate(const int32_t stage) {
  return stage_caches_[stage];
}

TensorDescObserver GraphStageCache::StageCache::CreateTensorDescObserver(
    const std::shared_ptr<CacheTensorDescObserver> &observer) {
  tensor_desc_observers_.push_back(observer);
  return TensorDescObserver(observer->GetCacheTensorDesc(), *observer);
}

Status GraphStageCache::StageCache::DoPropagate() {
  const std::lock_guard<std::mutex> lk(sync_);
  for (const auto &server : tensor_desc_observers_) {
    GE_CHK_STATUS_RET_NOLOG(server->DoPropagate());
  }
  return SUCCESS;
}

Status GraphStageCache::DoPropagate(const int32_t stage) {
  if (stage < 0) {
    return SUCCESS;
  }
  return GetOrCreate(stage).DoPropagate();
}
} // namespace hybrid
} // namespace ge
