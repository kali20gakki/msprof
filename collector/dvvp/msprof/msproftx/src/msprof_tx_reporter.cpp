/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: creat msprof reporter
 * Author:
 * Create: 2021-11-22
 */

#include "msprof_tx_reporter.h"

#include "profapi_plugin.h"

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
    isInit_ = false;
    MSPROF_LOGI("[UnInit]ReportCallback UnInit success.");
    return PROFILING_SUCCESS;
}

void MsprofTxReporter::SetReporterCallback(const MsprofAddiInfoReporterCallback func)
{
    reporterCallback_ = func;
}

int MsprofTxReporter::Report(MsprofTxInfo &data) const
{
    if (reporterCallback_ == nullptr) {
        MSPROF_LOGE("[TxReport]ReporterCallback_ is nullptr!");
        return PROFILING_FAILED;
    }

    if (!isInit_) {
        MSPROF_LOGE("[Report]ProfReporter is not inited!");
        return PROFILING_FAILED;
    }

    MsprofAdditionalInfo info;
    info.level = MSPROF_REPORT_TX_LEVEL;
    info.type = MSPROF_REPORT_TX_BASE_TYPE;
    info.dataLen = sizeof(data);
    auto ret = memcpy_s(info.data, MSPROF_ADDITIONAL_INFO_DATA_LENGTH, &data, sizeof(MsprofTxInfo));
    if (ret != EOK) {
        MSPROF_LOGE("[TxReport]Failed to memcpy for tx info data.");
        return PROFILING_FAILED;
    }
    return reporterCallback_(1, &info, sizeof(info));
}
} // namespace MsprofTx
} // namespace Msprof
