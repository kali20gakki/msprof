/**
* @log_recv_by_drv.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef LOG_RECV_BY_DRV_H
#define LOG_RECV_BY_DRV_H
typedef void *ParentSource;
typedef void *ChildSource;
#define DRV_RECV_MAX_LEN 1048576 // 1MB buffer space
#define RECV_FULLY_DATA_LEN (4 * DRV_RECV_MAX_LEN) // 4MB fully recv buffer space
#define PARENTSOURCE_INSTANCE 0x01
#endif
