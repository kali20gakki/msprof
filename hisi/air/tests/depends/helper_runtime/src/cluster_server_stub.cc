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
#include "deploy/hcom/cluster/cluster_server.h"

namespace ge {
class ClusterServer::Impl {
};

ClusterServer::ClusterServer() = default;
ClusterServer::~ClusterServer() = default;
Status ClusterServer::StartClusterServer(const std::string &server_addr) {
  return SUCCESS;
}
Status ClusterServer::StopClusterServer() {
  return SUCCESS;
}
std::thread &ClusterServer::GetThread() {
  static std::thread t;
  return t;
}
}  // namespace geClusterServer