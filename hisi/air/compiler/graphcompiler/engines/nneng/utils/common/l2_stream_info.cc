/**
 * @file l2_stream_info.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 *
 * @brief Singleton of l2 stream info
 *
 * @version 1.0
 *
 */

#include "common/l2_stream_info.h"
#include "common/comm_log.h"

namespace fe {
StreamL2Info::StreamL2Info() {}
StreamL2Info::~StreamL2Info() {}

StreamL2Info& StreamL2Info::Instance() {
  static StreamL2Info stream_l2_info;
  return stream_l2_info;
}

Status StreamL2Info::GetStreamL2Info(const rtStream_t &stream_id, std::string node_name, TaskL2Info_t *&l2_data,
                                     std::string batch_label) {
  std::lock_guard<std::mutex> lock_guard(stream_l2_mutex_);
  CM_LOGD("Node[name=%s]: stream_id is %lu, stream_l2_map_ size is %zu.", node_name.c_str(),
          (uint64_t)(uintptr_t)stream_id, stream_l2_map_.size());
  const char *stream_id_char =  reinterpret_cast<const char*>(&stream_id);
  std::string stream_id_str = stream_id_char;
  std::string stream_l2_map_key = stream_id_str + batch_label;
  auto iter = stream_l2_map_.find(stream_l2_map_key);
  if (iter == stream_l2_map_.end()) {
    CM_LOGW("Node[name=%s]: streamiter find Fail, stream_id %lu, key is %s.",
            node_name.c_str(), (uint64_t)(uintptr_t)stream_id, stream_l2_map_key.c_str());
    return FAILED;
  }

  TaskL2InfoFEMap_t &l2_info_map = iter->second;
  auto opiter = l2_info_map.find(node_name);
  CM_LOGD("Node[name=%s]: l2_info_map size is %zu.", node_name.c_str(), l2_info_map.size());
  if (opiter == l2_info_map.end()) {
    CM_LOGW("Node[name=%s]: can not find the node from l2_info_map.", node_name.c_str());
    return FAILED;
  }

  l2_data = &(opiter->second);

  return SUCCESS;
}

Status StreamL2Info::SetStreamL2Info(const rtStream_t &stream_id, TaskL2InfoFEMap_t &l2_alloc_res,
                                     std::string batch_label) {
  std::lock_guard<std::mutex> lock_guard(stream_l2_mutex_);
  CM_CHECK(stream_id == nullptr,
           REPORT_CM_ERROR("[StreamOpt][L2Opt][SetStreamL2Info] stream_id is nullptr."), return fe::FAILED);
  const char *stream_id_char =  reinterpret_cast<const char*>(&stream_id);
  std::string stream_id_str = stream_id_char;
  std::string stream_l2_map_key = stream_id_str + batch_label;
  const auto it = stream_l2_map_.find(stream_l2_map_key);
  if (it != stream_l2_map_.end()) {
    CM_LOGD("SetStreamL2InfoMap steam_map has been set, key is %s.", stream_l2_map_key.c_str());
    it->second = l2_alloc_res;
    return fe::SUCCESS;
  }

  for (TaskL2InfoFEMap_t::iterator iter = l2_alloc_res.begin(); iter != l2_alloc_res.end(); ++iter) {
    CM_LOGD("SetStreamL2InfoMap node name is %s.", iter->first.c_str());
    TaskL2Info_t *l2task = &(iter->second);
    l2task->isUsed = 0;
  }
  CM_LOGD("l2_alloc_res size is %zu.", l2_alloc_res.size());
  stream_l2_map_[stream_l2_map_key] = l2_alloc_res;
  CM_LOGD("SetStreamL2InfoMap key is %s.", stream_l2_map_key.c_str());
  return SUCCESS;
}
}  // namespace fe
