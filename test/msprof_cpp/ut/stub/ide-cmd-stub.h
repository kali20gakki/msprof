#ifndef _ANALYSIS_DVVP_UT_IDE_CMD_STUB_H_
#define _ANALYSIS_DVVP_UT_IDE_CMD_STUB_H_

#include "hdc-common/ide_daemon_sock.h"

#ifdef __cplusplus
extern "C" {
#endif
//int CommandRes(int sock, int cmd_or_file);
int CommandRes(struct IdeSockHandle, int cmd_or_file);
#ifdef __cplusplus
}
#endif
#endif
