/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#include "fusion_statistic_writer.h"
#include <fcntl.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <climits>
#include <cstdlib>
#include <set>
#include <iostream>

namespace fe {
static const std::string FUSION_INFO_FILENAME = "fusion_result.json";
static const std::string SESSION_AND_GRAPH_ID = "session_and_graph_id";
static const std::string GRAPH_FUSION_LOG_TYPE = "graph_fusion";
static const std::string UB_FUSION_LOG_TYPE = "ub_fusion";
static const std::string MATCH_TIMES_LOG = "match_times";
static const std::string EFFECT_TIMES_LOG = "effect_times";

FusionStatisticWriter::FusionStatisticWriter(){};

FusionStatisticWriter::~FusionStatisticWriter(){};

FusionStatisticWriter &FusionStatisticWriter::Instance() {
  static FusionStatisticWriter fusion_statistic_writer;
  return fusion_statistic_writer;
}

void FusionStatisticWriter::SaveFusionTimesToJson(const std::map<std::string, FusionInfo> &fusion_info_map,
                                                  const string &graph_type, json &js) const {
  for (auto &graph_iter : fusion_info_map) {
    json js_object_pass;
    FusionInfo fusion_info = graph_iter.second;
    js_object_pass[MATCH_TIMES_LOG] = std::to_string(fusion_info.GetMatchTimes());
    js_object_pass[EFFECT_TIMES_LOG] = std::to_string(fusion_info.GetEffectTimes());
    js[graph_type][graph_iter.first] = js_object_pass;
  }
}

string FusionStatisticWriter::RealPath(const string &file_name) const {
  char resolved_path[PATH_MAX] = {0};
  if (realpath(file_name.c_str(), resolved_path) == nullptr) {
    FE_LOGD("file %s does not exist, it will be created.", file_name.c_str());
  }
  FE_LOGD("file and real_path %s", resolved_path);
  return resolved_path;
}

void FusionStatisticWriter::Finalize() {
  FE_LOGD("FusionStatisticWriter start to finalize!");
  WriteAllFusionInfoToJsonFile();
  std::lock_guard<std::mutex> lock_guard(file_mutex_);
  if (file_.is_open()) {
    file_.close();
  }
}

void FusionStatisticWriter::WriteJsonFile(const std::string &file_name,
                                          const std::map<std::string, FusionInfo> &graph_fusion_info_map,
                                          const std::map<std::string, FusionInfo> &buffer_fusion_info_map,
                                          const string &session_and_graph_id) {
  std::lock_guard<std::mutex> lock_guard(file_mutex_);
  string file_path = RealPath(file_name);
  // Check file
  if (file_path.empty()) {
    FE_LOGW("The file path [%s] is invalid.", file_path.c_str());
    return;
  }
  const int FILE_AUTHORITY = 0640;
  int fd = open(file_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, FILE_AUTHORITY);
  if (fd < 0) {
    FE_LOGW("fail to open the file: %s", file_path.c_str());
    return;
  }
  if (close(fd) != 0) {
    FE_LOGW("Fail to close the file: %s.", file_path.c_str());
    return;
  }
  // Open file
  file_.open(file_path, std::ios::out | std::ios::app);
  if (!file_.is_open()) {
    FE_LOGW("Failed to open this file %s .", file_path.c_str());
    return;
  }
  json js;
  js[SESSION_AND_GRAPH_ID] = session_and_graph_id;
  SaveFusionTimesToJson(graph_fusion_info_map, GRAPH_FUSION_LOG_TYPE, js);
  SaveFusionTimesToJson(buffer_fusion_info_map, UB_FUSION_LOG_TYPE, js);
  file_ << js.dump(4);
  js.clear();
  file_.close();
}

void FusionStatisticWriter::WriteFusionInfoToJsonFile(const std::string &file_name,
                                                      const std::string &session_and_graph_id) {
  // save match & effect file
  std::map<std::string, FusionInfo> graph_fusion_info_map;
  std::map<std::string, FusionInfo> buffer_fusion_info_map;
  FusionStatisticRecorder::Instance().GetAndClearFusionInfo(session_and_graph_id,
                                                            graph_fusion_info_map, buffer_fusion_info_map);
  FE_LOGD("Start to save graph fusion and ub fusion (session_and_graph_id: %s) to file.", session_and_graph_id.c_str());
  if (!graph_fusion_info_map.empty() || !buffer_fusion_info_map.empty()) {
    WriteJsonFile(file_name, graph_fusion_info_map, buffer_fusion_info_map, session_and_graph_id);
    FE_LOGD("Finish saving the file[file name: %s] about match & effect times.", file_name.c_str());
  }
}

void FusionStatisticWriter::WriteAllFusionInfoToJsonFile() {
  std::vector<std::string> session_graph_id_vec;
  FusionStatisticRecorder::Instance().GetAllSessionAndGraphIdList(session_graph_id_vec);
  if (session_graph_id_vec.empty()) {
    FE_LOGD("The statistic of graph fusion and buffer fusion is empty.");
    return;
  }
  for (std::string &session_graph_id : session_graph_id_vec) {
    WriteFusionInfoToJsonFile(FUSION_INFO_FILENAME, session_graph_id);
  }
}

void FusionStatisticWriter::ClearHistoryFile() {
  FE_LOGD("Analyzer start to clear history file!");
  std::lock_guard<std::mutex> lock_guard(file_mutex_);
  // Remove history files
  int res = remove(FUSION_INFO_FILENAME.c_str());
  FE_LOGD("remove file %s, result:%d", FUSION_INFO_FILENAME.c_str(), res);
}
}
