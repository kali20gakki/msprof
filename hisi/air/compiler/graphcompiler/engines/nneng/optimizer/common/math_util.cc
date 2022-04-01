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

#include "common/math_util.h"

namespace fe {
const uint16_t kFp16ExpBias = 15;
const uint32_t kFp32ExpBias = 127;
const uint16_t kFp16ManLen = 10;
const uint32_t kFp32ManLen = 23;
const uint32_t kFp32SignIndex = 31;
const uint16_t kFp16ManMask = 0x03FF;
const uint16_t kFp16ManHideBit = 0x0400;
const uint16_t kFp16MaxExp = 0x001F;
const uint32_t kFp32MaxMan = 0x7FFFFF;

float Uint16ToFloat(const uint16_t &intVal) {
  float ret;

  uint16_t hfSign = (intVal >> 15) & 1;
  int16_t hfExp = (intVal >> kFp16ManLen) & kFp16MaxExp;
  uint16_t hfMan = ((intVal >> 0) & 0x3FF) | ((((intVal >> 10) & 0x1F) > 0 ? 1 : 0) * 0x400);
  if (hfExp == 0) {
    hfExp = 1;
  }

  while (hfMan && !(hfMan & kFp16ManHideBit)) {
    hfMan <<= 1;
    hfExp--;
  }

  uint32_t sRet, eRet, mRet, fVal;

  sRet = hfSign;
  if (!hfMan) {
    eRet = 0;
    mRet = 0;
  } else {
    eRet = hfExp - kFp16ExpBias + kFp32ExpBias;
    mRet = hfMan & kFp16ManMask;
    mRet = mRet << (kFp32ManLen - kFp16ManLen);
  }
  fVal = ((sRet) << kFp32SignIndex) | ((eRet) << kFp32ManLen) | ((mRet) & kFp32MaxMan);
  ret = *(reinterpret_cast<float *>(&fVal));

  return ret;
}
}  // namespace fe
