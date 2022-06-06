/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: mmpa interface
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-04-15
 */
#include "mmpa_plugin.h"

#include "securec.h"

namespace Analysis {
namespace Dvvp {
namespace Plugin {
void MmpaPlugin::LoadMmpaSo()
{
    PluginStatus ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return;
        }
    }
}

bool MmpaPlugin::IsFuncExist(const std::string &funcName) const
{
    return pluginHandle_.IsFuncExist(funcName);
}

INT32 MmpaPlugin::MsprofMmOpen2(const CHAR *pathName, INT32 flags, MODE mode)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmOpen2_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, const CHAR *, INT32, MODE>("mmOpen2", mmOpen2_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmOpen2_(pathName, flags, mode);
}

mmSsize_t MmpaPlugin::MsprofMmRead(INT32 fd, VOID *buf, UINT32 bufLen)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmRead_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<mmSsize_t, INT32, VOID *, UINT32>("mmRead", mmRead_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return 0;
        }
    }
    return mmRead_(fd, buf, bufLen);
}

mmSsize_t MmpaPlugin::MsprofMmWrite(INT32 fd, VOID *buf, UINT32 bufLen)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmWrite_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<mmSsize_t, INT32, VOID *, UINT32>("mmWrite", mmWrite_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return 0;
        }
    }
    return mmWrite_(fd, buf, bufLen);
}

INT32 MmpaPlugin::MsprofMmClose(INT32 fd)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmClose_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, INT32>("mmClose", mmClose_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmClose_(fd);
}

CHAR *MmpaPlugin::MsprofMmGetErrorFormatMessage(mmErrorMsg errnum, CHAR *buf, mmSize size)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmGetErrorFormatMessage_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<CHAR *, mmErrorMsg, CHAR *, mmSize>("mmGetErrorFormatMessage",
            mmGetErrorFormatMessage_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return nullptr;
        }
    }
    return mmGetErrorFormatMessage_(errnum, buf, size);
}

INT32 MmpaPlugin::MsprofMmGetErrorCode()
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmGetErrorCode_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32>("mmGetErrorCode", mmGetErrorCode_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmGetErrorCode_();
}

INT32 MmpaPlugin::MsprofMmAccess2(const CHAR *pathName, INT32 mode)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmAccess2_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, const CHAR *, INT32>("mmAccess2", mmAccess2_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmAccess2_(pathName, mode);
}

INT32 MmpaPlugin::MsprofMmGetOptLong(INT32 argc,
                                     CHAR *const *argv,
                                     const CHAR *opts,
                                     const mmStructOption *longOpts,
                                     INT32 *longIndex)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmGetOptLong_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, INT32, CHAR *const *, const CHAR *,
                              const mmStructOption *, INT32 *>("mmGetOptLong", mmGetOptLong_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmGetOptLong_(argc, argv, opts, longOpts, longIndex);
}

INT32 MmpaPlugin::MsprofMmGetOptInd()
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmGetOptInd_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32>("mmGetOptInd", mmGetOptInd_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmGetOptInd_();
}

CHAR *MmpaPlugin::MsprofMmGetOptArg()
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmGetOptArg_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<CHAR *>("mmGetOptArg", mmGetOptArg_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return nullptr;
        }
    }
    return mmGetOptArg_();
}

INT32 MmpaPlugin::MsprofMmSleep(UINT32 milliSecond)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmSleep_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, UINT32>("mmSleep", mmSleep_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmSleep_(milliSecond);
}

INT32 MmpaPlugin::MsprofMmUnlink(const CHAR *filename)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmUnlink_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, const CHAR *>("mmUnlink", mmUnlink_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmUnlink_(filename);
}

INT32 MmpaPlugin::MsprofMmGetTid()
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmGetTid_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32>("mmGetTid", mmGetTid_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmGetTid_();
}

INT32 MmpaPlugin::MsprofMmGetTimeOfDay(mmTimeval *timeVal, mmTimezone *timeZone)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmGetTimeOfDay_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, mmTimeval *, mmTimezone *>("mmGetTimeOfDay",
            mmGetTimeOfDay_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmGetTimeOfDay_(timeVal, timeZone);
}

mmTimespec MmpaPlugin::MsprofMmGetTickCount()
{
    mmTimespec rts;
    memset_s(&rts, sizeof(mmTimespec), 0, sizeof(mmTimespec));
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmGetTickCount_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<mmTimespec>("mmGetTickCount", mmGetTickCount_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return rts;
        }
    }
    return mmGetTickCount_();
}

INT32 MmpaPlugin::MsprofMmGetFileSize(const CHAR *fileName, ULONGLONG *length)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmGetFileSize_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, const CHAR *, ULONGLONG *>("mmGetFileSize",
            mmGetFileSize_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmGetFileSize_(fileName, length);
}

INT32 MmpaPlugin::MsprofMmGetDiskFreeSpace(const CHAR *path, mmDiskSize *diskSize)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmGetDiskFreeSpace_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, const CHAR *, mmDiskSize *>("mmGetDiskFreeSpace",
            mmGetDiskFreeSpace_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmGetDiskFreeSpace_(path, diskSize);
}

INT32 MmpaPlugin::MsprofMmIsDir(const CHAR *fileName)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmIsDir_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, const CHAR *>("mmIsDir", mmIsDir_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmIsDir_(fileName);
}

INT32 MmpaPlugin::MsprofMmAccess(const CHAR *pathName)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmAccess_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, const CHAR *>("mmAccess", mmAccess_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmAccess_(pathName);
}

CHAR *MmpaPlugin::MsprofMmDirName(CHAR *path)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmDirName_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<CHAR *, CHAR *>("mmDirName", mmDirName_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return nullptr;
        }
    }
    return mmDirName_(path);
}

CHAR *MmpaPlugin::MsprofMmBaseName(CHAR *path)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmBaseName_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<CHAR *, CHAR *>("mmBaseName", mmBaseName_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return nullptr;
        }
    }
    return mmBaseName_(path);
}

INT32 MmpaPlugin::MsprofMmMkdir(const CHAR *pathName, mmMode_t mode)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmMkdir_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, const CHAR *, mmMode_t>("mmMkdir", mmMkdir_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmMkdir_(pathName, mode);
}

INT32 MmpaPlugin::MsprofMmChmod(const CHAR *filename, INT32 mode)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmChmod_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, const CHAR *, INT32>("mmChmod", mmChmod_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmChmod_(filename, mode);
}

INT32 MmpaPlugin::MsprofMmScandir(const CHAR *path, mmDirent ***entryList, mmFilter filterFunc, mmSort sort)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmScandir_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, const CHAR *, mmDirent ***, mmFilter, mmSort>("mmScandir",
            mmScandir_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmScandir_(path, entryList, filterFunc, sort);
}

INT32 MmpaPlugin::MsprofMmRmdir(const CHAR *pathName)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmRmdir_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, const CHAR *>("mmRmdir", mmRmdir_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmRmdir_(pathName);
}

INT32 MmpaPlugin::MsprofMmRealPath(const CHAR *path, CHAR *realPath, INT32 realPathLen)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmRealPath_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, const CHAR *, CHAR *, INT32>("mmRealPath", mmRealPath_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmRealPath_(path, realPath, realPathLen);
}

INT32 MmpaPlugin::MsprofMmCreateProcess(const CHAR *fileName, const mmArgvEnv *env,
                                        const CHAR* stdoutRedirectFile, mmProcess *id)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmCreateProcess_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, const CHAR *, const mmArgvEnv *,
                              const CHAR*, mmProcess *>("mmCreateProcess", mmCreateProcess_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmCreateProcess_(fileName, env, stdoutRedirectFile, id);
}

VOID MmpaPlugin::MsprofMmScandirFree(mmDirent **entryList, INT32 count)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmScandirFree_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<VOID, mmDirent **, INT32>("mmScandirFree", mmScandirFree_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return;
        }
    }
    return mmScandirFree_(entryList, count);
}

INT32 MmpaPlugin::MsprofMmChdir(const CHAR *path)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmChdir_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, const CHAR *>("mmChdir", mmChdir_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmChdir_(path);
}

INT32 MmpaPlugin::MsprofMmWaitPid(mmProcess pid, INT32 *status, INT32 options)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmWaitPid_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, mmProcess, INT32 *, INT32>("mmWaitPid", mmWaitPid_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmWaitPid_(pid, status, options);
}

INT32 MmpaPlugin::MsprofMmGetMac(mmMacInfo **list, INT32 *count)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmGetMac_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, mmMacInfo **, INT32 *>("mmGetMac", mmGetMac_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmGetMac_(list, count);
}

INT32 MmpaPlugin::MsprofMmGetMacFree(mmMacInfo *list, INT32 count)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmGetMacFree_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, mmMacInfo *, INT32>("mmGetMacFree", mmGetMacFree_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmGetMacFree_(list, count);
}

INT32 MmpaPlugin::MsprofMmGetLocalTime(mmSystemTime_t *sysTimePtr)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmGetLocalTime_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, mmSystemTime_t *>("mmGetLocalTime", mmGetLocalTime_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmGetLocalTime_(sysTimePtr);
}

INT32 MmpaPlugin::MsprofMmGetPid()
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmGetPid_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32>("mmGetPid", mmGetPid_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmGetPid_();
}

INT32 MmpaPlugin::MsprofMmGetCwd(CHAR *buffer, INT32 maxLen)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmGetCwd_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, CHAR *, INT32>("mmGetCwd", mmGetCwd_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmGetCwd_(buffer, maxLen);
}

INT32 MmpaPlugin::MsprofMmGetEnv(const CHAR *name, CHAR *value, UINT32 len)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmGetEnv_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, const CHAR *, CHAR *, UINT32>("mmGetEnv", mmGetEnv_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmGetEnv_(name, value, len);
}

INT32 MmpaPlugin::MsprofMmCreateTaskWithThreadAttr(mmThread *threadHandle, const mmUserBlock_t *funcBlock,
                                                   const mmThreadAttr *threadAttr)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmCreateTaskWithThreadAttr_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, mmThread *, const mmUserBlock_t *,
                              const mmThreadAttr *>("mmCreateTaskWithThreadAttr", mmCreateTaskWithThreadAttr_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmCreateTaskWithThreadAttr_(threadHandle, funcBlock, threadAttr);
}

INT32 MmpaPlugin::MsprofMmJoinTask(mmThread *threadHandle)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmJoinTask_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, mmThread *>("mmJoinTask", mmJoinTask_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmJoinTask_(threadHandle);
}

INT32 MmpaPlugin::MsprofMmSetCurrentThreadName(const CHAR* name)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmSetCurrentThreadName_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, const CHAR *>("mmSetCurrentThreadName",
            mmSetCurrentThreadName_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmSetCurrentThreadName_(name);
}

INT32 MmpaPlugin::MsprofMmStatGet(const CHAR *path, mmStat_t *buffer)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmStatGet_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, const CHAR *, mmStat_t *>("mmStatGet", mmStatGet_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmStatGet_(path, buffer);
}

INT32 MmpaPlugin::MsprofMmGetOsVersion(CHAR* versionInfo, INT32 versionLength)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmGetOsVersion_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, CHAR*, INT32>("mmGetOsVersion", mmGetOsVersion_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmGetOsVersion_(versionInfo, versionLength);
}

INT32 MmpaPlugin::MsprofMmGetOsName(CHAR* name, INT32 nameSize)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmGetOsName_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, CHAR *, INT32>("mmGetOsName", mmGetOsName_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmGetOsName_(name, nameSize);
}

INT32 MmpaPlugin::MsprofMmGetCpuInfo(mmCpuDesc **cpuInfo, INT32 *count)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (MsprofMmGetCpuInfo_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, mmCpuDesc **, INT32 *>("mmGetCpuInfo",
            MsprofMmGetCpuInfo_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return MsprofMmGetCpuInfo_(cpuInfo, count);
}

INT32 MmpaPlugin::MsprofMmCpuInfoFree(mmCpuDesc *cpuInfo, INT32 count)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmCpuInfoFree_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, mmCpuDesc *, INT32>("mmCpuInfoFree", mmCpuInfoFree_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmCpuInfoFree_(cpuInfo, count);
}

INT32 MmpaPlugin::MsprofMmDup2(INT32 oldFd, INT32 newFd)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmDup2_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, INT32, INT32>("mmDup2", mmDup2_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmDup2_(oldFd, newFd);
}

// mmGetPidHandle
INT32 MmpaPlugin::MsprofMmGetPidHandle(mmProcess *pstProcessHandle)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmGetPidHandle_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, mmProcess *>("mmGetPidHandle", mmGetPidHandle_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmGetPidHandle_(pstProcessHandle);
}
    
// mmCloseSocket
INT32 MmpaPlugin::MsprofMmCloseSocket(mmSockHandle sockFd)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmCloseSocket_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, mmSockHandle>("mmCloseSocket", mmCloseSocket_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmCloseSocket_(sockFd);
}
    
// mmSocketSend
mmSsize_t MmpaPlugin::MsprofMmSocketSend(mmSockHandle sockFd, VOID *pstSendBuf, INT32 sendLen, INT32 sendFlag)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmSocketSend_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<mmSsize_t, mmSockHandle, VOID *, INT32, INT32>("mmSocketSend",
            mmSocketSend_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return 0;
        }
    }
    return mmSocketSend_(sockFd, pstSendBuf, sendLen, sendFlag);
}
    
// mmSocketRecv
mmSsize_t MmpaPlugin::MsprofMmSocketRecv(mmSockHandle sockFd, VOID *pstRecvBuf, INT32 recvLen, INT32 recvFlag)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmSocketRecv_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<mmSsize_t, mmSockHandle, VOID *, INT32, INT32>("mmSocketRecv",
            mmSocketRecv_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return 0;
        }
    }
    return mmSocketRecv_(sockFd, pstRecvBuf, recvLen, recvFlag);
}
    
// mmSocket
mmSockHandle MmpaPlugin::MspofMmSocket(INT32 sockFamily, INT32 type, INT32 protocol)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmSocket_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<mmSockHandle, INT32, INT32, INT32>("mmSocket", mmSocket_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmSocket_(sockFamily, type, protocol);
}
    
// mmBind
INT32 MmpaPlugin::MsprofMmBind(mmSockHandle sockFd, mmSockAddr* addr, mmSocklen_t addrLen)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmBind_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, mmSockHandle, mmSockAddr*, mmSocklen_t>("mmBind", mmBind_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmBind_(sockFd, addr, addrLen);
}
    
// mmListen
INT32 MmpaPlugin::MsprofMmListen(mmSockHandle sockFd, INT32 backLog)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmListen_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, mmSockHandle, INT32>("mmListen", mmListen_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmListen_(sockFd, backLog);
}
    
// mmAccept
mmSockHandle MmpaPlugin::MsprofMmAccept(mmSockHandle sockFd, mmSockAddr *addr, mmSocklen_t *addrLen)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmAccept_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<mmSockHandle, mmSockHandle, mmSockAddr *,
            mmSocklen_t *>("mmAccept", mmAccept_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmAccept_(sockFd, addr, addrLen);
}
    
// mmConnect
INT32 MmpaPlugin::MsprofMmConnect(mmSockHandle sockFd, mmSockAddr* addr, mmSocklen_t addrLen)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmConnect_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, mmSockHandle, mmSockAddr*, mmSocklen_t>("mmConnect",
            mmConnect_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmConnect_(sockFd, addr, addrLen);
}

// mmSAStartup
INT32 MmpaPlugin::MsprofMmSAStartup()
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmSAStartup_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32>("mmSAStartup", mmSAStartup_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmSAStartup_();
}

// mmSACleanup
INT32 MmpaPlugin::MsprofMmSACleanup()
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmSACleanup_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32>("mmSACleanup", mmSACleanup_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmSACleanup_();
}

// mmCreateTask
INT32 MmpaPlugin::MsprofMmCreateTask(mmThread *threadHandle, mmUserBlock_t *funcBlock)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmCreateTask_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, mmThread *, mmUserBlock_t *>("mmCreateTask",
            mmCreateTask_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmCreateTask_(threadHandle, funcBlock);
}

// mmCreateTaskWithThreadAttrStub
INT32 MmpaPlugin::MsprofMmCreateTaskWithThreadAttrStub(mmThread *threadHandle, const mmUserBlock_t *funcBlock,
                                                       const mmThreadAttr *threadAttr)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmCreateTaskWithThreadAttrStub_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, mmThread *, const mmUserBlock_t *,
            const mmThreadAttr *>("mmCreateTaskWithThreadAttrStub", mmCreateTaskWithThreadAttrStub_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmCreateTaskWithThreadAttrStub_(threadHandle, funcBlock, threadAttr);
}
                                     
// mmCreateTaskWithThreadAttrNormalStub
INT32 MmpaPlugin::MsprofMmCreateTaskWithThreadAttrNormalStub(mmThread *threadHandle, const mmUserBlock_t *funcBlock,
                                                             const mmThreadAttr *threadAttr)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmCreateTaskWithThreadAttrNormalStub_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, mmThread *, const mmUserBlock_t *,
            const mmThreadAttr *>("mmCreateTaskWithThreadAttrNormalStub", mmCreateTaskWithThreadAttrNormalStub_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmCreateTaskWithThreadAttrNormalStub_(threadHandle, funcBlock, threadAttr);
}

// mmMutexInit
INT32 MmpaPlugin::MsprofMmMutexInit(mmMutex_t *mutex)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmMutexInit_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, mmMutex_t *>("mmMutexInit", mmMutexInit_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmMutexInit_(mutex);
}

// mmMutexLock
INT32 MmpaPlugin::MsprofMmMutexLock(mmMutex_t *mutex)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmMutexLock_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, mmMutex_t *>("mmMutexLock", mmMutexLock_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmMutexLock_(mutex);
}
    
// mmMutexUnLock
INT32 MmpaPlugin::MsprofMmMutexUnLock(mmMutex_t *mutex)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmMutexUnLock_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, mmMutex_t *>("mmMutexUnLock", mmMutexUnLock_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmMutexUnLock_(mutex);
}

// mmGetOpt
INT32 MmpaPlugin::MsprofMmGetOpt(INT32 argc, CHAR * const * argv, const CHAR *opts)
{
    PthreadOnce(&loadFlag_, []()->void {MmpaPlugin::instance()->LoadMmpaSo();});
    if (mmGetOpt_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<INT32, INT32, CHAR * const *, const CHAR *>("mmGetOpt",
            mmGetOpt_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return mmGetOpt_(argc, argv, opts);
}
} // namespace Plugin
} // namespace Dvvp
} // namespace Analysis