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

#ifndef MSPROF_ENGINE_UPLOADER_DUMPER_H
#define MSPROF_ENGINE_UPLOADER_DUMPER_H

#include <memory>
#include "prof_reporter.h"
#include "data_dumper.h"

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
    * @return : success return PROFILING_SUCCESS, failed return PROFILING_FAILED
    */
    int Report(CONST_REPORT_DATA_PTR rData) override;

    /**
    * @brief Start: create a TCP collection to PROFILING SERVER
    *               start a new thread to deal with data from user
    * @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
    */
    int Start() override;

    /**
    * @brief Stop: stop the thread to deal with data
    *              disconnect the TCP to PROFILING SERVER
    */
    int Stop() override;

    /**
    * @brief Flush: wait all datas to be send to remove host
    *               then send a FileChunkFlushReq data to remote host tell it data report finished
    * @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
    */
    int Flush() override;

    uint32_t GetReportDataMaxLen() override;

    /**
    * @brief DumpModelLoadData: dump cached model load data
    */
    void DumpModelLoadData(const std::string &devId) override;
    /**
    * @brief SendData: use interface dump to send data
    */
    int SendData(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunk) override;

protected:
    void WriteDone() override;
    /**
     * @brief Run: the thread function for deal with user data
     */
    void Run(const struct error_message::Context &errorContext) override;
private:
    /**
    * @brief Dump: transfer ProfileFileChunk
    * @param [in] dataChunk: the user data to be send to remote host
    * @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
    */
    virtual int Dump(std::vector<SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk>> &dataChunk);
    void TimedTask() override;
    void AddToUploader(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> dataChunk);

private:
    std::string module_; // the module name
    bool needCache_;
    static const size_t MAX_CACHE_SIZE = 1024; // cached 1024 dataChunks at most
    std::mutex mtx_;
    // model load data of Framework
    std::map<std::string, std::list<SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> > > modelLoadData_;
    // model load data cached
    std::map<std::string, std::list<SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> > > modelLoadDataCached_;
    // dynamic profiling cached message {deviceId: [dynProf_1th:FileChunkReq1, ...]}}
    std::map<std::string, std::list<std::map<uint32_t,
        SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk>>>> cachedMsg_;
};
}}
#endif
