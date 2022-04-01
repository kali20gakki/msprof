/**
 * @klogd.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef SKLOGD_H
#define SKLOGD_H

// syslog levels highest to lowest priority
#define SKLOG_EMERG 0 // system is unusable
#define SKLOG_ALERT 1 // action must be taken immediately
#define SKLOG_CRIT 2 // critical conditions
#define SKLOG_ERROR 3 // error conditions
#define SKLOG_WARNING 4 // warning conditions
#define SKLOG_NOTICE 5 // normal but significant condition
#define SKLOG_INFO 6 // informational
#define SKLOG_DEBUG 7 // debug-level messages
#define SKLOG_PRIMASK 0x07  // mask to extract priority part (internal)
#define UNIT_US_TO_S 1000000

void OpenKernelLog(void);
void KLogdSetLogLevel(int lvl);
int ReadKernelLog(char *bufP, int len);
void CloseKernelLog(void);
void ParseArgv(int argc, const char **argv);
void ProcKernelLog(void);
void KLogToSLog(unsigned int priority, const char *msg);
void ParseKernelLog(const char *start);
int ProcessBuf(char *msg, unsigned int length);
void DecodeMsg(char *msg, unsigned int length);
unsigned int GetChValue(char ch);
int IsDigit(char c);
int CheckProessBufParm(const char *msg, unsigned int length, char **heapBuf);
#endif