/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: dynamic profiling manager
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-12-10
 */
#ifndef COLLECTOR_DYNAMIC_PROFILING_COMMON_H
#define COLLECTOR_DYNAMIC_PROFILING_COMMON_H
 
namespace Collector {
namespace Dvvp {
namespace DynProf {
const std::string DYN_PROF_SOCK_UNIX_DOMAIN  = "dynamic_profiling_socket_pid_";
const uint32_t DYN_PROF_PARAMS_MAX_LEN = 4096;
 
enum class DynProfMsgType {
    START_REQ = 0,
    START_RSQ,
    STOP_REQ,
    STOP_RSQ,
    QUIT_REQ,
    QUIT_RSQ,
};
 
enum class DynProfMsgRsqCode {
    RSQ_SUCCESS = 0,
    RSQ_FAIL,
};
 
struct DynProfMsg {
    DynProfMsgType msgType;
    uint32_t msgDataLen;
    union {
        DynProfMsgRsqCode statusCode;
        char msgData[DYN_PROF_PARAMS_MAX_LEN];
    };
};
}
}
}
#endif // COLLECTOR_DYNAMIC_PROFILING_COMMON_H