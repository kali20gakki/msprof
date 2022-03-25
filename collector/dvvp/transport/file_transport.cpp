/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 * Description: file transport
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2018-06-13
 */
#include "file_transport.h"
#include "config/config.h"
#include "errno/error_code.h"
#include "message/codec.h"
#include "message/prof_params.h"
#include "mmpa_api.h"
#include "msprof_dlog.h"
#include "proto/profiler.pb.h"
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

int FILETransport::SendBuffer(CONST_VOID_PTR buffer, int length)
{
    int retLengthError = 0;
    if (buffer == nullptr || length <= 0) {
        MSPROF_LOGE("buffer to be sent is nullptr, or data len:%d is invalid!", length);
        return retLengthError;
    }

    auto message = analysis::dvvp::message::DecodeMessage(std::string((CONST_CHAR_PTR)buffer, length));
    auto fileChunkReq = std::dynamic_pointer_cast<analysis::dvvp::proto::FileChunkReq>(message);
    if (fileChunkReq == nullptr) {
        MSPROF_LOGW("Failed to parse fileChunkReq");
        return retLengthError;
    }

    if (fileChunkReq->datamodule() == FileChunkDataModule::PROFILING_IS_FROM_DEVICE ||
        fileChunkReq->datamodule() == FileChunkDataModule::PROFILING_IS_FROM_MSPROF ||
        fileChunkReq->datamodule() == FileChunkDataModule::PROFILING_IS_FROM_MSPROF_DEVICE ||
        fileChunkReq->datamodule() == FileChunkDataModule::PROFILING_IS_FROM_MSPROF_HOST) {
        if (UpdateFileName(fileChunkReq) != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to update file name");
            return retLengthError;
        }
    }
    if (fileSlice_->SaveDataToLocalFiles(message) != PROFILING_SUCCESS) {
        MSPROF_LOGE("write data to local files failed, fileName: %s", fileChunkReq->filename().c_str());
        return retLengthError;
    }

    return length;
}

int FILETransport::CloseSession()
{
    return PROFILING_SUCCESS;
}

int FILETransport::UpdateFileName(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> fileChunkReq)
{
    std::string fileName = fileChunkReq->filename();
    size_t pos = fileName.find_last_of("/\\");
    if (pos != std::string::npos && pos + 1 < fileName.length()) {
        fileName = fileName.substr(pos + 1, fileName.length());
    }
    if (fileChunkReq->datamodule() != FileChunkDataModule::PROFILING_IS_FROM_MSPROF_HOST) {
        analysis::dvvp::message::JobContext jobCtx;
        if (!jobCtx.FromString(fileChunkReq->hdr().job_ctx())) {
            MSPROF_LOGE("Failed to parse jobCtx json %s. fileName:%s",
                fileChunkReq->hdr().job_ctx().c_str(), fileName.c_str());
            return PROFILING_FAILED;
        }
        if (!ParamValidation::instance()->CheckDeviceIdIsValid(jobCtx.dev_id)) {
            MSPROF_LOGE("jobCtx.dev_id is not valid!");
            return PROFILING_FAILED;
        }
        if (fileChunkReq->tag().length() == 0) {
            fileName.append(".").append(jobCtx.dev_id);
        } else {
            fileName.append(".").append(fileChunkReq->tag()).append(".").append(jobCtx.dev_id);
        }
    } else {
        if (fileChunkReq->tag().length() != 0) {
            fileName.append(".").append(fileChunkReq->tag());
        }
    }
    fileName = "data" + std::string(MSVP_SLASH) + fileName;
    fileChunkReq->set_filename(fileName);
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
