/**
* @log_hardware_zip.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef LOG_HARDWARE_ZIP_H
#define LOG_HARDWARE_ZIP_H
#if (OS_TYPE_DEF == LINUX)
#include "ascend_hal.h"
#endif
#include "print_log.h"

#define ZIP_RATIO 2
#define ST_GZIP_HEADER_SZ 10
#define ST_GZIP_HEADER "\x1f\x8b\x08\x00\x00\x00\x00\x00\x00\x03"
#define BLOCK_SIZE (1024 * 1024) // 1MB

typedef struct {
    unsigned short version;  // zip tool version
    unsigned short zip_sw;   // zip switch
    unsigned int resv;       // reserve
    unsigned int data_len;   // log_data length
} LogMsgHeadInfo;

/*
* @brief HardwareZip: compress <source> to <destination>
* @param [in]source: origin buffer
* @param [in]sourceLen: origin buffer length
* @param [out]dest: 2nd ptr point to output buffer
* @param [out]destLen: ptr point to output buffer length
* @return: SUCCESS: succeed; others: failed;
*/
LogRt HardwareZip(const char *source, int sourceLen, char **dest, int *destLen);

/*
* @brief WriteLogBlock: write compress data to file
* @param [in]fd: file fd
* @param [in]msgBlock: log msg head data struct
* @param [in]logData: compress data
* @return: SUCCESS: succeed; others: failed;
*/
LogRt WriteLogBlock(int fd, const LogMsgHeadInfo *msgBlock, const char *logData);
#endif // LOG_HARDWARE_ZIP_H
