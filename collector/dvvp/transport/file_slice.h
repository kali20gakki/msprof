/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 * Description: file slice
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_STREAMIO_COMMON_FILE_SLICE_H
#define ANALYSIS_DVVP_STREAMIO_COMMON_FILE_SLICE_H

#include <map>
#include "file_ageing.h"
#include "message/prof_params.h"
#include "proto/msprofiler.pb.h"
#include "queue/bound_queue.h"
#include "statistics/perf_count.h"
#include "thread/thread.h"

namespace analysis {
namespace dvvp {
namespace transport {
using namespace Analysis::Dvvp::Common::Statistics;
using namespace analysis::dvvp::common::utils;

class FileSlice {
public:
    FileSlice(int sliceFileMaxKByte, const std::string &storageDir, const std::string &storageLimit);
    ~FileSlice();

public:
    int Init(bool needSlice = true);
    int FileSliceFlush();
    int SaveDataToLocalFiles(SHARED_PTR_ALIA<google::protobuf::Message> message);

private:
    int FileSliceFlushByJobID(const std::string &jobIDRelative, const std::string &devID);
    int FileFlush(const std::string &sliceName, uint64_t sliceId);
    bool CreateDoneFile(const std::string &absolutePath, const std::string &fileSize,
                        const std::string &startTime, const std::string &endTime, const std::string &timeKey);
    std::string GetSliceKey(const std::string &dir, std::string &fileName);
    int SetChunkTime(const std::string &key, uint64_t startTime, uint64_t endTime);
    int WriteToLocalFiles(const std::string &key, CONST_CHAR_PTR data, int dataLen, int offset, bool isLastChunk);
    int CheckDirAndMessage(analysis::dvvp::message::JobContext &jobCtx,
                           SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> &fileChunkReq);
    int WriteCtrlDataToFile(const std::string &absolutePath, const std::string &data, int dataLen);

private:
    int sliceFileMaxKByte_;
    std::map<std::string, uint64_t> sliceNum_;
    std::map<std::string, uint64_t> totalSize_;
    std::map<std::string, uint64_t> chunkStartTime_;
    std::map<std::string, uint64_t> chunkEndTime_;
    std::mutex sliceFileMtx_;
    SHARED_PTR_ALIA<PerfCount> writeFilePerfCount_;
    std::string storageDir_;
    bool needSlice_;
    std::string storageLimit_;
    SHARED_PTR_ALIA<FileAgeing> fileAgeing_;
};
}
}
}
#endif  // _ANALYSIS_DVVP_STREAMIO_COMMON_FILE_SLICE_H
