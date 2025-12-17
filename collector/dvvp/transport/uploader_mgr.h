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
#ifndef ANALYSIS_DVVP_TRANSPORT_UPLOADER_MGR_H
#define ANALYSIS_DVVP_TRANSPORT_UPLOADER_MGR_H
#include <atomic>
#include "cstdint"
#include "config/config.h"
#include "message/prof_params.h"
#include "singleton/singleton.h"
#include "uploader.h"

namespace analysis {
namespace dvvp {
namespace transport {
struct FileDataParams {
    std::string fileName;
    bool isLastChunk;
    analysis::dvvp::common::config::FileChunkDataModule mode;
    FileDataParams(const std::string &fileNameStr,
                   bool isLastChunkB,
                   analysis::dvvp::common::config::FileChunkDataModule modeE)
        : fileName(fileNameStr), isLastChunk(isLastChunkB), mode(modeE)
    {
    }
};
class UploaderMgr : public analysis::dvvp::common::singleton::Singleton<UploaderMgr> {
public:
    UploaderMgr();
    virtual ~UploaderMgr();

    int Init() const;
    int Uninit();

    int CreateUploader(const std::string &id, SHARED_PTR_ALIA<ITransport> transport,
        size_t queueSize = analysis::dvvp::common::config::UPLOADER_QUEUE_CAPACITY);
    void AddUploader(const std::string &id, SHARED_PTR_ALIA<Uploader> uploader);
    void GetUploader(const std::string &id, SHARED_PTR_ALIA<Uploader> &uploader);
    void DelUploader(const std::string &id);
    void DelAllUploader();

    int UploadData(const std::string &id, CONST_VOID_PTR data, uint32_t dataLen);
    int UploadData(const std::string &id, SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunkReq);
    int UploadCtrlFileData(const std::string &id,
                       const std::string &data,
                       const struct FileDataParams &fileDataParams,
                       SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx);
    void AddMapByDevIdMode(int devId, const std::string &mode, const std::string &jobId);
    std::string GetJobId(int devId, const std::string &mode);
    void SetUploaderStatus(bool status);

private:
    bool IsUploadDataStart(const std::string& id, const std::string& fileName);

private:
    std::map<std::string, SHARED_PTR_ALIA<Uploader>> uploaderMap_;
    std::mutex uploaderMutex_;
    std::map<std::string, std::string> devModeJobMap_;
    std::atomic<bool> isStarted_;
};
}
}
}
#endif

