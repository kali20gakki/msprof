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

#ifndef AIR_CXX_STATUS_H
#define AIR_CXX_STATUS_H
namespace grpc {
enum StatusCode {
  OK = 0,
  CANCELLED = 1
};

class Status {
 public:
  Status() 
    : code_(StatusCode::OK) {}
  bool ok() {
    return true;
  }

  int error_code() {
    return 0;
  }

  std::string error_message() {
    return "ok";
  }

  static const Status &OK;
  static const Status &CANCELLED;
  StatusCode code_;
};
}
#endif //AIR_CXX_STATUS_H
