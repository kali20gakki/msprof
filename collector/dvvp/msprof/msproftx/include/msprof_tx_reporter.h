/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: creat msprof reporter
 * Author:
 * Create: 2021-11-22
 */

#ifndef PROFILER_MSPROFTXREPORTER_H
#define PROFILER_MSPROFTXREPORTER_H

#include "prof_callback.h"

namespace Msprof {
namespace MsprofTx {
class MsprofTxReporter {
public:
    MsprofTxReporter();

    virtual ~MsprofTxReporter();

    int Init();

    int UnInit();

    void SetReporterCallback(const MsprofReporterCallback func)
    {
        reporterCallback_ = func;
    }

    int Report(ReporterData &data) const;

private:
    bool isInit_;
    MsprofReporterCallback reporterCallback_;
};
}
}
#endif // PROFILER_MSPROFTXREPORTER_H
