/**
 * @file common/string_utils.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include <cctype>
#include "log/adx_log.h"
#include "string_utils.h"
namespace Adx {
/**
 * @brief Check if is digital
 * @param [in] digital: digital
 * @return
 *        true:   string all digital
 *        false:  string have other char
 */
bool StringUtils::IsIntDigital(const std::string &digital)
{
    if (digital.empty()) {
        return false;
    }

    size_t len = digital.length();
    for (size_t i = 0; i < len; i++) {
        if (!isdigit(digital[i])) {
            return false;
        }
    }
    return true;
}

bool StringUtils::IpValid(const std::string &ipStr)
{
    if (ipStr.empty()) {
        return false;
    }

    std::string checkStr = ipStr;
    size_t num;
    size_t j = 0;
    for (size_t i = 0; i < IP_VALID_PART_NUM; ++i) {
        size_t index = checkStr.find('.', j);
        if (index == std::string::npos) {
            return false;
        }
        try {
            num = std::stoi(checkStr.substr(j, index - j));
        } catch (...) {
            return false;
        }

        if (num > IP_MAX_NUM) {
            return false;
        }
        j = index + 1;
    }
    std::string end = checkStr.substr(j);
    for (size_t i = 0; i < end.length(); ++i) {
        if (end[i] < '0' || end[i] > '9') {
            return false;
        }
    }
    try {
        num = stoi(end);
    } catch (...) {
        return false;
    }
    if (num > IP_MAX_NUM) {
        return false;
    }
    return true;
}

bool StringUtils::ParsePrivInfo(const std::string &privInfo, std::string &hostId, std::string &hostPid)
{
    std::string privInfoStr;
    std::string::size_type idx;

    privInfoStr = privInfo;

    idx = privInfoStr.find(";");
    if (idx == std::string::npos) {
        IDE_LOGE("invalid private info %s format, valid format like \"host:port;host_id;host_pid\"", privInfo.c_str());
        return false;
    }
    IDE_LOGD("info str check host:port;host_id success");

    std::string hostIdHostPidStr = privInfoStr.substr(idx + 1);
    idx = hostIdHostPidStr.find(";");
    if (idx == std::string::npos) {
        IDE_LOGE("invalid private info %s format, valid format like \"host:port;host_id;host_pid\"", privInfo.c_str());
        return false;
    }
    IDE_LOGD("info str check host_id;host_pid success");

    hostId = hostIdHostPidStr.substr(0, idx);
    hostPid = hostIdHostPidStr.substr(idx + 1);

    if (!IsIntDigital(hostId)) {
        IDE_LOGE("hostId is not a number, %s", hostId.c_str());
        return false;
    }

    if (!IsIntDigital(hostPid)) {
        IDE_LOGE("hostPid is not a number, %s", hostPid.c_str());
        return false;
    }

    IDE_LOGI("hostId: %s, hostPid: %s", hostId.c_str(), hostPid.c_str());
    IDE_LOGD("info str check host_id and host_pid number format success");
    return true;
}
}
