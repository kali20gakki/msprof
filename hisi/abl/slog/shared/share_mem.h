/**
 * @share_mem.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef SHARE_MEM_H
#define SHARE_MEM_H
#include "log_sys_package.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    SHM_ERROR = -1,
    SHM_SUCCEED = 0
} ShmErr;

enum {
    MSG_MEMORY_KEY = 0x474f4c46
};

#define PATH_LEN 1024
#define MODULE_ARR_LEN 2048
#define LEVEL_ARR_LEN 1024

#define SHM_SIZE 4096
#define SHM_MODE 0640

ShmErr CreatShMem(int *shmId);
ShmErr OpenShMem(int *shmId);
ShmErr WriteToShMem(int shmId, const char *value, unsigned int len, unsigned int offset);
ShmErr ReadFromShMem(int shmId, char *value, unsigned int len, unsigned int offset);
void RemoveShMem(void);
#ifdef __cplusplus
}
#endif
#endif /* SHARE_MEM_H */
