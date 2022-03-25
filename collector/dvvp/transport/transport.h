/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_COMMON_TRANSPORT_H
#define ANALYSIS_DVVP_COMMON_TRANSPORT_H

#include <condition_variable>
#include <memory>
#include <google/protobuf/message.h>
#include "hdc_api.h"
#include "statistics/perf_count.h"
#include "utils/utils.h"

namespace analysis {
namespace dvvp {
namespace transport {
using namespace Analysis::Dvvp::Common::Statistics;
using namespace analysis::dvvp::common::utils;
using TLV_REQ_PTR = struct tlv_req *;
using CONST_TLV_REQ_PTR = const struct tlv_req *;
using TLV_REQ_2PTR = struct tlv_req **;
struct FileChunk {
    std::string
    relativeFileName;   // from subpath begin; For example: subA/subB/example.txt; Note: the begin don't has '/';
    unsigned char *dataBuf;  // the pointer to the data
    unsigned int bufLen;     // the len of dataBuf
    long long offset;         // the begin location of the file to write; if the offset is -1, directly append data.
    int isLastChunk;        // = 1, the last chunk of the file; != 1, not the last chunk of the file
    FileChunk() : dataBuf(nullptr), bufLen(0), offset(0), isLastChunk(0)
    {
    }
};

class ITransport {
public:
    explicit ITransport() : perfCount_(nullptr) {}
    virtual ~ITransport() {}

public:
    virtual int SendFiles(const std::string &jobId,
                          const std::string &rootDir,
                          const std::string &destDir = "");
    virtual int SendFile(const std::string &jobCtx,
                         const std::string &file,
                         const std::string &relativePath,
                         const std::string &destDir = "");
    virtual int SendFileChunk(const std::string &jobCtx, FileChunk &chunk);
    virtual int SendBuffer(CONST_VOID_PTR buffer, int length) = 0;
    virtual int CloseSession() = 0;
    virtual void WriteDone() = 0;

public:
    SHARED_PTR_ALIA<PerfCount> perfCount_; // calculate statistics
};

class AdxTransport : public ITransport {
public:
    int SendBuffer(CONST_VOID_PTR buffer, int length) override = 0;
    int CloseSession() override = 0;
    void WriteDone() override {}
    virtual int SendAdxBuffer(IdeBuffT out, int outLen) = 0;
    virtual int RecvPacket(TLV_REQ_2PTR packet) = 0;
    virtual void DestroyPacket(TLV_REQ_PTR packet) = 0;
};

int SendBufferWithFixedLength(AdxTransport &transport, CONST_VOID_PTR buffer, int length);

class TransportFactory {
public:
    TransportFactory() {}
    virtual ~TransportFactory() {}

public:
    SHARED_PTR_ALIA<ITransport> CreateIdeTransport(IDE_SESSION session);
};
}  // namespace transport
}  // namespace dvvp
}  // namespace analysis

#endif
