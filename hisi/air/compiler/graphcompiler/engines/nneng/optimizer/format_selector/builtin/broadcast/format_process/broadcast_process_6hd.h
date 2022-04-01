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

#ifndef FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_BUILTIN_BROADCAST_FORMAT_PROCESS_BROADCAST_PROCESS_6HD_H_
#define FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_BUILTIN_BROADCAST_FORMAT_PROCESS_BROADCAST_PROCESS_6HD_H_
#include "format_selector/builtin/broadcast/format_process/broadcast_format_process.h"

namespace fe {
class BroadcastProcess6HD : public BroadcastFormatProcess {
 public:
  BroadcastProcess6HD(){};
  ~BroadcastProcess6HD() override {};

  bool CheckPartNonScalarInputs(const FormatProccessInputArg &input_arg) override;

  bool CheckAllNonScalarInputs(const FormatProccessArgs &args) override;
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_BUILTIN_BROADCAST_FORMAT_PROCESS_BROADCAST_PROCESS_6HD_H_
