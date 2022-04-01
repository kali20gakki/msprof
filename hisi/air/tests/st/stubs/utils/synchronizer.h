/**
 * Copyright 2022 Huawei Technologies Co., Ltd
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

#ifndef AIR_CXX_TESTS_ST_STUBS_UTILS_SYNCHRONIZER_H_
#define AIR_CXX_TESTS_ST_STUBS_UTILS_SYNCHRONIZER_H_
#include <mutex>
namespace ge {
class Synchronizer {
 public:
  Synchronizer() {
    mu_.lock();
  }
  void OnDone() {
    mu_.unlock();
  }
  bool WaitFor(int64_t seconds) {
    return mu_.try_lock_for(std::chrono::seconds(seconds));
  }
 private:
  std::timed_mutex mu_;
};
}
#endif //AIR_CXX_TESTS_ST_STUBS_UTILS_SYNCHRONIZER_H_
