/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: lixubo
 * Create: 2018-06-13
 */
#include "prof_device_core.h"
#include <memory>
#include "collection_entry.h"
#include "config/config.h"
#include "errno/error_code.h"
#include "msprof_dlog.h"
#include "param_validation.h"
#include "task_manager.h"
#include "utils/utils.h"
#include "adx_prof_api.h"

using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::common::validation;

int IdeDeviceProfileInit()
{
    MSPROF_EVENT("Begin to init profiling");
    int ret = PROFILING_FAILED;

    do {
        // try the block to catch the exceptions
        MSVP_TRY_BLOCK_BREAK({
            ret = analysis::dvvp::device::CollectionEntry::instance()->Init();
        });
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("CollectionEntry instance init failed");
            break;
        }
        // try the block to catch the exceptions
        MSVP_TRY_BLOCK_BREAK({
            ret = analysis::dvvp::device::TaskManager::instance()->Init();
        });
    } while (0);
    MSPROF_EVENT("End to init profiling, ret:%d", ret);
    return ret;
}

int IdeDeviceProfileProcess(HDC_SESSION session, CONST_TLV_REQ_PTR req)
{
    MSPROF_EVENT("Begin to process profiling");
    int ret = PROFILING_FAILED;
    do {
        if (session == nullptr) {
            MSPROF_LOGE("HDC session is invalid");
            break;
        }

        if ((req == nullptr) || (req->len <= 0) || (req->len > PROFILING_PACKET_MAX_LEN)) {
            MSPROF_LOGE("HDC request is invalid");
            Analysis::Dvvp::Adx::AdxHdcSessionClose(session);
            session = nullptr;
            break;
        }

        int devIndexId = 0;
        int err = Analysis::Dvvp::Adx::AdxIdeGetDevIdBySession(session, &devIndexId);
        if (err != IDE_DAEMON_OK) {
            MSPROF_LOGE("IdeGetDevIdBySession failed, err: %d", err);
            Analysis::Dvvp::Adx::AdxHdcSessionClose(session);
            session = nullptr;
            break;
        }
        if (!ParamValidation::instance()->CheckDeviceIdIsValid(std::to_string(devIndexId))) {
            MSPROF_LOGE("[IdeDeviceProfileProcess]devIndexId is not valid! devIndexId: %d", devIndexId);
            Analysis::Dvvp::Adx::AdxHdcSessionClose(session);
            session = nullptr;
            break;
        }

        auto transport = analysis::dvvp::transport::HDCTransportFactory().CreateHdcTransport(session);
        if (transport == nullptr) {
            MSPROF_LOGE("create HDC transport failed");
            Analysis::Dvvp::Adx::AdxHdcSessionClose(session);
            session = nullptr;
            break;
        }

        MSPROF_LOGI("device %d step handle function", devIndexId);

        // try the block to catch the exceptions
        MSVP_TRY_BLOCK_BREAK({
            ret = analysis::dvvp::device::CollectionEntry::instance()->Handle(
                transport, std::string(req->value, req->len), devIndexId);
        });
    } while (0);
    MSPROF_EVENT("End to process profiling, ret:%d", ret);
    return ret;
}

int IdeDeviceProfileCleanup()
{
    MSPROF_EVENT("Begin to cleanup profiling");
    int ret = PROFILING_FAILED;

    do {
        // try the block to catch the exceptions
        MSVP_TRY_BLOCK_BREAK({
            ret = analysis::dvvp::device::CollectionEntry::instance()->Uinit();
        });
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("CollectionEntry instance unInit failed");
        }
        // try the block to catch the exceptions
        MSVP_TRY_BLOCK_BREAK({
            ret = analysis::dvvp::device::TaskManager::instance()->Uninit();
        });
    } while (0);
    MSPROF_EVENT("End to cleanup profiling, ret:%d", ret);
    return ret;
}
