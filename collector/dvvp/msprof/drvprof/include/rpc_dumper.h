/**
* @file rpc_dumper.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef MSPROF_ENGINE_RPC_DUMPER_H
#define MSPROF_ENGINE_RPC_DUMPER_H

#include "data_dumper.h"
#include "receive_data.h"
#include "rpc_data_handle.h"
#include "proto/profiler.pb.h"

namespace Msprof {
namespace Engine {
using namespace analysis::dvvp::proto;
class RpcDumper : public DataDumper {
public:
    /**
    * @brief RpcDumper: the construct function
    * @param [in] module: the name of the plugin
    */
    explicit RpcDumper(const std::string &module);
    ~RpcDumper() override;

public:
    /**
    * @brief Report: API for user to report data to profiling
    * @param [in] rData: the data from user
    * @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
    */
    virtual int Report(CONST_REPORT_DATA_PTR rData);

    /**
    * @brief Start: create a TCP collection to PROFILING SERVER
    *               start a new thread to deal with data from user
    * @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
    */
    int Start();

    /**
    * @brief Stop: stop the thread to deal with data
    *              disconnect the TCP to PROFILING SERVER
    */
    int Stop();

    /**
    * @brief Flush: wait all datas to be send to remove host
    *               then send a FileChunkFlushReq data to remote host tell it data report finished
    * @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
    */
    int Flush();

    uint32_t GetReportDataMaxLen();

protected:
    virtual void WriteDone();
    /**
     * @brief Run: the thread function for deal with user data
     */
    virtual void Run(const struct error_message::Context &errorContext) override;
private:
    /**
    * @brief Dump: transfer FileChunkReq 
    * @param [in] message: the user data to be send to remote host
    * @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
    */
    virtual int Dump(std::vector<SHARED_PTR_ALIA<FileChunkReq>> &message);
    virtual void TimedTask();
    int GetNameAndId(const std::string &module);

private:
    std::string module_; // the module name, like: DATA_PREPROCESS
    std::string moduleNameWithId_; // like: DATA_PREPROCESS-80858-3
    int32_t hostPid_;
    int32_t devId_;
    SHARED_PTR_ALIA<RpcDataHandle> dataHandle_;
};
}}
#endif
