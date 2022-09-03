/**
* @file uploader_dumper.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef MSPROF_ENGINE_UPLOADER_DUMPER_H
#define MSPROF_ENGINE_UPLOADER_DUMPER_H

#include <memory>
#include "prof_reporter.h"
#include "data_dumper.h"

namespace analysis {
namespace dvvp {
namespace proto {
class FileChunkReq;
}}}

namespace Msprof {
namespace Engine {
class UploaderDumper : public DataDumper {
public:
    /**
    * @brief UploaderDumper: the construct function
    * @param [in] module: the name of the plugin
    */
    explicit UploaderDumper(const std::string &module);
    virtual ~UploaderDumper();

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

    /**
    * @brief DumpModelLoadData: dump cached model load data
    */
    void DumpModelLoadData(const std::string &devId) override;
    /**
    * @brief SendData: use interface dump to send data
    */
    int SendData(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> fileChunk) override;

protected:
    virtual void WriteDone();
    /**
     * @brief Run: the thread function for deal with user data
     */
    void Run(const struct error_message::Context &errorContext) override;
private:
    /**
    * @brief Dump: transfer FileChunkReq
    * @param [in] message: the user data to be send to remote host
    * @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
    */
    virtual int Dump(std::vector<SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq>> &message);
    virtual void TimedTask();
    void AddToUploader(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message);
    void SaveModelLoadData(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message);

private:
    std::string module_; // the module name
    bool needCache_;
    static const size_t MAX_CACHE_SIZE = 1024; // cached 1024 messages at most
    std::mutex mtx_;
    // model load data of Framework
    std::map<std::string, std::list<SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> > > modelLoadData_;
    // model load data cached
    std::map<std::string, std::list<SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> > > modelLoadDataCached_;
};
}}
#endif
