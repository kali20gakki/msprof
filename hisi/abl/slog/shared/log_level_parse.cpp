/**
 * @log_level_parse.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "log_level_parse.h"
#ifdef __cplusplus
#ifndef LOG_CPP
extern "C" {
#endif // LOG_CPP
#endif // __cplusplus
#define MOUDLE_NAME_ID_LEVEL_MAP(x, y) \
    { \
#x, x, y \
    }

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

// module id order should follow slog.h, or error will occur
static ModuleInfo g_moduleInfo[] = {
    MOUDLE_NAME_ID_LEVEL_MAP(SLOG, MODULE_INIT_LOG_LEVEL),          // Slog
    MOUDLE_NAME_ID_LEVEL_MAP(IDEDD, MODULE_INIT_LOG_LEVEL),         // ascend debug device agent
    MOUDLE_NAME_ID_LEVEL_MAP(IDEDH, MODULE_INIT_LOG_LEVEL),         // ascend debug agent
    MOUDLE_NAME_ID_LEVEL_MAP(HCCL, MODULE_INIT_LOG_LEVEL),          // HCCL
    MOUDLE_NAME_ID_LEVEL_MAP(FMK, MODULE_INIT_LOG_LEVEL),           // Framework
    MOUDLE_NAME_ID_LEVEL_MAP(HIAIENGINE, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(DVPP, MODULE_INIT_LOG_LEVEL),          // DVPP
    MOUDLE_NAME_ID_LEVEL_MAP(RUNTIME, MODULE_INIT_LOG_LEVEL),       // Runtime
    MOUDLE_NAME_ID_LEVEL_MAP(CCE, MODULE_INIT_LOG_LEVEL),           // CCE
#if (OS_TYPE_DEF == LINUX)
    MOUDLE_NAME_ID_LEVEL_MAP(HDC, MODULE_INIT_LOG_LEVEL),           // HDC
#else
    MOUDLE_NAME_ID_LEVEL_MAP(HDCL, MODULE_INIT_LOG_LEVEL),          // HDCL
#endif
    MOUDLE_NAME_ID_LEVEL_MAP(DRV, MODULE_INIT_LOG_LEVEL),           // Driver
    MOUDLE_NAME_ID_LEVEL_MAP(MDCFUSION, MODULE_INIT_LOG_LEVEL),     // Mdc fusion
    MOUDLE_NAME_ID_LEVEL_MAP(MDCLOCATION, MODULE_INIT_LOG_LEVEL),   // Mdc location
    MOUDLE_NAME_ID_LEVEL_MAP(MDCPERCEPTION, MODULE_INIT_LOG_LEVEL), // Mdc perception
    MOUDLE_NAME_ID_LEVEL_MAP(MDCFSM, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(MDCCOMMON, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(MDCMONITOR, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(MDCBSWP, MODULE_INIT_LOG_LEVEL),       // MDC basesoftware platform
    MOUDLE_NAME_ID_LEVEL_MAP(MDCDEFAULT, MODULE_INIT_LOG_LEVEL),    // MDC UNDEFINE
    MOUDLE_NAME_ID_LEVEL_MAP(MDCSC, MODULE_INIT_LOG_LEVEL),         // MDC spatial cognition
    MOUDLE_NAME_ID_LEVEL_MAP(MDCPNC, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(MLL, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(DEVMM, MODULE_INIT_LOG_LEVEL),         // Dlog memory managent
    MOUDLE_NAME_ID_LEVEL_MAP(KERNEL, MODULE_INIT_LOG_LEVEL),        // Kernel
    MOUDLE_NAME_ID_LEVEL_MAP(LIBMEDIA, MODULE_INIT_LOG_LEVEL),      // Libmedia
    MOUDLE_NAME_ID_LEVEL_MAP(CCECPU, MODULE_INIT_LOG_LEVEL),        // ai cpu
    MOUDLE_NAME_ID_LEVEL_MAP(ASCENDDK, MODULE_INIT_LOG_LEVEL),      // AscendDK
    MOUDLE_NAME_ID_LEVEL_MAP(ROS, MODULE_INIT_LOG_LEVEL),           // ROS
    MOUDLE_NAME_ID_LEVEL_MAP(HCCP, MODULE_INIT_LOG_LEVEL),          // HCCP
    MOUDLE_NAME_ID_LEVEL_MAP(ROCE, MODULE_INIT_LOG_LEVEL),          // RoCE
    MOUDLE_NAME_ID_LEVEL_MAP(TEFUSION, MODULE_INIT_LOG_LEVEL),      // TE
    MOUDLE_NAME_ID_LEVEL_MAP(PROFILING, MODULE_INIT_LOG_LEVEL),     // Profiling
    MOUDLE_NAME_ID_LEVEL_MAP(DP, MODULE_INIT_LOG_LEVEL),            // Data Preprocess
    MOUDLE_NAME_ID_LEVEL_MAP(APP, MODULE_INIT_LOG_LEVEL),           // User Application call HIAI_ENGINE_LOG
    MOUDLE_NAME_ID_LEVEL_MAP(TS, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(TSDUMP, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(AICPU, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(LP, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(TDT, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(FE, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(MD, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(MB, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(ME, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(IMU, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(IMP, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(GE, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(MDCFUSA, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(CAMERA, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(ASCENDCL, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(TEEOS, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(ISP, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(SIS, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(HSM, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(DSS, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(PROCMGR, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(BBOX, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(AIVECTOR, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(TBE, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(FV, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(MDCMAP, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(TUNE, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(HSS, MODULE_INIT_LOG_LEVEL),
    MOUDLE_NAME_ID_LEVEL_MAP(FFTS, MODULE_INIT_LOG_LEVEL),
    {NULL, -1, -1}
};

// level order should follow DLOG_XXX value, or error will occur
#define LEVEL_NAME_ARRAY_LENGTH (DLOG_EVENT + 1)

#if (OS_TYPE_DEF == LINUX)
#ifdef IAM
#define GLOABLE_DEFAULT_LOG_LEVEL (DLOG_ERROR)
#define MODULE_DEFAULT_LOG_LEVEL (DLOG_ERROR)
#else
#define GLOABLE_DEFAULT_LOG_LEVEL (DLOG_INFO)
#define MODULE_DEFAULT_LOG_LEVEL (DLOG_INFO)
#endif
#else
#define GLOABLE_DEFAULT_LOG_LEVEL (DLOG_INFO)
#define MODULE_DEFAULT_LOG_LEVEL (DLOG_INFO)
#endif

static ModuleInfo g_levelName[LEVEL_NAME_ARRAY_LENGTH] = {
    { "DEBUG",   DLOG_DEBUG,    DLOG_DEBUG }, // DLOG_DEBUG refer to 0
    { "INFO",    DLOG_INFO,     DLOG_INFO },
    { "WARNING", DLOG_WARN,     DLOG_WARN },
    { "ERROR",   DLOG_ERROR,    DLOG_ERROR },
    { "NULL",    DLOG_NULL,     DLOG_NULL },
    { "TRACE",   DLOG_TRACE,    DLOG_TRACE },
    { "OPLOG",   DLOG_OPLOG,    DLOG_OPLOG },
    { NULL,      -1,            -1 },
    { NULL,      -1,            -1 },
    { NULL,      -1,            -1 },
    { NULL,      -1,            -1 },
    { NULL,      -1,            -1 },
    { NULL,      -1,            -1 },
    { NULL,      -1,            -1 },
    { NULL,      -1,            -1 },
    { NULL,      -1,            -1 },
    { "EVENT",   DLOG_EVENT,    DLOG_EVENT } // DLOG_EVENT refer to 16(0x10)
};

/**
* @brief SetLevelByModuleId: set module loglevel by moduleId
* @param [in]moduleId: module id
* @param [in]value: module log level
* @return: TRUE: SUCCEED, FALSE: FAILED
*/
int SetLevelByModuleId(const int moduleId, int value)
{
    ModuleInfo *set = g_moduleInfo;
    if ((moduleId >= 0) && (moduleId < INVLID_MOUDLE_ID)) {
        set[moduleId].moduleLevel = value;
        return TRUE;
    }
    return FALSE;
}

/**
* @brief SetLevelToAllModule: set all module level
* @param [in]level: module log level
* @return: void
*/
void SetLevelToAllModule(const int level)
{
    ModuleInfo *set = g_moduleInfo;
    for (; set->moduleName != nullptr; set++) {
        set->moduleLevel = level;
    }
}

/**
* @brief GetLevelByModuleId: get module loglevel by moduleId
* @param [in]moduleId: module id
* @return: module log level
*/
int GetLevelByModuleId(const int moduleId)
{
    const ModuleInfo *set = g_moduleInfo;
    if ((moduleId >= 0) && (moduleId < INVLID_MOUDLE_ID)) {
        return set[moduleId].moduleLevel;
    }
    return MODULE_DEFAULT_LOG_LEVEL;
}

/**
* @brief GetModuleInfos: get g_moduleInfo
* @return: g_moduleInfo
*/
const ModuleInfo *GetModuleInfos()
{
    return (const ModuleInfo *)g_moduleInfo;
}

static const ModuleInfo *GetInfoByName(const char *name, const ModuleInfo *set)
{
    if (name == nullptr) {
        return nullptr;
    }
    for (; (set != nullptr) && (set->moduleName != nullptr); set++) {
        if (strcmp(name, set->moduleName) == 0) {
            return set;
        }
    }
    return nullptr;
}
/**
* @brief GetModuleInfoByName: get module info by module name
* @param [in]name: module name
* @return: module info struct
*/
const ModuleInfo *GetModuleInfoByName(const char *name)
{
    return GetInfoByName(name, g_moduleInfo);
}

/**
* @brief GetLevelInfoByName: get level info by level name
* @param [in]name: level name
* @return: level info struct
*/
const ModuleInfo *GetLevelInfoByName(const char *name)
{
    return GetInfoByName(name, g_levelName);
}

/**
* @brief GetModuleNameById: get module name by module id
* @param [in]moduleId: module id
* @return: module name
*/
const char *GetModuleNameById(const int moduleId)
{
    if ((moduleId >= 0) && (moduleId < INVLID_MOUDLE_ID)) {
        return g_moduleInfo[moduleId].moduleName;
    }
    return nullptr;
}

/**
* @brief GetLevelNameById: get level name by level
* @param [in]level: log level, include debug, info, warning, error, null, trace, oplog, event
* @return: level name
*/
const char *GetLevelNameById(const int level)
{
    if (((level >= DLOG_DEBUG) && (level <= DLOG_OPLOG)) || (level == DLOG_EVENT)) {
        return g_levelName[level].moduleName;
    }
    return nullptr;
}

/**
* @brief GetBasicLevelNameById: get level name by level
* @param [in]level: log level, include debug, info, warning, error, null
* @return: level name
*/
const char *GetBasicLevelNameById(const int level)
{
    if ((level >= DLOG_DEBUG) && (level <= DLOG_NULL)) {
        return g_levelName[level].moduleName;
    }
    return "INVALID";
}

#ifdef __cplusplus
#ifndef LOG_CPP
}
#endif // LOG_CPP
#endif // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

const ModuleInfo *GetModuleInfoByNameForC(const char *name)
{
    return GetModuleInfoByName(name);
}

#ifdef __cplusplus
}
#endif
