/**
* @log_zip.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef LOG_ZIP_H
#define LOG_ZIP_H
#include <sys/types.h>
#include "cfg_file_parse.h"
#include "log_sys_package.h"
#include "log_common_util.h"
#if defined(HARDWARE_ZIP)
    #include "log_hardware_zip.h"
#endif // HARDWARE_ZIP

#define FILE_VERSION        0
#define ZIP_SWITCH          "zip_switch"
#define ZIP_OFF             0
#define ZIP_ON              1

/*
* @brief IsZip: judge zip switch on(1) or off(0).
* @return: zip switch(1/0)
*/
unsigned short IsZip(void);

/*
* @brief GetZipCfg: get zip config value, switch flag and zip level.
* @return: zip config data struct
*/
void GetZipCfg(void);
#endif // LOG_ZIP_H
