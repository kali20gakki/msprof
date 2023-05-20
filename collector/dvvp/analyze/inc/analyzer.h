/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2023. All rights reserved.
 * Description: handle profiling data
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2020-10-24
 */
#ifndef ANALYSIS_DVVP_ANALYZE_ANALYZER_H
#define ANALYSIS_DVVP_ANALYZE_ANALYZER_H

#include <map>

#include "data_struct.h"
#include "utils/utils.h"
#include "transport/uploader.h"
#include "analyzer_ge.h"
#include "analyzer_hwts.h"
#include "analyzer_ffts.h"
#include "analyzer_ts.h"
#include "analyzer_rt.h"
#include "analyzer_base.h"

namespace Analysis {
namespace Dvvp {
namespace Analyze {
using namespace analysis::dvvp::common::utils;
class Analyzer {
public:
    explicit Analyzer(SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> uploader);
    ~Analyzer();

public:
    int Init();
    void OnNewData(CONST_VOID_PTR data, uint32_t len);
    void Flush();
    void SetDevId(const std::string &devIdStr);
    void PrintStats();

private:
    void DispatchData(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message);
    void DispatchOptimizeData(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message);
    void ConstructAndUploadData(const std::string &opId, OpTime &opTime);
    void TsDataPostProc();
    void HwtsDataPostProc();
    void UploadAppOp(std::multimap<std::string, OpTime> &opTimes);
    void UploadKeypointOp();
    bool IsNeedUpdateIndexId();
    void UpdateOpIndexId(std::multimap<std::string, OpTime> &opTimes);
    uint64_t GetOpIndexId(uint64_t opTimeStamp);
    void FftsDataPostProc();

    void PrintDeviceStats();
    void PrintHostStats();
    void UploadProfOpDescProc();

private:
    bool inited_;
    std::string devIdStr_;
    uint64_t resultCount_;
    uint32_t profileMode_;
    bool flushedChannel_;
    int64_t flushQueueLen_;
    SHARED_PTR_ALIA<AnalyzerGe> analyzerGe_;
    SHARED_PTR_ALIA<AnalyzerHwts> analyzerHwts_;
    SHARED_PTR_ALIA<AnalyzerTs> analyzerTs_;
    SHARED_PTR_ALIA<AnalyzerFfts> analyzerFfts_;
    SHARED_PTR_ALIA<AnalyzerRt> analyzerRt_;
    SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> uploader_;
};
}  // namespace Analyze
}  // namespace Dvvp
}  // namespace Analysis

#endif
