/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#include "transport.h"
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <sys/stat.h>
#include "config/config.h"
#include "errno/error_code.h"
#include "message/codec.h"
#include "message/prof_params.h"
#include "mmpa_plugin.h"
#include "msprof_dlog.h"
#include "securec.h"
#include "param_validation.h"
#include "adx_prof_api.h"
#include "proto/profiler.pb.h"

namespace analysis {
namespace dvvp {
namespace transport {
using namespace analysis::dvvp::proto;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::common::validation;
using namespace analysis::dvvp::common::utils;
using namespace Analysis::Dvvp::Plugin;

int ITransport::SendFile(const std::string &jobCtx,
                         const std::string &file,
                         const std::string &relativePath,
                         const std::string &destDir /* = "" */)
{
    UNUSED(destDir);
    static const int fileBufLen = 1 * 1024 * 1024; // 1 * 1024 * 1024 means 1mb

    SHARED_PTR_ALIA<char> buffer;
    MSVP_MAKE_SHARED_ARRAY_RET(buffer, char, fileBufLen, PROFILING_FAILED);

    long long len = analysis::dvvp::common::utils::Utils::GetFileSize(file);
    if (len < 0 || len > MSVP_LARGE_FILE_MAX_LEN) {
        MSPROF_LOGE("Failed to get size of file: %s, err=%d", Utils::BaseName(file).c_str(), static_cast<int>(errno));
        return PROFILING_FAILED;
    }
    MSPROF_LOGI("size of file: %s is %lld", Utils::BaseName(file).c_str(), len);

    INT32 fd = MmpaPlugin::instance()->MsprofMmOpen2(file.c_str(), M_RDONLY, M_IRUSR | M_IWUSR);
    if (fd == EN_ERROR || fd == EN_INVALID_PARAM) {
        MSPROF_LOGE("Failed to open: %s, err=%d", Utils::BaseName(file).c_str(), errno);
        return PROFILING_FAILED;
    }

    long long sizeRead = 0;
    long long offset = 0;
    int ret = PROFILING_SUCCESS;
    do {
        sizeRead = (long long)MmpaPlugin::instance()->MsprofMmRead(fd, buffer.get(), fileBufLen);
        if (sizeRead > 0) {
            FileChunk chunk;
            chunk.relativeFileName = relativePath;
            chunk.dataBuf = (UNSIGNED_CHAR_PTR)buffer.get();
            chunk.bufLen = static_cast<unsigned int>(sizeRead);;
            chunk.offset = offset;
            chunk.isLastChunk = ((offset + sizeRead < len) ? false : true);
            ret = SendFileChunk(jobCtx, chunk);
            if (ret != PROFILING_SUCCESS) {
                MSPROF_LOGE("Failed to send file: %s, chunk offset: %lld",
                    Utils::BaseName(file).c_str(), offset);
                break;
            }
            offset += sizeRead;
        }
    } while (sizeRead > 0);
    MmpaPlugin::instance()->MsprofMmClose(fd);

    return ret;
}

int ITransport::SendFiles(const std::string &jobId,
                          const std::string &rootDir,
                          const std::string &destDir /* = "" */)
{
    std::vector<std::string> files;
    analysis::dvvp::common::utils::Utils::GetFiles(rootDir, true, files);
    for (size_t ii = 0; ii < files.size(); ++ii) {
        auto file = files[ii];
        std::string relative_path;
        int ret = analysis::dvvp::common::utils::Utils::RelativePath(file,
                                                                     rootDir,
                                                                     relative_path);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to get relative path for: %s", Utils::BaseName(file).c_str());
            continue;
        }

        analysis::dvvp::message::JobContext jobCtx;
        jobCtx.job_id = jobId;
        ret = SendFile(jobCtx.ToString(), file, relative_path, destDir);
        MSPROF_LOGI("file:%s, relative_path:%s, destDir:%s, ret: %d", Utils::BaseName(file).c_str(),
            Utils::BaseName(relative_path).c_str(), Utils::BaseName(destDir).c_str(), ret);
    }

    return PROFILING_SUCCESS;
}

int ITransport::SendFileChunk(const std::string &jobCtx, FileChunk &chunk)
{
    SHARED_PTR_ALIA<FileChunkReq> fileChunkReq = nullptr;
    MSVP_MAKE_SHARED0_RET(fileChunkReq, FileChunkReq, PROFILING_FAILED);

    fileChunkReq->mutable_hdr()->set_job_ctx(jobCtx);
    fileChunkReq->set_filename(chunk.relativeFileName);
    fileChunkReq->set_offset(chunk.offset);
    fileChunkReq->set_chunk(chunk.dataBuf, chunk.bufLen);
    fileChunkReq->set_chunksizeinbytes(chunk.bufLen);
    fileChunkReq->set_islastchunk(chunk.isLastChunk);
    fileChunkReq->set_needack(false);
    fileChunkReq->set_datamodule(FileChunkDataModule::PROFILING_IS_CTRL_DATA);
    std::string encoded = analysis::dvvp::message::EncodeMessage(fileChunkReq);

    int len = SendBuffer(encoded.c_str(), static_cast<int>(encoded.size()));
    if (len != static_cast<int>(encoded.size())) {
        MSPROF_LOGE("sent size:%d, encoded size:%d", len, static_cast<int>(encoded.size()));
        return PROFILING_FAILED;
    }

    return PROFILING_SUCCESS;
}

int SendBufferWithFixedLength(AdxTransport &transport, CONST_VOID_PTR buffer, int length)
{
    int retLengthError = -1;
    if (buffer == nullptr) {
        MSPROF_LOGE("buffer is null");
        return retLengthError;
    }
    IdeBuffT out = nullptr;
    int outLen = 0;
    uint64_t startRawTime = analysis::dvvp::common::utils::Utils::GetClockMonotonicRaw();
    int ret = Analysis::Dvvp::Adx::AdxIdeCreatePacket(buffer, length, out, outLen);
    if (ret != IDE_DAEMON_OK) {
        MSPROF_LOGE("Failed to AdxIdeCreatePacket, err=%d.", ret);
        return retLengthError;
    }
    ret = transport.SendAdxBuffer(out, outLen);
    Analysis::Dvvp::Adx::AdxIdeFreePacket(out);
    if (ret != PROFILING_SUCCESS) {
        return retLengthError;
    }
    uint64_t endRawTime = analysis::dvvp::common::utils::Utils::GetClockMonotonicRaw();
    if (transport.perfCount_ != nullptr) {
        transport.perfCount_->UpdatePerfInfo(startRawTime, endRawTime, length);
    }
    return length;
}
}  // namespace transport
}  // namespace dvvp
}  // namespace analysis
