/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
#ifndef COLLECTOR_DYNAMIC_PROFILING_COMMON_H
#define COLLECTOR_DYNAMIC_PROFILING_COMMON_H
#include <string>

namespace Collector {
namespace Dvvp {
namespace DynProf {
const std::string DYN_PROF_SOCK_UNIX_DOMAIN  = "/dynamic_profiling_socket_";
const std::string INTERACTION_MODE_PREFIX  = "> ";
const std::string INTERACTION_MODE_CMD_START  = "start";
const std::string INTERACTION_MODE_CMD_STOP  = "stop";
const std::string INTERACTION_MODE_CMD_QUIT  = "quit";
const uint32_t DYN_PROF_REQ_MSG_MAX_LEN = 4096;
const uint32_t DYN_PROF_RSP_MSG_MAX_LEN = 128;
const uint32_t DYN_PROF_CLIENT_CONNECT_WAIT_TIME = 100000;  // 100ms
const uint32_t DYN_PROF_CLIENT_SEND_WAIT_TIME = 1;          // 30s, when stop profiling, flush data need some time
const uint32_t DYN_PROF_CLIENT_RECV_WAIT_TIME = 30;         // 30s, when stop profiling, flush data need some time
const uint32_t DYN_PROF_SERVER_ACCEPT_WAIT_TIME = 1;
const uint32_t DYN_PROF_SERVER_RECV_WAIT_TIME = 1;
const uint32_t DYN_PROF_MAX_STARTABLE_TIMES = 10000;
const uint32_t DYN_PROF_IDLE_LINK_HOLD_TIME = 1800 / DYN_PROF_SERVER_RECV_WAIT_TIME; // hold idle link 1800 seconds
const uint32_t DYN_PROF_SERVER_PROC_MSG_MAX_NUM = 100;
const uint32_t DYN_PROF_READ_INPUT_CMD_WAIT_TIME = 1;
const uint32_t DYN_PROF_MAX_ACCEPT_TIMES = 128;

enum class DynProfMsgType {
    START_REQ = 0,
    START_RSP,
    STOP_REQ,
    STOP_RSP,
    QUIT_REQ,
    QUIT_RSP,
    TEST_REQ,
    TEST_RSP,
    DISCONN_RSP,
};

enum class DynProfMsgProcRes {
    EXE_SUCC = 0,
    EXE_FAIL,
};

struct DynProfReqMsg {
    DynProfMsgType msgType;
    uint32_t msgDataLen;
    char msgData[DYN_PROF_REQ_MSG_MAX_LEN];
};

struct DynProfRspMsg {
    DynProfMsgType msgType;
    DynProfMsgProcRes statusCode;
    uint32_t msgDataLen;
    char msgData[DYN_PROF_RSP_MSG_MAX_LEN];
};
}
}
}
#endif // COLLECTOR_DYNAMIC_PROFILING_COMMON_H