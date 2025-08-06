/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 * Description: file transport
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2018-06-13
 */
#include "file_transport.h"
#include "config/config.h"
#include "errno/error_code.h"
#include "mmpa_api.h"
#include "msprof_dlog.h"
#include "securec.h"
#include "param_validation.h"

namespace analysis {
namespace dvvp {
namespace transport {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::common::validation;
using namespace analysis::dvvp::common::utils;

FILETransport::FILETransport(const std::string &storageDir, const std::string &storageLimit)
    : fileSlice_(nullptr), storageDir_(storageDir), storageLimit_(storageLimit), needSlice_(true)
{
}

FILETransport::~FILETransport() {}

int FILETransport::Init()
{
    const int sliceFileMaxKbyte = 2048; // size is about 2MB per file
    MSVP_MAKE_SHARED3_RET(fileSlice_, FileSlice, sliceFileMaxKbyte,
                          storageDir_, storageLimit_, PROFILING_FAILED);
    if (fileSlice_->Init(needSlice_) != PROFILING_SUCCESS) {
        MSPROF_LOGE("file slice init failed.");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

void FILETransport::SetAbility(bool needSlice)
{
    needSlice_ = needSlice;
}

void FILETransport::WriteDone()
{
    fileSlice_->FileSliceFlush();
}

int FILETransport::SendBuffer(CONST_VOID_PTR /* buffer */, int /* length */)
{
    MSPROF_LOGW("No need to send buffer");
    return PROFILING_SUCCESS;
}

int FILETransport::SendBuffer(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunkReq)
{
    if (fileChunkReq == nullptr) {
        MSPROF_LOGW("Failed to parse fileChunkReq");
        return PROFILING_FAILED;
    }

    if (fileChunkReq->chunkModule == FileChunkDataModule::PROFILING_IS_FROM_DEVICE ||
        fileChunkReq->chunkModule == FileChunkDataModule::PROFILING_IS_FROM_MSPROF ||
        fileChunkReq->chunkModule == FileChunkDataModule::PROFILING_IS_FROM_MSPROF_DEVICE ||
        fileChunkReq->chunkModule == FileChunkDataModule::PROFILING_IS_FROM_MSPROF_HOST) {
        if (fileChunkReq->extraInfo.empty()) {
            MSPROF_LOGE("FileChunk info is empty in SendBuffer.");
            return PROFILING_FAILED;
        }
        if (fileChunkReq->fileName.find("adprof.data") != std::string::npos) {
            return ParseTlvChunk(fileChunkReq);
        }
        std::string devId = Utils::GetInfoSuffix(fileChunkReq->extraInfo);
        std::string jobId = Utils::GetInfoPrefix(fileChunkReq->extraInfo);
        if (UpdateFileName(fileChunkReq, devId) != PROFILING_SUCCESS) {
            if (jobId.compare("64") == 0) {
                return PROFILING_SUCCESS;
            }
            MSPROF_LOGE("Failed to update file name");
            return PROFILING_FAILED;
        }
    }
    if (fileSlice_->SaveDataToLocalFiles(fileChunkReq) != PROFILING_SUCCESS) {
        MSPROF_LOGE("write data to local files failed, fileName: %s", fileChunkReq->fileName.c_str());
        return PROFILING_FAILED;
    }

    return PROFILING_SUCCESS;
}

int FILETransport::CloseSession()
{
    return PROFILING_SUCCESS;
}

int FILETransport::UpdateFileName(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunkReq,
    const std::string &devId) const
{
    std::string fileName = fileChunkReq->fileName;
    size_t pos = fileName.find_last_of("/\\");
    if (pos != std::string::npos && pos + 1 < fileName.length()) {
        fileName = fileName.substr(pos + 1, fileName.length());
    }
    std::string tag = Utils::GetInfoSuffix(fileName);
    std::string fileNameOri = Utils::GetInfoPrefix(fileName);
    if (fileChunkReq->chunkModule != FileChunkDataModule::PROFILING_IS_FROM_MSPROF_HOST) {
        if (!ParamValidation::instance()->CheckDeviceIdIsValid(devId)) {
            MSPROF_LOGW("dev_id is not valid!");
            return PROFILING_FAILED;
        }
        if (tag.length() == 0 || tag == "null") {
            fileName = fileNameOri.append(".").append(devId);
        } else {
            fileName = fileName.append(".").append(devId);
        }
    } else {
        if (tag.length() == 0 || tag == "null") {
            fileName = fileNameOri;
        }
    }
    fileChunkReq->fileName = "data" + std::string(MSVP_SLASH) + fileName;
    return PROFILING_SUCCESS;
}

/**
 * @brief parse data block that contains multiple tlv chunk for adprof, and save to target file
 * @param [in] fileChunkReq: ProfileFileChunk type shared_ptr
 * @return 0:SUCCESS, !0:FAILED
 */
int32_t FILETransport::ParseTlvChunk(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunkReq)
{
    SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunk;
    MSVP_MAKE_SHARED0_RET(fileChunk, analysis::dvvp::ProfileFileChunk, PROFILING_FAILED);
    fileChunk->extraInfo = fileChunkReq->extraInfo;

    const uint32_t structSize = sizeof(ProfTlv);
    const char *data = fileChunkReq->chunk.data();
    std::string &fileName = fileChunkReq->fileName;

    // If there is cached data, some data in front of the chunk belongs to the previous chunk.
    if (channelBuffer_.find(fileName) != channelBuffer_.end() && channelBuffer_[fileName].size() != 0 &&
        channelBuffer_[fileName].size() < structSize) {
        uint32_t prevDataSize = channelBuffer_[fileName].size();
        uint32_t leftDataSize = structSize - prevDataSize;
        if (fileChunkReq->chunkSize < leftDataSize) {
            MSPROF_LOGE("fileChunk size smaller than expected, expected minimum size %u, received size %zu",
                leftDataSize, fileChunkReq->chunkSize);
            return PROFILING_FAILED;
        }
        channelBuffer_[fileName].append(data, leftDataSize);
        if (SaveChunk(channelBuffer_[fileName].data(), fileChunk) != PROFILING_SUCCESS) {
            return PROFILING_FAILED;
        }
        channelBuffer_[fileName].clear();
        data += leftDataSize;
        fileChunkReq->chunkSize -= leftDataSize;
    }

    // Store complete struct
    uint32_t structNum = fileChunkReq->chunkSize / structSize;
    for (uint32_t i = 0; i < structNum; ++i) {
        if (SaveChunk(data, fileChunk) != PROFILING_SUCCESS) {
            return PROFILING_FAILED;
        }
        data += structSize;
    }

    // Cache last truncated struct data
    const uint32_t dataLeft = fileChunkReq->chunkSize - structNum * structSize;
    if (dataLeft != 0) {
        MSPROF_LOGI("Cache truncated data, fileName: %s, cache size: %u", fileName.c_str(), dataLeft);
        channelBuffer_[fileName].reserve(structSize + 1);
        channelBuffer_[fileName].append(data, dataLeft);
    }

    return PROFILING_SUCCESS;
}

/**
 * @brief save ProfileFileChunk to local file
 * @param [in] data: struct data pointer
 * @param [in] fileChunk: ProfileFileChunk type shared_ptr
 * @return 0:SUCCESS, !0:FAILED
 */
int32_t FILETransport::SaveChunk(const char *data, SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunk) const
{
    if (data == nullptr) {
        MSPROF_LOGW("Unable to parse struct data, pointer is null");
        return PROFILING_SUCCESS;
    }

    const ProfTlv *packet = reinterpret_cast<const ProfTlv *>(data);
    if (packet->head != TLV_HEAD) {
        MSPROF_LOGE("Check tlv head failed");
        return PROFILING_FAILED;
    }
    const ProfTlvValue *tlvValue = reinterpret_cast<const ProfTlvValue *>(packet->value);
    fileChunk->isLastChunk = tlvValue->isLastChunk;
    fileChunk->chunkModule = tlvValue->chunkModule;
    fileChunk->chunkSize = tlvValue->chunkSize;
    fileChunk->offset = tlvValue->offset;
    fileChunk->chunk = std::string(tlvValue->chunk, tlvValue->chunkSize);
    fileChunk->fileName = tlvValue->fileName;

    std::string jobId = Utils::GetInfoPrefix(fileChunk->extraInfo);
    std::string devId = Utils::GetInfoSuffix(fileChunk->extraInfo);
    if (UpdateFileName(fileChunk, devId) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to update file name");
        return PROFILING_FAILED;
    }

    if (fileSlice_->SaveDataToLocalFiles(fileChunk) != PROFILING_SUCCESS) {
        MSPROF_LOGE("write data to local files failed, fileName: %s", fileChunk->fileName.c_str());
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

SHARED_PTR_ALIA<ITransport> FileTransportFactory::CreateFileTransport(
    const std::string &storageDir, const std::string &storageLimit, bool needSlice)
{
    SHARED_PTR_ALIA<FILETransport> fileTransport;
    MSVP_MAKE_SHARED2_RET(fileTransport, FILETransport, storageDir, storageLimit, fileTransport);
    MSVP_MAKE_SHARED2_RET(fileTransport->perfCount_, PerfCount, FILE_PERFCOUNT_MODULE_NAME,
        TRANSPORT_PRI_FREQ, nullptr);
    fileTransport->SetAbility(needSlice);
    int ret = fileTransport->Init();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("fileTransport init failed");
        MSPROF_INNER_ERROR("EK9999", "fileTransport init failed");
        return nullptr;
    }
    return fileTransport;
}
}  // namespace transport
}  // namespace dvvp
}  // namespace analysis
