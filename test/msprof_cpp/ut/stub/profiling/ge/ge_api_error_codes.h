/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#ifndef INC_EXTERNAL_GE_GE_API_ERROR_CODES_H_
#define INC_EXTERNAL_GE_GE_API_ERROR_CODES_H_

#include <map>
#include <string>
#include "prof_api.h"

namespace ge {
class StatusFactory {
 public:
  static StatusFactory *Instance() {
    static StatusFactory instance;
    return &instance;
  }

  void RegisterErrorNo(uint32_t err, const std::string &desc) {
    // Avoid repeated addition
    if (err_desc_.find(err) != err_desc_.end()) {
      return;
    }
    err_desc_[err] = desc;
  }

  std::string GetErrDesc(uint32_t err) {
    auto iter_find = err_desc_.find(err);
    if (iter_find == err_desc_.end()) {
      return "";
    }
    return iter_find->second;
  }

 protected:
  StatusFactory() {}
  ~StatusFactory() {}

 private:
  std::map<uint32_t, std::string> err_desc_;
};

class ErrorNoRegisterar {
 public:
  ErrorNoRegisterar(uint32_t err, const std::string &desc) { StatusFactory::Instance()->RegisterErrorNo(err, desc); }
  ~ErrorNoRegisterar() {}
};

// // Code compose(4 byte), runtime: 2 bit,  type: 2 bit,   level: 3 bit,  sysid: 8 bit, modid: 5 bit, value: 12 bit
// #define GE_ERRORNO(runtime, type, level, sysid, modid, name, value, desc)                              \
//   constexpr ge::Status name =                                                                          \
//     ((0xFF & (static_cast<uint8_t>(runtime))) << 30) | ((0xFF & (static_cast<uint8_t>(type))) << 28) | \
//     ((0xFF & (static_cast<uint8_t>(level))) << 25) | ((0xFF & (static_cast<uint8_t>(sysid))) << 17) |  \
//     ((0xFF & (static_cast<uint8_t>(modid))) << 12) | (0x0FFF & (static_cast<uint16_t>(value)));        \
//   const ErrorNoRegisterar g_##name##_errorno(name, desc);

// using Status = uint32_t;

// // General error code
// GE_ERRORNO(0, 0, 0, 0, 0, SUCCESS, 0, "success");
// GE_ERRORNO(0b11, 0b11, 0b111, 0xFF, 0b11111, FAILED, 0xFFF, "failed");
}  // namespace ge

#endif  // INC_EXTERNAL_GE_GE_API_ERROR_CODES_H_
