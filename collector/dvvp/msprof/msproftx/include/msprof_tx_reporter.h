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

#ifndef PROFILER_MSPROFTXREPORTER_H
#define PROFILER_MSPROFTXREPORTER_H

#include "prof_callback.h"
#include "prof_common.h"

namespace Msprof {
namespace MsprofTx {
class MsprofTxReporter {
public:
    MsprofTxReporter();
    virtual ~MsprofTxReporter();
    int Init();
    int UnInit();
    void SetReporterCallback(MsprofAddiInfoReporterCallback func);
    int Report(MsprofTxInfo &data) const;

private:
    bool isInit_;
    MsprofAddiInfoReporterCallback reporterCallback_;
};
}
}
#endif // PROFILER_MSPROFTXREPORTER_H
