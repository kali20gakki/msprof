/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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

#ifndef AIR_COMPILER_GRAPHCOMPILER_ENGINES_NNENG_OPTIMIZER_CMO_GENERATE_CMO_TYPE_MANAGER_H_
#define AIR_COMPILER_GRAPHCOMPILER_ENGINES_NNENG_OPTIMIZER_CMO_GENERATE_CMO_TYPE_MANAGER_H_

#include "generate_cmo_type_base.h"
#include "common/fe_inner_error_codes.h"

namespace fe {
using GenerateCMOTypeBasePtr = std::shared_ptr<GenerateCMOTypeBase>;

class GenerateCMOTypeManager {
public:
  static GenerateCMOTypeManager &Instance() {
    static GenerateCMOTypeManager instance;
    return instance;
  }

  Status Initialize();

  Status Finalize();

  void GenerateType(const ge::NodePtr &node) const;

private:
  GenerateCMOTypeManager() = default;
  ~GenerateCMOTypeManager() = default;

  Status Register(CmoType type);

  std::map<CmoType, GenerateCMOTypeBasePtr> cmo_generate_map_{};
};
} // namespace fe
#endif
