/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: file transport
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2020-10-07
 */
#ifndef ANALYSIS_DVVP_COMMON_FILE_TRANSPORT_H
#define ANALYSIS_DVVP_COMMON_FILE_TRANSPORT_H

#include "file_slice.h"
#include "utils/utils.h"
#include "transport.h"

namespace analysis {
namespace dvvp {
namespace transport {
class FILETransport : public ITransport {
public:
    // using the existing session
    explicit FILETransport(const std::string &storageDir, const std::string &storageLimit);
    ~FILETransport() override;

public:
    int SendBuffer(CONST_VOID_PTR buffer, int length) override;
    int SendBuffer(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunkReq) override;
    int CloseSession() override;
    void WriteDone() override;
    int Init();
    void SetAbility(bool needSlice);

private:
    int UpdateFileName(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunkReq,
        const std::string &devId) const;
    int32_t ParseTlvChunk(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunkReq);
    int32_t SaveChunk(const char *data, SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunk) const;

private:
    SHARED_PTR_ALIA<FileSlice> fileSlice_;
    std::string storageDir_;
    std::string storageLimit_;
    bool needSlice_;
    std::map<std::string, std::string> channelBuffer_;
};

class FileTransportFactory {
public:
    FileTransportFactory() {}
    virtual ~FileTransportFactory() {}

public:
    SHARED_PTR_ALIA<ITransport> CreateFileTransport(const std::string &storageDir, const std::string &storageLimit,
        bool needSlice);
};
}  // namespace transport
}  // namespace dvvp
}  // namespace analysis

#endif
