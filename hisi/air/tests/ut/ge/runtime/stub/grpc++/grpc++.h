/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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

#ifndef __STUB_PROTO_H
#define __STUB_PROTO_H
#include <string>
#include <utility>
#include <stddef.h>
#include "proto/deployer.grpc.pb.h"

namespace grpc {

class Server {
 public:
  Server() = default;
  ~Server() = default;
  void Shutdown() {}
  void Wait() {}
};

class InsecureServerCredentials {
 public:
  InsecureServerCredentials() = default;
  ~InsecureServerCredentials() = default;
};

class ServerBuilder {
 public:
  ServerBuilder() = default;
  ~ServerBuilder() = default;
  void AddChannelArgument(const std::string &arg, int32_t value) {
    return;
  }

  void AddListeningPort(const std::string &addr, InsecureServerCredentials insecureServerCredentials) {
    return;
  }

  void RegisterService(deployer::DeployerService::Service *deployerService) {
    return;
  }

  void SetMaxReceiveMessageSize(int size) {
  }

  void SetMaxSendMessageSize(int size) {
  }

  std::unique_ptr<grpc::Server> BuildAndStart() {
    std::unique_ptr<grpc::Server> server = std::unique_ptr<grpc::Server>(new(std::nothrow) grpc::Server());
    return server;
  }
};
} // namespace grpc
#endif // __STUB_PROTO_H