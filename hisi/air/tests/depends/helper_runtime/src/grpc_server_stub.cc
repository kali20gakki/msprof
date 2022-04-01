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

#include "daemon/grpc_server.h"
#include <condition_variable>
#include <mutex>
#include "common/plugin/ge_util.h"
#include "framework/common/debug/log.h"
#include "daemon/deployer_service_impl.h"

namespace ge {
class GrpcServer::Impl {
 public:
  bool running_ = true;
  std::mutex mu_;
  std::condition_variable cv_;
};

GrpcServer::GrpcServer() = default;
GrpcServer::~GrpcServer() = default;

Status GrpcServer::Run() {
  impl_ = MakeUnique<GrpcServer::Impl>();
  GE_CHECK_NOTNULL(impl_);
  std::unique_lock<std::mutex> lk(impl_->mu_);
  impl_->cv_.wait(lk, [&] () { return !impl_->running_; });
  return SUCCESS;
}

void GrpcServer::Finalize() {
  std::unique_lock<std::mutex> lk(impl_->mu_);
  impl_->running_ = false;
  impl_->cv_.notify_all();
}
} // namespace ge
