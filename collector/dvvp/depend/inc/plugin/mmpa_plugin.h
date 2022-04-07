#ifndef MMPA_PLUGIN_H
#define MMPA_PLUGIN_H

#include "singleton/singleton.h"
#include "mmpa/mmpa_api.h"
#include "plugin_manager.h"

namespace Analysis {
namespace Dvvp {
namespace Plugin {
class MmpaPlugin : public analysis::dvvp::common::singleton::Singleton<DriverPlugin> {
public:
    MmpaPlugin()
     :load_(false),
     :soName_("libmmpa.so"),
      pluginManager_(PluginManager(soName_))
    {}
    ~MmpaPlugin(pluginManager_.CloseHandle());

    using MSPROF_MMOPEN2_T = INT32 (*)(const CHAR *, INT32, MODE);
    INT32 MsprofMmOpen2(const CHAR *pathName, INT32 flags, MODE mode);

    using MSPROF_MMREAD_T = mmSsize_t (*)(INT32, VOID *, UINT32);
    mmSsize_t MsprofMmRead(INT32 fd, VOID *buf, UINT32 bufLen);

    using MSPROF_MMCLOSE_T = INT32 (*)(INT32);
    INT32 MsprofMmClose(INT32 fd);

    using MSPROF_MMGETERRORFORMATMESSAGE_T = CHAR * (*)(mmErrorMsg, CHAR *, mmSize);
    CHAR *MsprofMmGetErrorFormatMessage(mmErrorMsg errnum, CHAR *buf, mmSize size);

    using MSPROF_MMGETERRORCODE_T = INT32 (*)();
    INT32 MsprofMmGetErrorCode();

    using MSPROF_MMACCESS2_T = INT32 (*)(const CHAR *, INT32);
    INT32 MsprofMmAccess2(const CHAR *pathName, INT32 mode);

    using MSPROF_MMGETOPTLONG_T = INT32 (*)(INT32, CHAR *const *, const CHAR *, const mmStructOption *, INT32 *);
    INT32 MsprofMmGetOptLong(INT32 argc,
                   CHAR *const *argv,
                   const CHAR *opts,
                   const mmStructOption *longOpts,
                   INT32 *longIndex);

    using MSPROF_MMGETOPTIND_T = INT32 (*)();
    INT32 MsprofMmGetOptInd();

    using MSPROF_MMGETOPTARG_T = CHAR * (*)();
    CHAR *MsprofMmGetOptArg();

    using MSPROF_MMSLEEP_T = INT32 (*)(UINT32);
    INT32 MsprofMmSleep(UINT32 milliSecond);

    using MSPROF_MMUNLINK_T = INT32 (*)(const CHAR *);
    INT32 MsprofMmUnlink(const CHAR *filename);

    using MSPROF_MMGETTID_T = INT32 (*)();
    INT32 MsprofMmGetTid();

private:
    std::string soName_;
    PluginManager pluginManager_;
};

} // Plugin
} // Dvvp
} // Analysis
#endif