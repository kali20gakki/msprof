/**
 * @log_level_parse.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef LOG_LEVEL_PARSE_H
#define LOG_LEVEL_PARSE_H
#include <stddef.h>
#include <string.h>
#include "slog.h"
#ifdef __cplusplus
#ifndef LOG_CPP
extern "C" {
#endif // LOG_CPP
#endif // __cplusplus
#define MODULE_INIT_LOG_LEVEL 5
typedef struct tagModuleInfo {
    const char *moduleName;
    const int moduleId;
    int moduleLevel;
} ModuleInfo;


int SetLevelByModuleId(const int moduleId, int value);
void SetLevelToAllModule(const int level);
int GetLevelByModuleId(const int moduleId);
const ModuleInfo *GetModuleInfos(void);
const char *GetModuleNameById(const int moduleId);
const char *GetLevelNameById(const int level);
const char *GetBasicLevelNameById(const int level);
const ModuleInfo *GetModuleInfoByName(const char *name);
const ModuleInfo *GetLevelInfoByName(const char *name);

typedef enum {
    SET_LOGLEVEL,
    GET_LOGLEVEL,
    EXPORT_BBOX_LOGS_DEVICE_EVENT,
    EXPORT_BBOX_LOGS_DEVICE_EVENT_MAINTENANCE_INFORMATION,
    EXPORT_SPECIFIC_LOG
} LogOperatonType;
#ifdef __cplusplus
#ifndef LOG_CPP
}
#endif // LOG_CPP
#endif // __cplusplus
#endif /* LOG_LEVEL_PARSE_H */

#ifdef __cplusplus
extern "C" {
#endif

const ModuleInfo *GetModuleInfoByNameForC(const char *name);

#ifdef __cplusplus
}
#endif
