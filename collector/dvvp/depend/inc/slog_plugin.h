#ifndef SLOG_PLUGIN_H
#define SLOG_PLUGIN_H


#include <cstdio>
#include <cstring>
#include "mmpa_api.h"
#include "slog.h"

#if (defined(linux) || defined(__linux__))
#include <syslog.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MSPROF_MODULE_NAME PROFILING

namespace Analysis {
namespace Dvvp {
namespace Plugin {
class SlogPlugin : public analysis::dvvp::common::singleton::Singleton<SlogPlugin> {
public:
    SlogPlugin()
     :load_(false),
     :soName_("libslog.so"),
      pluginManager_(PluginManager(soName_))
    {}
    ~SlogPlugin() = default;

    // int CheckLogLevel(int moduleId, int logLevel);
    using MSPROF_CHECKLOGLEVEL_T = int (*)(int, int);
    int MsprofCheckLogLevel(int moduleId, int logLevel);

    // void DlogDebugInner(int moduleId, const char *fmt, ...);
    using MSPROF_DLOGDEBUGINNER_T = void (*)(int, const char *, ...);
    void MsprofDlogDebugInner(int moduleId, const char *fmt, ...);

    // void DlogInfoInner(int moduleId, const char *fmt, ...);
    using MSPROF_DLOGINFOINNER_T = void (*)(int, const char *, ...);
    void MsprofDlogInfoInner(int moduleId, const char *fmt, ...);

    // void DlogWarnInner(int moduleId, const char *fmt, ...);
    using MSPROF_DLOGWARNINNER_T = void (*)(int, const char *, ...);
    void MsprofDlogWarnInner(int moduleId, const char *fmt, ...);

    // void DlogErrorInner(int moduleId, const char *fmt, ...);
    using MSPROF_DLOGERRORINNER_T = void (*)(int, const char *, ...);
    void MsprofDlogErrorInner(int moduleId, const char *fmt, ...);

    // void DlogEventInner(int moduleId, const char *fmt, ...);
    using MSPROF_DLOGEVENTINNER_T = void (*)(int, const char *, ...);
    void MsprofDlogEventInner(int moduleId, const char *fmt, ...);

private:
    bool load_;
    std::string soName_;
    PluginManager pluginManager_;
};

} // Plugin
} // Dvvp
} // Analysis

#ifdef __cplusplus
}
#endif

#endif