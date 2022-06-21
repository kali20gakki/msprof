/**
* @file uploader_mgr.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ANALYSIS_DVVP_TRANSPORT_UPLOADER_MGR_H
#define ANALYSIS_DVVP_TRANSPORT_UPLOADER_MGR_H
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
    int UploadFileData(const std::string &id,
                       const std::string &data,
                       const struct FileDataParams &fileDataParams,
                       SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx);
    void AddMapByDevIdMode(int devId, const std::string &mode, const std::string &jobId);
    std::string GetJobId(int devId, const std::string &mode);

private:
    std::map<std::string, SHARED_PTR_ALIA<Uploader>> uploaderMap_;
    std::mutex uploaderMutex_;
    std::map<std::string, std::string> devModeJobMap_;
};
}
}
}
#endif

