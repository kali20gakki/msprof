/**
 * @log_monitor.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef LOG_MONITOR_H
#define LOG_MONITOR_H
#define SLOGD_MONITOR_FLAG 0
#define LOGDAEMON_MONITOR_FLAG 1
#define SKLOGD_MONITOR_FLAG 2

void LogMonitorStop(void);
void LogMonitorInit(unsigned int flagLog);
int LogMonitorRegister(unsigned int flagLog);
#endif
