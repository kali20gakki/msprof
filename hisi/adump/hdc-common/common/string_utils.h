/**
 * @file common/string_utils.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef ADX_STRING_UTILS_H
#define ADX_STRING_UTILS_H
#include <string>
namespace Adx {
constexpr uint32_t IP_VALID_PART_NUM      = 3;
constexpr uint32_t IP_MAX_NUM             = 255;
constexpr uint32_t IP_MIN_NUM             = 0;
class StringUtils {
public:
    static bool IsIntDigital(const std::string &digital);
    static bool IpValid(const std::string &ipStr);
    static bool ParsePrivInfo(const std::string &privInfo,
                                       std::string &hostId,
                                       std::string &hostPid);
};
}
#endif
