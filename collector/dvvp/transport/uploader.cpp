/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: lixubo
 * Create: 2018-06-13
 */
#include "uploader.h"
#include "config/config.h"
#include "errno/error_code.h"
#include "msprof_dlog.h"
#include "msprof_error_manager.h"
#include "utils/utils.h"

namespace analysis {
namespace dvvp {
namespace transport {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
using namespace Analysis::Dvvp::MsprofErrMgr;

Uploader::Uploader(SHARED_PTR_ALIA<analysis::dvvp::transport::ITransport> transport)
    : transport_(transport), queue_(nullptr), isInited_(false), forceQuit_(false), isStopped_(false)
{
}

Uploader::~Uploader()
{
    Uinit();
}

int Uploader::Init(size_t size)
{
    int ret = PROFILING_FAILED;
    do {
        MSVP_MAKE_SHARED1_BREAK(queue_, UploaderQueue, size);
        queue_->SetQueueName(UPLOADER_QUEUE_NAME);
        isInited_ = true;

        ret = PROFILING_SUCCESS;
    } while (0);
    return ret;
}

int Uploader::Uinit()
{
    if (isInited_) {
        (void)Stop(true);
        transport_.reset();
        isInited_ = false;
    }

    return PROFILING_SUCCESS;
}

int Uploader::UploadData(CONST_VOID_PTR data, int len)
{
    if (!isInited_) {
        MSPROF_LOGE("Uploader was not inited.");
        MSPROF_INNER_ERROR("EK9999", "Uploader was not inited.");
        return PROFILING_FAILED;
    }

    if (data == nullptr) {
        MSPROF_LOGE("[Uploader::UploadData]data is nullptr.");
        MSPROF_INNER_ERROR("EK9999", "data is nullptr.");
        return PROFILING_FAILED;
    }

    void *charPtr = const_cast<void *>(data);
    SHARED_PTR_ALIA<std::string> buffer;
    MSVP_MAKE_SHARED2_RET(buffer, std::string, reinterpret_cast<CONST_CHAR_PTR>(charPtr), len, PROFILING_FAILED);
    bool ret = queue_->Push(buffer);
    if (!ret) {
        MSPROF_LOGE("[Uploader::UploadData]Push data failed.");
        MSPROF_INNER_ERROR("EK9999", "Push data failed.");
        return PROFILING_FAILED;
    }

    return PROFILING_SUCCESS;
}

void Uploader::Run(const struct error_message::Context &errorContext)
{
    MsprofErrorManager::instance()->SetErrorContext(errorContext);
    if (!isInited_) {
        MSPROF_LOGE("Uploader was not inited.");
        MSPROF_INNER_ERROR("EK9999", "Uploader was not inited.");
        return;
    }

    do {
        SHARED_PTR_ALIA<std::string> data = nullptr;
        if (!queue_->TryPop(data) &&
            Thread::IsQuit()) {
            break;
        }

        if (data == nullptr) {
            (void)queue_->Pop(data);
        }

        // send the data
        if (data != nullptr) {
            int sentLen = transport_->SendBuffer(reinterpret_cast<const void *>(data->c_str()),
                                                 static_cast<int>(data->size()));
            if (sentLen != static_cast<int>(data->size())) {
                MSPROF_LOGE("Failed to upload data, data_len=%d, sent len=%d",
                    static_cast<int>(data->size()), sentLen);
                MSPROF_INNER_ERROR("EK9999", "Failed to upload data, data_len=%d, sent len=%d",
                    static_cast<int>(data->size()), sentLen);
            }
        }
    } while (!forceQuit_);

    MSPROF_LOGI("queue size remaining: %d, force_quit:%d",
        static_cast<int>(queue_->size()), (forceQuit_ ? 1 : 0));
}

// Before you invoke stop, all data should already been enqueued
int Uploader::Stop(bool force)
{
    if (!isStopped_ && isInited_) {
        isStopped_ = true;
        forceQuit_ = force;

        MSPROF_LOGI("Stopping uploader, force_quit:%d.", (forceQuit_ ? 1 : 0));

        queue_->Quit();
        int ret = Thread::Stop();
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to stop uploader");
            MSPROF_INNER_ERROR("EK9999", "Failed to stop uploader");
        }
    }

    return PROFILING_SUCCESS;
}
// wait all data to send
void Uploader::Flush() const
{
    while (queue_->size() != 0) {
        const unsigned long UPLOADER_SLEEP_TIME_IN_US = 100000;
        analysis::dvvp::common::utils::Utils::UsleepInterupt(UPLOADER_SLEEP_TIME_IN_US);
    }
}

SHARED_PTR_ALIA<analysis::dvvp::transport::ITransport> Uploader::GetTransport()
{
    return transport_;
}

int Uploader::GetQueueSize() const
{
    return queue_->size();
}
}  // namespace transport
}  // namespace dvvp
}  // namespace analysis
