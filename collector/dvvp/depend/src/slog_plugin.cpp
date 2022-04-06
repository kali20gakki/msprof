#include "slog_plugin.h"
#include "mmpa.h"

namespace Analysis {
namespace Dvvp {
namespace Plugin {

#define MSPROF_MODULE_NAME PROFILING

#define MSPROF_LOGD(format, ...)                                                                  \
    do {                                                                                          \
        if (SlogPlugin::instance()->MsprofCheckLogLevel(MSPROF_MODULE_NAME, DLOG_DEBUG) == 1) {   \
            SlogPlugin::instance()->MsprofDlogDebugInner(MSPROF_MODULE_NAME, "[%s:%d]" format,    \
                __FILE__, __LINE__, ##__VA_ARGS__);                                               \
        }                                                                                         \
    } while (TMP_LOG != 0)

#define MSPROF_LOGI(format, ...)
    do {                                                                                          \
        if (SlogPlugin::instance()->MsprofCheckLogLevel(MSPROF_MODULE_NAME, DLOG_INFO) == 1) {    \
            SlogPlugin::instance()->MsprofDlogInfoInner(MSPROF_MODULE_NAME, "[%s:%d]" format,     \
                __FILE__, __LINE__, ##__VA_ARGS__);                                               \
        }                                                                                         \
    } while (TMP_LOG != 0)

#define MSPROF_LOGW(format, ...)
    do {                                                                                          \
        if (SlogPlugin::instance()->MsprofCheckLogLevel(MSPROF_MODULE_NAME, DLOG_WARN) == 1) {    \
            SlogPlugin::instance()->MsprofDlogWarnInner(MSPROF_MODULE_NAME, "[%s:%d]" format,     \
                __FILE__, __LINE__, ##__VA_ARGS__);                                               \
        }                                                                                         \
    } while (TMP_LOG != 0)

#define MSPROF_LOGE(format, ...)
    do {                                                                                          \
            SlogPlugin::instance()->MsprofDlogErrorInner(MSPROF_MODULE_NAME, "[%s:%d]" format,    \
                __FILE__, __LINE__, ##__VA_ARGS__);                                               \
    } while (TMP_LOG != 0)

#define MSPROF_EVENT(format, ...)
    do {                                                                                          \
            SlogPlugin::instance()->MsprofDlogEventInner(MSPROF_MODULE_NAME, "[%s:%d]" format,    \
                __FILE__, __LINE__, ##__VA_ARGS__);                                               \
    } while (TMP_LOG != 0)

int SlogPlugin::MsprofCheckLogLevel(int moduleId, int logLevel)
{
    MSPROF_CHECKLOGLEVEL_T func;
    Status ret = pluginManager_.GetFunction<int, int>("CheckLogLevel", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(moduleId, logLevel);
}

void SlogPlugin::MsprofDlogDebugInner(int moduleId, const char *fmt, ...)
{
    MSPROF_DLOGDEBUGINNER_T func;
    Status ret = pluginManager_.GetFunction<int, const char *, ...>("DlogDebugInner", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(moduleId, fmt, ...);
}

void SlogPlugin::MsprofDlogInfoInner(int moduleId, const char *fmt, ...)
{
    MSPROF_DLOGINFOINNER_T func;
    Status ret = pluginManager_.GetFunction<int, const char *, ...>("DlogInfoInner", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(moduleId, fmt, ...);
}

void SlogPlugin::MsprofDlogWarnInner(int moduleId, const char *fmt, ...)
{
    MSPROF_DLOGWARNINNER_T func;
    Status ret = pluginManager_.GetFunction<int, const char *, ...>("DlogWarnInner", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(moduleId, fmt, ...);
}

void SlogPlugin::MsprofDlogErrorInner(int moduleId, const char *fmt, ...)
{
    MSPROF_DLOGERRORINNER_T func;
    Status ret = pluginManager_.GetFunction<int, const char *, ...>("DlogErrorInner", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(moduleId, fmt, ...);
}

void SlogPlugin::MsprofDlogEventInner(int moduleId, const char *fmt, ...)
{
    MSPROF_DLOGEVENTINNER_T func;
    Status ret = pluginManager_.GetFunction<int, const char *, ...>("DlogEventInner", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(moduleId, fmt, ...);
}

} // Plugin
} // Dvvp
} // Analysis