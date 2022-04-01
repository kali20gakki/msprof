/**
 * Copyright 2019-2021 Huawei Technologies Co., Ltd
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

#include "comm_log.h"
#include "common/resource_def.h"

namespace fe {
struct funcStateInfo {
  bool isConfiged;
  bool isOpen;
};
using funcState = struct funcStateInfo;

class FuncSetting {
 public:
  static FuncSetting *Instance();
  bool SetFuncState(FuncParamType type, bool isOpen);
  bool GetFuncState(FuncParamType type);

 protected:
  FuncSetting()
  {
    for (int i = 0; i < static_cast<int>(FuncParamType::MAX_NUM); i++) {
      state[i].isConfiged = false;
      state[i].isOpen = false;
    }
  }
  ~FuncSetting() {}

 private:
  funcState state[static_cast<size_t>(FuncParamType::MAX_NUM)];
};

FuncSetting *FuncSetting::Instance()
{
  static FuncSetting instance;
  return &instance;
}

bool FuncSetting::SetFuncState(FuncParamType type, bool isOpen)
{
  if (type < FuncParamType::MAX_NUM) {
    state[static_cast<size_t>(type)].isConfiged = true;
    state[static_cast<size_t>(type)].isOpen = isOpen;
    return true;
  } else {
    CM_LOGE("SetFuncState type[%d] is invilid!", static_cast<int32_t>(type));
    return false;
  }
}

bool FuncSetting::GetFuncState(FuncParamType type)
{
  if (type < FuncParamType::MAX_NUM) {
    if (state[static_cast<size_t>(type)].isConfiged)
    {
      return state[static_cast<size_t>(type)].isOpen;
    }
  }
  return false;
}

bool setFuncState(FuncParamType type, bool isOpen)
{
  return FuncSetting::Instance()->SetFuncState(type, isOpen);
}

bool getFuncState(FuncParamType type)
{
  return FuncSetting::Instance()->GetFuncState(type);
}

}
