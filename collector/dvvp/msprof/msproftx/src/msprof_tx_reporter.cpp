/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

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
