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
 * -------------------------------------------------------------------------
*/

#ifndef MSPTI_COMMON_CONFIG_H
#define MSPTI_COMMON_CONFIG_H

#include <limits>

constexpr int64_t SUPPORT_SYSCNT_DRV_VERSION = 0x071905;
constexpr uint32_t HOST_ID = 64;
constexpr uint32_t MSTONS = 1000000;
constexpr uint32_t MARK_MAX_CACHE_NUM = std::numeric_limits<uint32_t>::max();
constexpr uint32_t MAX_MARK_MSG_LEN = std::numeric_limits<uint8_t>::max();
constexpr uint32_t DEFAULT_PERIOD_FLUSH_TIME = 60000;

#define CHANNEL_PROF_ERROR (-1)
#define CHANNEL_PROF_STOPPED_ALREADY (-4)
#define COMM_NAME_MAX_LENGTH 128
#endif
