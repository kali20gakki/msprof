/**
 * @share_mem.c
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "share_mem.h"
#include "log_common_util.h"
#include "print_log.h"

/**
 * @brief CreatShMem: create shared memory
 * @param [in/out]shmId: shared memory id
 * @return succeed:SHM_SUCCEED,failed:SHM_ERROR
*/
ShmErr CreatShMem(int *shmId)
{
    if (shmId == NULL) {
        return SHM_ERROR;
    }
    int memId = ToolShmGet(MSG_MEMORY_KEY, SHM_SIZE, IPC_CREAT | IPC_EXCL | SyncGroupToOther(SHM_MODE));
    if (memId == (int)SHM_ERROR) {
        SYSLOG_WARN("[ERROR] CreatShMem error, strerr=%s.try \"ipcs -m\" to check.\n", strerror(ToolGetErrorCode()));
        return SHM_ERROR;
    }
    *shmId = memId;
    return SHM_SUCCEED;
}

/**
 * @brief OpenShMem: open shared memory
 * @param [out]shmId:identifier ID
 * @return: SHM_SUCCEED/SHM_ERROR
*/
ShmErr OpenShMem(int *shmId)
{
    if (shmId == NULL) {
        return SHM_ERROR;
    }
    int memId = ToolShmGet(MSG_MEMORY_KEY, 0, 0);
    if (memId == (int)SHM_ERROR) {
        return SHM_ERROR;
    }
    *shmId = memId;
    return SHM_SUCCEED;
}

/**
 * @brief WriteToShMem: write string to shared memory
 * @param [in]shmId:share ID to identify shared memory
 * @param [in]value:string to be write
 * @param [in]len: max length of string
 * @return: SHM_SUCCEED/SHM_ERROR
*/
ShmErr WriteToShMem(int shmId, const char *value, unsigned int len, unsigned int offset)
{
    if ((shmId == (int)SHM_ERROR) || (value == NULL) || (len == 0)) {
        SYSLOG_WARN("[input]shmId or value is error, shmId = %d\n ", shmId);
        return SHM_ERROR;
    }
    char *shmvalue = (char *)ToolShMat(shmId, NULL, 0);
    if ((intptr_t)shmvalue == -1) {
        SYSLOG_WARN("[ERROR] WriteToShMem shmat failed ,strerr=%s.\n", strerror(ToolGetErrorCode()));
        return SHM_ERROR;
    }
    if (shmvalue == NULL) {
        return SHM_ERROR;
    }
    snprintf_truncated_s(shmvalue + offset, len, "%s", value);
    if (ToolShmDt(shmvalue) != (int)SHM_SUCCEED) {
        SYSLOG_WARN("[ERROR] shmdt failed, strerr=%s.\n", strerror(ToolGetErrorCode()));
        return SHM_ERROR;
    }
    return SHM_SUCCEED;
}

/**
* @brief ReadFromShMem: read string from shared memory
 * @param [in]shmId:share ID to identify shared memory
 * @param [in]value:buffer to store string
 * @param [in]len: max length of string
 * @return: SHM_SUCCEED/SHM_ERROR
*/
ShmErr ReadFromShMem(int shmId, char *value, unsigned int len, unsigned int offset)
{
    if ((value == NULL) || (len == 0)) {
        return SHM_ERROR;
    }
    char *shmvalue = (char *)ToolShMat(shmId, NULL, SHM_RDONLY);
    if ((intptr_t)shmvalue == -1) {
        return SHM_ERROR;
    }
    if (shmvalue == NULL) {
        return SHM_ERROR;
    }
    if ((strlen(shmvalue) == 0) || ((unsigned int)strlen(shmvalue) > len)) {
        return SHM_ERROR;
    }
    snprintf_truncated_s(value, len, "%s", shmvalue + offset);
    if (ToolShmDt(shmvalue) != (int)SHM_SUCCEED) {
        return SHM_ERROR;
    }
    return SHM_SUCCEED;
}

/**
* @brief RemoveShMem: remove the shared memory
 * @return: SHM_SUCCEED/SHM_ERROR
*/
void RemoveShMem(void)
{
    int shmId;
    if (OpenShMem(&shmId) == SHM_ERROR) {
        return;
    }
    if (ToolShmCtl(shmId, IPC_RMID, NULL) != 0) {
        SYSLOG_WARN("[ERROR] ToolShmCtl failed, strerr=%s.\n", strerror(ToolGetErrorCode()));
        return;
    }
    return;
}
