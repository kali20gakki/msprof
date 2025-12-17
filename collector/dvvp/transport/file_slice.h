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
#ifndef ANALYSIS_DVVP_STREAMIO_COMMON_FILE_SLICE_H
#define ANALYSIS_DVVP_STREAMIO_COMMON_FILE_SLICE_H

#include <map>
#include "file_ageing.h"
#include "message/prof_params.h"
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
    int SaveDataToLocalFiles(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunkReq);

private:
    int FileSliceFlushByJobID(const std::string &jobIDRelative, const std::string &devID);
    int FileFlush(const std::string &sliceName, uint64_t sliceId);
    bool CreateDoneFile(const std::string &absolutePath, const std::string &fileSize,
                        const std::string &startTime, const std::string &endTime, const std::string &timeKey);
    std::string GetSliceKey(const std::string &dir, std::string &fileName);
    int SetChunkTime(const std::string &key, uint64_t startTime, uint64_t endTime);
    int WriteToLocalFiles(const std::string &key, CONST_CHAR_PTR data, int dataLen, int offset, bool isLastChunk);
    int CheckDirAndMessage(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunkReq);
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
