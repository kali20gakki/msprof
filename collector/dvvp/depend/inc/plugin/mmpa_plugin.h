#ifndef MMPA_PLUGIN_H
#define MMPA_PLUGIN_H

#include "singleton/singleton.h"
#include "mmpa/mmpa_api.h"
#include "plugin_manager.h"

namespace Analysis {
namespace Dvvp {
namespace Plugin {
class MmpaPlugin : public analysis::dvvp::common::singleton::Singleton<MmpaPlugin> {
public:
    MmpaPlugin()
    : soName_("libmmpa.so"),
      pluginManager_(PluginManager(soName_))
    {}
    ~MmpaPlugin();

    bool IsFuncExist(const std::string &funcName) const;

    using MSPROF_MMOPEN2_T = std::function<INT32(const CHAR *, INT32, MODE)>;
    INT32 MsprofMmOpen2(const CHAR *pathName, INT32 flags, MODE mode);

    using MSPROF_MMREAD_T = std::function<mmSsize_t(INT32, VOID *, UINT32)>;
    mmSsize_t MsprofMmRead(INT32 fd, VOID *buf, UINT32 bufLen);

    using MSPROF_MMWRITE_T = std::function<mmSsize_t(INT32, VOID *, UINT32)>;
    mmSsize_t MsprofMmWrite(INT32 fd, VOID *buf, UINT32 bufLen);

    using MSPROF_MMCLOSE_T = std::function<INT32(INT32)>;
    INT32 MsprofMmClose(INT32 fd);

    using MSPROF_MMGETERRORFORMATMESSAGE_T = std::function<CHAR *(mmErrorMsg, CHAR *, mmSize)>;
    CHAR *MsprofMmGetErrorFormatMessage(mmErrorMsg errnum, CHAR *buf, mmSize size);

    using MSPROF_MMGETERRORCODE_T = std::function<INT32()>;
    INT32 MsprofMmGetErrorCode();

    using MSPROF_MMACCESS2_T = std::function<INT32(const CHAR *, INT32)>;
    INT32 MsprofMmAccess2(const CHAR *pathName, INT32 mode);

    using MSPROF_MMGETOPTLONG_T = std::function<INT32(INT32, CHAR *const *, const CHAR *, const mmStructOption *, INT32 *)>;
    INT32 MsprofMmGetOptLong(INT32 argc,
                   CHAR *const *argv,
                   const CHAR *opts,
                   const mmStructOption *longOpts,
                   INT32 *longIndex);

    using MSPROF_MMGETOPTIND_T = std::function<INT32()>;
    INT32 MsprofMmGetOptInd();

    using MSPROF_MMGETOPTARG_T = std::function<CHAR *()>;
    CHAR *MsprofMmGetOptArg();

    using MSPROF_MMSLEEP_T = std::function<INT32(UINT32)>;
    INT32 MsprofMmSleep(UINT32 milliSecond);

    using MSPROF_MMUNLINK_T = std::function<INT32(const CHAR *)>;
    INT32 MsprofMmUnlink(const CHAR *filename);

    using MSPROF_MMGETTID_T = std::function<INT32()>;
    INT32 MsprofMmGetTid();

    using MSPROF_MMGETTIMEOFDAY_T = std::function<INT32(mmTimeval *, mmTimezone *)>;
    INT32 MsprofMmGetTimeOfDay(mmTimeval *timeVal, mmTimezone *timeZone);

    using MSPROF_MMGETTICKCOUNT_T = std::function<mmTimespec()>;
    mmTimespec MsprofMmGetTickCount(VOID);

    using MSPROF_MMGETFILESIZE_T = std::function<INT32(const CHAR *, ULONGLONG *)>;
    INT32 MsprofMmGetFileSize(const CHAR *fileName, ULONGLONG *length);

    using MSPROF_MMGETDISKFREESPACE_T = std::function<INT32(const CHAR *, mmDiskSize *)>;
    INT32 MsprofMmGetDiskFreeSpace(const CHAR *path, mmDiskSize *diskSize);

    using MSPROF_MMISDIR_T = std::function<INT32(const CHAR *)>;
    INT32 MsprofMmIsDir(const CHAR *fileName);

    using MSPROF_MMACCESS_T = std::function<INT32(const CHAR *)>;
    INT32 MsprofMmAccess(const CHAR *pathName);

    using MSPROF_MMDIRNAME_T = std::function<CHAR *(CHAR *)>;
    CHAR *MsprofMmDirName(CHAR *path);
    
    using MSPROF_MMBASENAME_T = std::function<CHAR *(CHAR *)>;
    CHAR *MsprofMmBaseName(CHAR *path);

    using MSPROF_MMMKDIR_T = std::function<INT32(const CHAR *, mmMode_t)>;
    INT32 MsprofMmMkdir(const CHAR *pathName, mmMode_t mode);

    using MSPROF_MMCHMOD_T = std::function<INT32(const CHAR *, INT32)>;
    INT32 MsprofMmChmod(const CHAR *filename, INT32 mode);
    
    using MSPROF_MMSCANDIR_T = std::function<INT32(const CHAR *, mmDirent ***, mmFilter, mmSort)>;
    INT32 MsprofMmScandir(const CHAR *path, mmDirent ***entryList, mmFilter filterFunc, mmSort sort);
    
    using MSPROF_MMRMDIR_T = std::function<INT32(const CHAR *)>;
    INT32 MsprofMmRmdir(const CHAR *pathName);
    
    using MSPROF_MMREALPATH_T = std::function<INT32(const CHAR *, CHAR *, INT32)>;
    INT32 MsprofMmRealPath(const CHAR *path, CHAR *realPath, INT32 realPathLen);
    
    using MSPROF_MMCREATEPROCESS_T = std::function<INT32(const CHAR *, const mmArgvEnv *, const CHAR*, mmProcess *)>;
    INT32 MsprofMmCreateProcess(const CHAR *fileName, const mmArgvEnv *env, const CHAR* stdoutRedirectFile, mmProcess *id);
    
    using MSPROF_MMSCANDIRFREE_T = std::function<VOID(mmDirent **, INT32)>;
    VOID MsprofMmScandirFree(mmDirent **entryList, INT32 count);
    
    using MSPROF_MMCHDIR_T = std::function<INT32(const CHAR *)>;
    INT32 MsprofMmChdir(const CHAR *path);
    
    using MSPROF_MMWAITPID_T = std::function<INT32(mmProcess, INT32 *, INT32)>;
    INT32 MsprofMmWaitPid(mmProcess pid, INT32 *status, INT32 options);
    
    using MSPROF_MMGETMAC_T = std::function<INT32(mmMacInfo **, INT32 *)>;
    INT32 MsprofMmGetMac(mmMacInfo **list, INT32 *count);
    
    using MSPROF_MMGETMACFREE_T = std::function<INT32(mmMacInfo *, INT32)>;
    INT32 MsprofMmGetMacFree(mmMacInfo *list, INT32 count);
    
    using MSPROF_MMGETLOCALTIME_T = std::function<INT32(mmSystemTime_t *)>;
    INT32 MsprofMmGetLocalTime(mmSystemTime_t *sysTimePtr);
    
    using MSPROF_MMGETPID_T = std::function<INT32()>;
    INT32 MsprofMmGetPid(VOID);
    
    using MSPROF_MMGETCWD_T = std::function<INT32(CHAR *, INT32)>;
    INT32 MsprofMmGetCwd(CHAR *buffer, INT32 maxLen);
    
    using MSPROF_MMGETENV_T = std::function<INT32(const CHAR *, CHAR *, UINT32)>;
    INT32 MsprofMmGetEnv(const CHAR *name, CHAR *value, UINT32 len);

    using MSPROF_MMCREATETASKWITHTHREADATTR_T = std::function<INT32(mmThread *, const mmUserBlock_t *, const mmThreadAttr *)>;
    INT32 MsprofMmCreateTaskWithThreadAttr(mmThread *threadHandle, const mmUserBlock_t *funcBlock,
        const mmThreadAttr *threadAttr);
        
    using MSPROF_MMJOINTASK_T = std::function<INT32(mmThread *)>;
    INT32 MsprofMmJoinTask(mmThread *threadHandle);
    
    using MSPROF_MMSETCURRENTTHREADNAME_T = std::function<INT32(const CHAR*)>;
    INT32 MsprofMmSetCurrentThreadName(const CHAR* name);
    
    using MSPROF_MMSTATGET_T = std::function<INT32(const CHAR *, mmStat_t *)>;
    INT32 MsprofMmStatGet(const CHAR *path, mmStat_t *buffer);
    
    using MSPROF_MMGETOSVERSION_T = std::function<INT32(CHAR*, INT32)>;
    INT32 MsprofMmGetOsVersion(CHAR* versionInfo, INT32 versionLength);
    
    using MSPROF_MMGETOSNAME_T = std::function<INT32(CHAR *, INT32)>;
    INT32 MsprofMmGetOsName(CHAR* name, INT32 nameSize);
    
    using MSPROF_MMGETCPUINFO_T = std::function<INT32(mmCpuDesc **, INT32 *)>;
    INT32 MsprofMmGetCpuInfo(mmCpuDesc **cpuInfo, INT32 *count);
    
    using MSPROF_MMCPUINFOFORFREE_T = std::function<INT32(mmCpuDesc *, INT32)>;
    INT32 MsprofMmCpuInfoFree(mmCpuDesc *cpuInfo, INT32 count);

    using MSPROF_MMDUP2_T = std::function<INT32(INT32, INT32)>;
    INT32 MsprofMmDup2(INT32 oldFd, INT32 newFd);
    
private:
    std::string soName_;
    PluginManager pluginManager_;
};

} // Plugin
} // Dvvp
} // Analysis
#endif