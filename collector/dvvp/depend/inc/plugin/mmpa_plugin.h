/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: mmpa interface
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-04-15
 */
#ifndef MMPA_PLUGIN_H
#define MMPA_PLUGIN_H

#include "singleton/singleton.h"
#include "mmpa/mmpa_api.h"
#include "plugin_handle.h"

namespace Collector {
namespace Dvvp {
namespace Plugin {
using MmOpen2Func = std::function<INT32(const CHAR *, INT32, MODE)>;
using MmReadFunc = std::function<mmSsize_t(INT32, void *, UINT32)>;
using MmWriteFunc = std::function<mmSsize_t(INT32, void *, UINT32)>;
using MmCloseFunc = std::function<INT32(INT32)>;
using MmGetErrorFormatMessageFunc = std::function<CHAR *(mmErrorMsg, CHAR *, mmSize)>;
using MmGetErrorCodeFunc = std::function<INT32()>;
using MmAccess2Func = std::function<INT32(const CHAR *, INT32)>;
using MmGetOptLongFunc = std::function<INT32(INT32, CHAR *const *, const CHAR *, const mmStructOption *, INT32 *)>;
using MmGetOptIndFunc = std::function<INT32()>;
using MmGetOptArgFunc = std::function<CHAR *()>;
using MmSleepFunc = std::function<INT32(UINT32)>;
using MmUnlinkFunc = std::function<INT32(const CHAR *)>;
using MmGetTidFunc = std::function<INT32()>;
using MmGetTimeOfDayFunc = std::function<INT32(mmTimeval *, mmTimezone *)>;
using MmGetTickCountFunc = std::function<mmTimespec()>;
using MmGetFileSizeFunc = std::function<INT32(const CHAR *, ULONGLONG *)>;
using MmGetDiskFreeSpaceFunc = std::function<INT32(const CHAR *, mmDiskSize *)>;
using MmIsDirFunc = std::function<INT32(const CHAR *)>;
using MmAccessFunc = std::function<INT32(const CHAR *)>;
using MmDirNameFunc = std::function<CHAR *(CHAR *)>;
using MmBaseNameFunc = std::function<CHAR *(CHAR *)>;
using MmMkdirFunc = std::function<INT32(const CHAR *, mmMode_t)>;
using MmChmodFunc = std::function<INT32(const CHAR *, INT32)>;
using MmScandirFunc = std::function<INT32(const CHAR *, mmDirent ***, mmFilter, mmSort)>;
using MmRmdirFunc = std::function<INT32(const CHAR *)>;
using MmRealPathFunc = std::function<INT32(const CHAR *, CHAR *, INT32)>;
using MmCreateProcessFunc = std::function<INT32(const CHAR *, const mmArgvEnv *, const CHAR*, mmProcess *)>;
using MmScandirFreeFunc = std::function<void(mmDirent **, INT32)>;
using MmChdirFunc = std::function<INT32(const CHAR *)>;
using MmWaitPidFunc = std::function<INT32(mmProcess, INT32 *, INT32)>;
using MmGetMacFunc = std::function<INT32(mmMacInfo **, INT32 *)>;
using MmGetMacFreeFunc = std::function<INT32(mmMacInfo *, INT32)>;
using MmGetLocalTimeFunc = std::function<INT32(mmSystemTime_t *)>;
using MmGetPidFunc = std::function<INT32()>;
using MmGetCwdFunc = std::function<INT32(CHAR *, INT32)>;
using MmGetEnvFunc = std::function<INT32(const CHAR *, CHAR *, UINT32)>;
using MmCreateTaskWithThreadAttrFunc = std::function<INT32(mmThread *, const mmUserBlock_t *, const mmThreadAttr *)>;
using MmJoinTaskFunc = std::function<INT32(mmThread *)>;
using MmSetCurrentThreadNameFunc = std::function<INT32(const CHAR*)>;
using MmStatGetFunc = std::function<INT32(const CHAR *, mmStat_t *)>;
using MmGetOsVersionFunc = std::function<INT32(CHAR*, INT32)>;
using MmGetOsNameFunc = std::function<INT32(CHAR *, INT32)>;
using MmGetCpuInfoFunc = std::function<INT32(mmCpuDesc **, INT32 *)>;
using MmCpuInfoFreeFunc = std::function<INT32(mmCpuDesc *, INT32)>;
using MmDup2Func = std::function<INT32(INT32, INT32)>;
using MmGetPidHandleFunc = std::function<INT32(mmProcess *)>;
using MmCloseSocketFunc = std::function<INT32(mmSockHandle)>;
using MmSocketSendFunc = std::function<mmSsize_t(mmSockHandle, void *, INT32, INT32)>;
using MmSocketRecvFunc = std::function<mmSsize_t(mmSockHandle, void *, INT32, INT32)>;
using MmSocketFunc = std::function<mmSockHandle(INT32, INT32, INT32)>;
using MmBindFunc = std::function<INT32(mmSockHandle, mmSockAddr*, mmSocklen_t)>;
using MmListenFunc = std::function<INT32(mmSockHandle, INT32)>;
using MmAcceptFunc = std::function<mmSockHandle(mmSockHandle, mmSockAddr *, mmSocklen_t *)>;
using MmConnectFun = std::function<INT32(mmSockHandle, mmSockAddr*, mmSocklen_t)>;
using MmSAStartupFunc = std::function<INT32()>;
using MmSACleanupFunc = std::function<INT32()>;
using MmCreateTaskFunc = std::function<INT32(mmThread *, mmUserBlock_t *)>;
using MmCreateTaskWithThreadAttrStubFunc = std::function<INT32(mmThread *, const mmUserBlock_t *, const mmThreadAttr *)>;
using MmCreateTaskWithThreadAttrNormalStubFunc = std::function<INT32(mmThread *, const mmUserBlock_t *, const mmThreadAttr *)>;
using MmMutexInitFunc = std::function<INT32(mmMutex_t *)>;
using MmMutexLockFunc = std::function<INT32(mmMutex_t *)>;
using MmMutexUnLockFunc = std::function<INT32(mmMutex_t *)>;
using MmGetOptFunc = std::function<INT32(INT32, CHAR * const *, const CHAR *)>;
class MmpaPlugin : public analysis::dvvp::common::singleton::Singleton<MmpaPlugin> {
public:
    MmpaPlugin()
    : soName_("libmmpa.so"),
      pluginHandle_(PluginHandle(soName_)),
      loadFlag_(0)
    {}

    bool IsFuncExist(const std::string &funcName) const;

    // mmOpen2
    INT32 MsprofMmOpen2(const CHAR *pathName, INT32 flags, MODE mode);

    // mmRead
    mmSsize_t MsprofMmRead(INT32 fd, void *buf, UINT32 bufLen);

    // mmWrite
    mmSsize_t MsprofMmWrite(INT32 fd, void *buf, UINT32 bufLen);

    // mmClose
    INT32 MsprofMmClose(INT32 fd);

    // mmGetErrorFormatMessage
    CHAR *MsprofMmGetErrorFormatMessage(mmErrorMsg errnum, CHAR *buf, mmSize size);

    // mmGetErrorCode
    INT32 MsprofMmGetErrorCode();

    // mmAccess2
    INT32 MsprofMmAccess2(const CHAR *pathName, INT32 mode);

    // mmGetOptLong
    INT32 MsprofMmGetOptLong(INT32 argc,
                   CHAR *const *argv,
                   const CHAR *opts,
                   const mmStructOption *longOpts,
                   INT32 *longIndex);

    // mmGetOptInd
    INT32 MsprofMmGetOptInd();

    // mmGetOptArg
    CHAR *MsprofMmGetOptArg();

    // mmSleep
    INT32 MsprofMmSleep(UINT32 milliSecond);

    // mmUnlink
    INT32 MsprofMmUnlink(const CHAR *filename);

    // mmGetTid
    INT32 MsprofMmGetTid();

    // mmGetTimeOfDay
    INT32 MsprofMmGetTimeOfDay(mmTimeval *timeVal, mmTimezone *timeZone);

    // mmGetTickCount
    mmTimespec MsprofMmGetTickCount(void);

    // mmGetFileSize
    INT32 MsprofMmGetFileSize(const CHAR *fileName, ULONGLONG *length);

    // mmGetDiskFreeSpace
    INT32 MsprofMmGetDiskFreeSpace(const CHAR *path, mmDiskSize *diskSize);

    // mmIsDir
    INT32 MsprofMmIsDir(const CHAR *fileName);

    // mmAccess
    INT32 MsprofMmAccess(const CHAR *pathName);

    // mmDirName
    CHAR *MsprofMmDirName(CHAR *path);
    
    // mmBaseName
    CHAR *MsprofMmBaseName(CHAR *path);

    // mmMkdir
    INT32 MsprofMmMkdir(const CHAR *pathName, mmMode_t mode);

    // mmChmod
    INT32 MsprofMmChmod(const CHAR *filename, INT32 mode);
    
    // mmScandir
    INT32 MsprofMmScandir(const CHAR *path, mmDirent ***entryList, mmFilter filterFunc, mmSort sort);
    
    // mmRmdir
    INT32 MsprofMmRmdir(const CHAR *pathName);
    
    // mmRealPath
    INT32 MsprofMmRealPath(const CHAR *path, CHAR *realPath, INT32 realPathLen);
    
    // mmCreateProcess
    INT32 MsprofMmCreateProcess(const CHAR *fileName, const mmArgvEnv *env, const CHAR* stdoutRedirectFile, mmProcess *id);
    
    // mmScandirFree
    void MsprofMmScandirFree(mmDirent **entryList, INT32 count);
    
    // mmChdir
    INT32 MsprofMmChdir(const CHAR *path);
    
    // mmWaitPid
    INT32 MsprofMmWaitPid(mmProcess pid, INT32 *status, INT32 options);
    
    // mmGetMac
    INT32 MsprofMmGetMac(mmMacInfo **list, INT32 *count);
    
    // mmGetMacFree
    INT32 MsprofMmGetMacFree(mmMacInfo *list, INT32 count);
    
    // mmGetLocalTime
    INT32 MsprofMmGetLocalTime(mmSystemTime_t *sysTimePtr);
    
    // mmGetPid
    INT32 MsprofMmGetPid(void);
    
    // mmGetCwd
    INT32 MsprofMmGetCwd(CHAR *buffer, INT32 maxLen);
    
    // mmGetEnv
    INT32 MsprofMmGetEnv(const CHAR *name, CHAR *value, UINT32 len);

    // mmCreateTaskWithThreadAttr
    INT32 MsprofMmCreateTaskWithThreadAttr(mmThread *threadHandle, const mmUserBlock_t *funcBlock,
        const mmThreadAttr *threadAttr);
        
    // mmJoinTask
    INT32 MsprofMmJoinTask(mmThread *threadHandle);
    
    // mmSetCurrentThreadName
    INT32 MsprofMmSetCurrentThreadName(const CHAR* name);
    
    // mmStatGet
    INT32 MsprofMmStatGet(const CHAR *path, mmStat_t *buffer);
    
    // mmGetOsVersion
    INT32 MsprofMmGetOsVersion(CHAR* versionInfo, INT32 versionLength);
    
    // mmGetOsName
    INT32 MsprofMmGetOsName(CHAR* name, INT32 nameSize);
    
    // mmGetCpuInfo
    INT32 MsprofMmGetCpuInfo(mmCpuDesc **cpuInfo, INT32 *count);
    
    // mmCpuInfoFree
    INT32 MsprofMmCpuInfoFree(mmCpuDesc *cpuInfo, INT32 count);

    // mmDup2
    INT32 MsprofMmDup2(INT32 oldFd, INT32 newFd);
    
    // mmGetPidHandle
    INT32 MsprofMmGetPidHandle(mmProcess *pstProcessHandle);
    
    // mmCloseSocket
    INT32 MsprofMmCloseSocket(mmSockHandle sockFd);
    
    // mmSocketSend
    mmSsize_t MsprofMmSocketSend(mmSockHandle sockFd,void *pstSendBuf,INT32 sendLen,INT32 sendFlag);
    
    // mmSocketRecv
    mmSsize_t MsprofMmSocketRecv(mmSockHandle sockFd, void *pstRecvBuf,INT32 recvLen,INT32 recvFlag);
    
    // mmSocket
    mmSockHandle MspofMmSocket(INT32 sockFamily, INT32 type, INT32 protocol);
    
    // mmBind
    INT32 MsprofMmBind(mmSockHandle sockFd, mmSockAddr* addr, mmSocklen_t addrLen);
    
    // mmListen
    INT32 MsprofMmListen(mmSockHandle sockFd, INT32 backLog);
    
    // mmAccept
    mmSockHandle MsprofMmAccept(mmSockHandle sockFd, mmSockAddr *addr, mmSocklen_t *addrLen);
    
    // mmConnect
    INT32 MsprofMmConnect(mmSockHandle sockFd, mmSockAddr* addr, mmSocklen_t addrLen);
    
    // mmSAStartup
    INT32 MsprofMmSAStartup();
    
    // mmSACleanup
    INT32 MsprofMmSACleanup();
    
    // mmCreateTask
    INT32 MsprofMmCreateTask(mmThread *threadHandle, mmUserBlock_t *funcBlock);
    
    // mmCreateTaskWithThreadAttrStub
    INT32 MsprofMmCreateTaskWithThreadAttrStub(mmThread *threadHandle, const mmUserBlock_t *funcBlock,
                                         const mmThreadAttr *threadAttr);
                                         
    // mmCreateTaskWithThreadAttrNormalStub
    INT32 MsprofMmCreateTaskWithThreadAttrNormalStub(mmThread *threadHandle, const mmUserBlock_t *funcBlock,
                                         const mmThreadAttr *threadAttr);
    
    // mmMutexInit
    INT32 MsprofMmMutexInit(mmMutex_t *mutex);
    
    // mmMutexLock
    INT32 MsprofMmMutexLock(mmMutex_t *mutex);
    
    // mmMutexUnLock
    INT32 MsprofMmMutexUnLock(mmMutex_t *mutex);
    
    // mmGetOpt
    INT32 MsprofMmGetOpt(INT32 argc, CHAR * const * argv, const CHAR *opts);
    
private:
    std::string soName_;
    PluginHandle pluginHandle_;
    PTHREAD_ONCE_T loadFlag_;
    MmOpen2Func mmOpen2_ = nullptr;
    MmReadFunc mmRead_ = nullptr;
    MmWriteFunc mmWrite_ = nullptr;
    MmCloseFunc mmClose_ = nullptr;
    MmGetErrorFormatMessageFunc mmGetErrorFormatMessage_ = nullptr;
    MmGetErrorCodeFunc mmGetErrorCode_ = nullptr;
    MmAccess2Func mmAccess2_ = nullptr;
    MmGetOptLongFunc mmGetOptLong_ = nullptr;
    MmGetOptIndFunc mmGetOptInd_ = nullptr;
    MmGetOptArgFunc mmGetOptArg_ = nullptr;
    MmSleepFunc mmSleep_ = nullptr;
    MmUnlinkFunc mmUnlink_ = nullptr;
    MmGetTidFunc mmGetTid_ = nullptr;
    MmGetTimeOfDayFunc mmGetTimeOfDay_ = nullptr;
    MmGetTickCountFunc mmGetTickCount_ = nullptr;
    MmGetFileSizeFunc mmGetFileSize_ = nullptr;
    MmGetDiskFreeSpaceFunc mmGetDiskFreeSpace_ = nullptr;
    MmIsDirFunc mmIsDir_ = nullptr;
    MmAccessFunc mmAccess_ = nullptr;
    MmDirNameFunc mmDirName_ = nullptr;
    MmBaseNameFunc mmBaseName_ = nullptr;
    MmMkdirFunc mmMkdir_ = nullptr;
    MmChmodFunc mmChmod_ = nullptr;
    MmScandirFunc mmScandir_ = nullptr;
    MmRmdirFunc mmRmdir_ = nullptr;
    MmRealPathFunc mmRealPath_ = nullptr;
    MmCreateProcessFunc mmCreateProcess_ = nullptr;
    MmScandirFreeFunc mmScandirFree_ = nullptr;
    MmChdirFunc mmChdir_ = nullptr;
    MmWaitPidFunc mmWaitPid_ = nullptr;
    MmGetMacFunc mmGetMac_ = nullptr;
    MmGetMacFreeFunc mmGetMacFree_ = nullptr;
    MmGetLocalTimeFunc mmGetLocalTime_ = nullptr;
    MmGetPidFunc mmGetPid_ = nullptr;
    MmGetCwdFunc mmGetCwd_ = nullptr;
    MmGetEnvFunc mmGetEnv_ = nullptr;
    MmCreateTaskWithThreadAttrFunc mmCreateTaskWithThreadAttr_ = nullptr;
    MmJoinTaskFunc mmJoinTask_ = nullptr;
    MmSetCurrentThreadNameFunc mmSetCurrentThreadName_ = nullptr;
    MmStatGetFunc mmStatGet_ = nullptr;
    MmGetOsVersionFunc mmGetOsVersion_ = nullptr;
    MmGetOsNameFunc mmGetOsName_ = nullptr;
    MmGetCpuInfoFunc MsprofMmGetCpuInfo_ = nullptr;
    MmCpuInfoFreeFunc mmCpuInfoFree_ = nullptr;
    MmDup2Func mmDup2_ = nullptr;
    MmGetPidHandleFunc mmGetPidHandle_ = nullptr;
    MmCloseSocketFunc mmCloseSocket_ = nullptr;
    MmSocketSendFunc mmSocketSend_ = nullptr;
    MmSocketRecvFunc mmSocketRecv_ = nullptr;
    MmSocketFunc mmSocket_ = nullptr;
    MmBindFunc mmBind_ = nullptr;
    MmListenFunc mmListen_ = nullptr;
    MmAcceptFunc mmAccept_ = nullptr;
    MmConnectFun mmConnect_ = nullptr;
    MmSAStartupFunc mmSAStartup_ = nullptr;
    MmSACleanupFunc mmSACleanup_ = nullptr;
    MmCreateTaskFunc mmCreateTask_ = nullptr;
    MmCreateTaskWithThreadAttrStubFunc mmCreateTaskWithThreadAttrStub_ = nullptr;
    MmCreateTaskWithThreadAttrNormalStubFunc mmCreateTaskWithThreadAttrNormalStub_ = nullptr;
    MmMutexInitFunc mmMutexInit_ = nullptr;
    MmMutexLockFunc mmMutexLock_ = nullptr;
    MmMutexUnLockFunc mmMutexUnLock_ = nullptr;
    MmGetOptFunc mmGetOpt_ = nullptr;
 
 private:
    void LoadMmpaSo();
};
} // Plugin
} // Dvvp
} // Collector
#endif