/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: creat msprof reporter
 * Author:
 * Create: 2021-11-22
 */

#include "msprof_tx_reporter.h"

#include "prof_api.h"

#include "errno/error_code.h"
#include "msprof_dlog.h"
using namespace analysis::dvvp::common::error;
namespace Msprof {
namespace MsprofTx {
MsprofTxReporter::MsprofTxReporter() : isInit_(false), reporterCallback_(nullptr) {}

MsprofTxReporter::~MsprofTxReporter() {}

int MsprofTxReporter::Init()
{
    if (reporterCallback_ == nullptr) {
        MSPROF_LOGE("[Init]ReporterCallback_ is nullptr!");
        return PROFILING_FAILED;
    }

    auto ret = reporterCallback_(
        static_cast<uint32_t>(MsprofReporterModuleId::MSPROF_MODULE_MSPROF),
        static_cast<uint32_t>(MsprofReporterCallbackType::MSPROF_REPORTER_INIT),
        nullptr, 0);
    if (ret != MSPROF_ERROR_NONE) {
        MSPROF_LOGE("[Init]Init reporterCallback failed, ret is %d", ret);
        return PROFILING_FAILED;
    }

    isInit_ = true;
    MSPROF_LOGI("[Init] MsprofTxReporter init success.");
    return PROFILING_SUCCESS;
}

int MsprofTxReporter::UnInit()
{
    if (reporterCallback_ == nullptr) {
        MSPROF_LOGE("[UnInit]ReporterCallback_ is nullptr!");
        return PROFILING_FAILED;
    }

    auto ret = reporterCallback_(
        static_cast<uint32_t>(MsprofReporterModuleId::MSPROF_MODULE_MSPROF),
        static_cast<uint32_t>(MsprofReporterCallbackType::MSPROF_REPORTER_UNINIT),
        nullptr, 0);
    if (ret != MSPROF_ERROR_NONE) {
        MSPROF_LOGE("[UnInit]UnInit reporterCallback failed, ret is %d", ret);
        return ret;
    }

    isInit_ = false;
    MSPROF_LOGI("[UnInit] reporetCallback UnInit success.");
    return ret;
}

int MsprofTxReporter::Report(ReporterData &data) const
{
    if (reporterCallback_ == nullptr) {
        MSPROF_LOGE("[Report]ReporterCallback_ is nullptr!");
        return PROFILING_FAILED;
    }

    if (!isInit_) {
        MSPROF_LOGE("[Report]Reporter is not inited!");
        return PROFILING_FAILED;
    }

    return reporterCallback_(
        static_cast<uint32_t>(MsprofReporterModuleId::MSPROF_MODULE_MSPROF),
        static_cast<uint32_t>(MsprofReporterCallbackType::MSPROF_REPORTER_REPORT),
        static_cast<void *>(&data), sizeof(data));
}
} // namespace MsprofTx
} // namespace Msprof
