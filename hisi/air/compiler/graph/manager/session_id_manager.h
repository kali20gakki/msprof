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

#ifndef GE_COMPILER_GRAPH_MANAGER_SESSION_ID_MANAGER_H_
#define GE_COMPILER_GRAPH_MANAGER_SESSION_ID_MANAGER_H_

#include <atomic>

namespace ge {
using SessionId = uint64_t;

class SessionIdManager {
 public:
  static SessionId GetNextSessionId();
};
}  // namespace ge

#endif  // GE_COMPILER_GRAPH_MANAGER_SESSION_ID_MANAGER_H_
