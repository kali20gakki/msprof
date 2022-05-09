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
MmpaPlugin::~MmpaPlugin()
{
    pluginHandle_.CloseHandle();
}

bool MmpaPlugin::IsFuncExist(const std::string &funcName) const
{
    return pluginHandle_.IsFuncExist(funcName);
}

INT32 MmpaPlugin::MsprofMmOpen2(const CHAR *pathName, INT32 flags, MODE mode)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");;
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMOPEN2_T func;
    ret = pluginHandle_.GetFunction<INT32, const CHAR *, INT32, MODE>("mmOpen2", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(pathName, flags, mode);
}

mmSsize_t MmpaPlugin::MsprofMmRead(INT32 fd, VOID *buf, UINT32 bufLen)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMREAD_T func;
    ret = pluginHandle_.GetFunction<mmSsize_t, INT32, VOID *, UINT32>("mmRead", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(fd, buf, bufLen);
}

mmSsize_t MmpaPlugin::MsprofMmWrite(INT32 fd, VOID *buf, UINT32 bufLen)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMWRITE_T func;
    ret = pluginHandle_.GetFunction<mmSsize_t, INT32, VOID *, UINT32>("mmWrite", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(fd, buf, bufLen);
}

INT32 MmpaPlugin::MsprofMmClose(INT32 fd)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMCLOSE_T func;
    ret = pluginHandle_.GetFunction<INT32, INT32>("mmClose", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(fd);
}

CHAR *MmpaPlugin::MsprofMmGetErrorFormatMessage(mmErrorMsg errnum, CHAR *buf, mmSize size)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return nullptr;
        }
    }
    MSPROF_MMGETERRORFORMATMESSAGE_T func;
    ret = pluginHandle_.GetFunction<CHAR *, mmErrorMsg, CHAR *, mmSize>("mmGetErrorFormatMessage", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return nullptr;
    }
    return func(errnum, buf, size);
}

INT32 MmpaPlugin::MsprofMmGetErrorCode()
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMGETERRORCODE_T func;
    ret = pluginHandle_.GetFunction<INT32>("mmGetErrorCode", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func();
}

INT32 MmpaPlugin::MsprofMmAccess2(const CHAR *pathName, INT32 mode)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMACCESS2_T func;
    ret = pluginHandle_.GetFunction<INT32, const CHAR *, INT32>("mmAccess2", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(pathName, mode);
}

INT32 MmpaPlugin::MsprofMmGetOptLong(INT32 argc,
                                     CHAR *const *argv,
                                     const CHAR *opts,
                                     const mmStructOption *longOpts,
                                     INT32 *longIndex)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMGETOPTLONG_T func;
    ret = pluginHandle_.GetFunction<INT32, INT32, CHAR *const *, const CHAR *,
                                    const mmStructOption *, INT32 *>("mmGetOptLong", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(argc, argv, opts, longOpts, longIndex);
}

INT32 MmpaPlugin::MsprofMmGetOptInd()
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMGETOPTIND_T func;
    ret = pluginHandle_.GetFunction<INT32>("mmGetOptInd", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func();
}

CHAR *MmpaPlugin::MsprofMmGetOptArg()
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return nullptr;
        }
    }
    MSPROF_MMGETOPTARG_T func;
    ret = pluginHandle_.GetFunction<CHAR *>("mmGetOptArg", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return nullptr;
    }
    return func();
}

INT32 MmpaPlugin::MsprofMmSleep(UINT32 milliSecond)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMSLEEP_T func;
    ret = pluginHandle_.GetFunction<INT32, UINT32>("mmSleep", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(milliSecond);
}

INT32 MmpaPlugin::MsprofMmUnlink(const CHAR *filename)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMUNLINK_T func;
    ret = pluginHandle_.GetFunction<INT32, const CHAR *>("mmUnlink", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(filename);
}

INT32 MmpaPlugin::MsprofMmGetTid()
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMGETTID_T func;
    ret = pluginHandle_.GetFunction<INT32>("mmGetTid", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func();
}

INT32 MmpaPlugin::MsprofMmGetTimeOfDay(mmTimeval *timeVal, mmTimezone *timeZone)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMGETTIMEOFDAY_T func;
    ret = pluginHandle_.GetFunction<INT32, mmTimeval *, mmTimezone *>("mmGetTimeOfDay", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(timeVal, timeZone);
}

mmTimespec MmpaPlugin::MsprofMmGetTickCount()
{
    mmTimespec rts;
    memset_s(&rts, sizeof(mmTimespec), 0, sizeof(mmTimespec));
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return rts;
        }
    }
    MSPROF_MMGETTICKCOUNT_T func;
    ret = pluginHandle_.GetFunction<mmTimespec>("mmGetTickCount", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return rts;
    }
    return func();
}

INT32 MmpaPlugin::MsprofMmGetFileSize(const CHAR *fileName, ULONGLONG *length)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMGETFILESIZE_T func;
    ret = pluginHandle_.GetFunction<INT32, const CHAR *, ULONGLONG *>("mmGetFileSize", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(fileName, length);
}

INT32 MmpaPlugin::MsprofMmGetDiskFreeSpace(const CHAR *path, mmDiskSize *diskSize)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMGETDISKFREESPACE_T func;
    ret = pluginHandle_.GetFunction<INT32, const CHAR *, mmDiskSize *>("mmGetDiskFreeSpace", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(path, diskSize);
}

INT32 MmpaPlugin::MsprofMmIsDir(const CHAR *fileName)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMISDIR_T func;
    ret = pluginHandle_.GetFunction<INT32, const CHAR *>("mmIsDir", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(fileName);
}

INT32 MmpaPlugin::MsprofMmAccess(const CHAR *pathName)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMACCESS_T func;
    ret = pluginHandle_.GetFunction<INT32, const CHAR *>("mmAccess", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(pathName);
}

CHAR *MmpaPlugin::MsprofMmDirName(CHAR *path)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return nullptr;
        }
    }
    MSPROF_MMDIRNAME_T func;
    ret = pluginHandle_.GetFunction<CHAR *, CHAR *>("mmDirName", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return nullptr;
    }
    return func(path);
}

CHAR *MmpaPlugin::MsprofMmBaseName(CHAR *path)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return nullptr;
        }
    }
    MSPROF_MMBASENAME_T func;
    ret = pluginHandle_.GetFunction<CHAR *, CHAR *>("mmBaseName", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return nullptr;
    }
    return func(path);
}

INT32 MmpaPlugin::MsprofMmMkdir(const CHAR *pathName, mmMode_t mode)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMMKDIR_T func;
    ret = pluginHandle_.GetFunction<INT32, const CHAR *, mmMode_t>("mmMkdir", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(pathName, mode);
}

INT32 MmpaPlugin::MsprofMmChmod(const CHAR *filename, INT32 mode)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMCHMOD_T func;
    ret = pluginHandle_.GetFunction<INT32, const CHAR *, INT32>("mmChmod", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(filename, mode);
}

INT32 MmpaPlugin::MsprofMmScandir(const CHAR *path, mmDirent ***entryList, mmFilter filterFunc, mmSort sort)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMSCANDIR_T func;
    ret = pluginHandle_.GetFunction<INT32, const CHAR *, mmDirent ***, mmFilter, mmSort>("mmScandir", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(path, entryList, filterFunc, sort);
}

INT32 MmpaPlugin::MsprofMmRmdir(const CHAR *pathName)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMRMDIR_T func;
    ret = pluginHandle_.GetFunction<INT32, const CHAR *>("mmRmdir", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(pathName);
}

INT32 MmpaPlugin::MsprofMmRealPath(const CHAR *path, CHAR *realPath, INT32 realPathLen)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMREALPATH_T func;
    ret = pluginHandle_.GetFunction<INT32, const CHAR *, CHAR *, INT32>("mmRealPath", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(path, realPath, realPathLen);
}

INT32 MmpaPlugin::MsprofMmCreateProcess(const CHAR *fileName, const mmArgvEnv *env,
                                        const CHAR* stdoutRedirectFile, mmProcess *id)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMCREATEPROCESS_T func;
    ret = pluginHandle_.GetFunction<INT32, const CHAR *, const mmArgvEnv *,
        const CHAR*, mmProcess *>("mmCreateProcess", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(fileName, env, stdoutRedirectFile, id);
}

VOID MmpaPlugin::MsprofMmScandirFree(mmDirent **entryList, INT32 count)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return;
        }
    }
    MSPROF_MMSCANDIRFREE_T func;
    ret = pluginHandle_.GetFunction<VOID, mmDirent **, INT32>("mmScandirFree", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return;
    }
    return func(entryList, count);
}

INT32 MmpaPlugin::MsprofMmChdir(const CHAR *path)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMCHDIR_T func;
    ret = pluginHandle_.GetFunction<INT32, const CHAR *>("mmChdir", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(path);
}

INT32 MmpaPlugin::MsprofMmWaitPid(mmProcess pid, INT32 *status, INT32 options)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMWAITPID_T func;
    ret = pluginHandle_.GetFunction<INT32, mmProcess, INT32 *, INT32>("mmWaitPid", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(pid, status, options);
}

INT32 MmpaPlugin::MsprofMmGetMac(mmMacInfo **list, INT32 *count)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMGETMAC_T func;
    ret = pluginHandle_.GetFunction<INT32, mmMacInfo **, INT32 *>("mmGetMac", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(list, count);
}

INT32 MmpaPlugin::MsprofMmGetMacFree(mmMacInfo *list, INT32 count)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMGETMACFREE_T func;
    ret = pluginHandle_.GetFunction<INT32, mmMacInfo *, INT32>("mmGetMacFree", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(list, count);
}

INT32 MmpaPlugin::MsprofMmGetLocalTime(mmSystemTime_t *sysTimePtr)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMGETLOCALTIME_T func;
    ret = pluginHandle_.GetFunction<INT32, mmSystemTime_t *>("mmGetLocalTime", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(sysTimePtr);
}

INT32 MmpaPlugin::MsprofMmGetPid()
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMGETPID_T func;
    ret = pluginHandle_.GetFunction<INT32>("mmGetPid", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func();
}

INT32 MmpaPlugin::MsprofMmGetCwd(CHAR *buffer, INT32 maxLen)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMGETCWD_T func;
    ret = pluginHandle_.GetFunction<INT32, CHAR *, INT32>("mmGetCwd", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(buffer, maxLen);
}

INT32 MmpaPlugin::MsprofMmGetEnv(const CHAR *name, CHAR *value, UINT32 len)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMGETENV_T func;
    ret = pluginHandle_.GetFunction<INT32, const CHAR *, CHAR *, UINT32>("mmGetEnv", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(name, value, len);
}

INT32 MmpaPlugin::MsprofMmCreateTaskWithThreadAttr(mmThread *threadHandle, const mmUserBlock_t *funcBlock,
                                                   const mmThreadAttr *threadAttr)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMCREATETASKWITHTHREADATTR_T func;
    ret = pluginHandle_.GetFunction<INT32, mmThread *, const mmUserBlock_t *,
        const mmThreadAttr *>("mmCreateTaskWithThreadAttr", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(threadHandle, funcBlock, threadAttr);
}

INT32 MmpaPlugin::MsprofMmJoinTask(mmThread *threadHandle)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMJOINTASK_T func;
    ret = pluginHandle_.GetFunction<INT32, mmThread *>("mmJoinTask", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(threadHandle);
}

INT32 MmpaPlugin::MsprofMmSetCurrentThreadName(const CHAR* name)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMSETCURRENTTHREADNAME_T func;
    ret = pluginHandle_.GetFunction<INT32, const CHAR *>("mmSetCurrentThreadName", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(name);
}

INT32 MmpaPlugin::MsprofMmStatGet(const CHAR *path, mmStat_t *buffer)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMSTATGET_T func;
    ret = pluginHandle_.GetFunction<INT32, const CHAR *, mmStat_t *>("mmStatGet", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(path, buffer);
}

INT32 MmpaPlugin::MsprofMmGetOsVersion(CHAR* versionInfo, INT32 versionLength)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMGETOSVERSION_T func;
    ret = pluginHandle_.GetFunction<INT32, CHAR*, INT32>("mmGetOsVersion", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(versionInfo, versionLength);
}

INT32 MmpaPlugin::MsprofMmGetOsName(CHAR* name, INT32 nameSize)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMGETOSNAME_T func;
    ret = pluginHandle_.GetFunction<INT32, CHAR *, INT32>("mmGetOsName", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(name, nameSize);
}

INT32 MmpaPlugin::MsprofMmGetCpuInfo(mmCpuDesc **cpuInfo, INT32 *count)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMGETCPUINFO_T func;
    ret = pluginHandle_.GetFunction<INT32, mmCpuDesc **, INT32 *>("mmGetCpuInfo", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(cpuInfo, count);
}

INT32 MmpaPlugin::MsprofMmCpuInfoFree(mmCpuDesc *cpuInfo, INT32 count)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMCPUINFOFORFREE_T func;
    ret = pluginHandle_.GetFunction<INT32, mmCpuDesc *, INT32>("mmCpuInfoFree", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(cpuInfo, count);
}

INT32 MmpaPlugin::MsprofMmDup2(INT32 oldFd, INT32 newFd)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMDUP2_T func;
    ret = pluginHandle_.GetFunction<INT32, INT32, INT32>("mmDup2", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(oldFd, newFd);
}

// mmGetPidHandle
INT32 MmpaPlugin::MsprofMmGetPidHandle(mmProcess *pstProcessHandle)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMGETPIDHANDLE_T func;
    ret = pluginHandle_.GetFunction<INT32, mmProcess *>("mmGetPidHandle", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(pstProcessHandle);
}
    
// mmCloseSocket
INT32 MmpaPlugin::MsprofMmCloseSocket(mmSockHandle sockFd)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMCLOSESOCKET_T func;
    ret = pluginHandle_.GetFunction<INT32, mmSockHandle>("mmCloseSocket", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(sockFd);
}
    
// mmSocketSend
mmSsize_t MmpaPlugin::MsprofMmSocketSend(mmSockHandle sockFd,VOID *pstSendBuf, INT32 sendLen, INT32 sendFlag)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMSOCKETSEND_T func;
    ret = pluginHandle_.GetFunction<mmSsize_t, mmSockHandle, VOID *, INT32, INT32>("mmSocketSend", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(sockFd, pstSendBuf, sendLen, sendFlag);
}
    
// mmSocketRecv
mmSsize_t MmpaPlugin::MsprofMmSocketRecv(mmSockHandle sockFd, VOID *pstRecvBuf, INT32 recvLen, INT32 recvFlag)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMSOCKETRECV_T func;
    ret = pluginHandle_.GetFunction<mmSsize_t, mmSockHandle, VOID *, INT32, INT32>("mmSocketRecv", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(sockFd, pstRecvBuf, recvLen, recvFlag);
}
    
// mmSocket
mmSockHandle MmpaPlugin::MspofMmSocket(INT32 sockFamily, INT32 type, INT32 protocol)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMSOCKET_T func;
    ret = pluginHandle_.GetFunction<mmSockHandle, INT32, INT32, INT32>("mmSocket", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(sockFamily, type, protocol);
}
    
// mmBind
INT32 MmpaPlugin::MsprofMmBind(mmSockHandle sockFd, mmSockAddr* addr, mmSocklen_t addrLen)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMBIND_T func;
    ret = pluginHandle_.GetFunction<INT32, mmSockHandle, mmSockAddr*, mmSocklen_t>("mmBind", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(sockFd, addr, addrLen);
}
    
// mmListen
INT32 MmpaPlugin::MsprofMmListen(mmSockHandle sockFd, INT32 backLog)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMLISTEN_T func;
    ret = pluginHandle_.GetFunction<INT32, mmSockHandle, INT32>("mmListen", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(sockFd, backLog);
}
    
// mmAccept
mmSockHandle MmpaPlugin::MsprofMmAccept(mmSockHandle sockFd, mmSockAddr *addr, mmSocklen_t *addrLen)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMSOCKETHANDLE_T func;
    ret = pluginHandle_.GetFunction<mmSockHandle, mmSockHandle, mmSockAddr *, mmSocklen_t *>("mmAccept", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(sockFd, addr, addrLen);
}
    
// mmConnect
INT32 MmpaPlugin::MsprofMmConnect(mmSockHandle sockFd, mmSockAddr* addr, mmSocklen_t addrLen)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMCONNECT_T func;
    ret = pluginHandle_.GetFunction<INT32, mmSockHandle, mmSockAddr*, mmSocklen_t>("mmConnect", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(sockFd, addr, addrLen);
}

// mmSAStartup
INT32 MmpaPlugin::MsprofMmSAStartup()
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMSASTARTUP_T func;
    ret = pluginHandle_.GetFunction<INT32>("mmSAStartup", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func();
}

// mmSACleanup
INT32 MmpaPlugin::MsprofMmSACleanup()
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMSACLEANUP_T func;
    ret = pluginHandle_.GetFunction<INT32>("mmSACleanup", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func();
}

// mmCreateTask
INT32 MmpaPlugin::MsprofMmCreateTask(mmThread *threadHandle, mmUserBlock_t *funcBlock)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMCREATETASK_T func;
    ret = pluginHandle_.GetFunction<INT32, mmThread *, mmUserBlock_t *>("mmCreateTask", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(threadHandle, funcBlock);
}

// mmCreateTaskWithThreadAttrStub
INT32 MmpaPlugin::MsprofMmCreateTaskWithThreadAttrStub(mmThread *threadHandle, const mmUserBlock_t *funcBlock,
                                                       const mmThreadAttr *threadAttr)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMCREATETASKWITHTHREADATTRSTUB_T func;
    ret = pluginHandle_.GetFunction<INT32, mmThread *, const mmUserBlock_t *,
        const mmThreadAttr *>("mmCreateTaskWithThreadAttrStub", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(threadHandle, funcBlock, threadAttr);
}
                                     
// mmCreateTaskWithThreadAttrNormalStub
INT32 MmpaPlugin::MsprofMmCreateTaskWithThreadAttrNormalStub(mmThread *threadHandle, const mmUserBlock_t *funcBlock,
                                                             const mmThreadAttr *threadAttr)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMCREATETASKWITHTHREADATTRNORMALSTUB_T func;
    ret = pluginHandle_.GetFunction<INT32, mmThread *, const mmUserBlock_t *,
        const mmThreadAttr *>("mmCreateTaskWithThreadAttrNormalStub", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(threadHandle, funcBlock, threadAttr);
}

// mmMutexInit
INT32 MmpaPlugin::MsprofMmMutexInit(mmMutex_t *mutex)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMMUTEXINIT_T func;
    ret = pluginHandle_.GetFunction<INT32, mmMutex_t *>("mmMutexInit", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(mutex);
}

// mmMutexLock
INT32 MmpaPlugin::MsprofMmMutexLock(mmMutex_t *mutex)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMMUTEXLOCK_T func;
    ret = pluginHandle_.GetFunction<INT32, mmMutex_t *>("mmMutexLock", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(mutex);
}
    
// mmMutexUnLock
INT32 MmpaPlugin::MsprofMmMutexUnLock(mmMutex_t *mutex)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMMUTEXUNLOCK_T func;
    ret = pluginHandle_.GetFunction<INT32, mmMutex_t *>("mmMutexUnLock", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(mutex);
}

// mmGetOpt
INT32 MmpaPlugin::MsprofMmGetOpt(INT32 argc, CHAR * const * argv, const CHAR *opts)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_MMGETOPT_T func;
    ret = pluginHandle_.GetFunction<INT32, INT32, CHAR * const *, const CHAR *>("mmGetOpt", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(argc, argv, opts);
}
} // namespace Plugin
} // namespace Dvvp
} // namespace Analysis