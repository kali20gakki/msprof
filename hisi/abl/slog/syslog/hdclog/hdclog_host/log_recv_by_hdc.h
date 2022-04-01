/**
* @log_recv_by_hdc.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef LOG_RECV_BY_HDC_H
#define LOG_RECV_BY_HDC_H
#include "ascend_hal.h"
typedef HDC_CLIENT ParentSource;
typedef HDC_SESSION ChildSource;

#define HDC_RECV_MAX_LEN 524288 // 512KB buffer space
#define RECV_FULLY_DATA_LEN (8 * HDC_RECV_MAX_LEN) // 4MB fully recv buffer space
#define HDCDRV_SERVICE_TYPE_LOG 5
#define MAX_HDC_SESSION_NUM 64
#define CONN_E_PRINT_NUM 20
#define CONN_W_PRINT_NUM 20
#endif
