/**
 * @file config.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef IDE_DAEMON_COMMON_CONFIG_H
#define IDE_DAEMON_COMMON_CONFIG_H
#include <string>
#include "extra_config.h"
namespace IdeDaemon {
namespace Common {
namespace Config {
const int32_t IDE_DAEMON_BLOCK               = 0;
const int32_t IDE_DAEMON_NOBLOCK             = 1;
const uint32_t IDE_MAX_HDC_SEGMENT           = 524288;         // (512 * 1024) max size of hdc segment
const uint32_t IDE_MIN_HDC_SEGMENT           = 1024;           // min size of hdc segment
} // end config
}
}

#endif

