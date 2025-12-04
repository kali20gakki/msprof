/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : external_correlation_reporter.h
 * Description        : 上报记录External Correlation数据
 * Author             : msprof team
 * Creation Date      : 2024/12/16
 * *****************************************************************************
*/

#ifndef EXTERNAL_CORRELATION_REPORTER_H
#define EXTERNAL_CORRELATION_REPORTER_H

#include <mutex>
#include <stack>
#include <map>

#include "csrc/include/mspti_activity.h"

namespace Mspti {
namespace Reporter {
class ExternalCorrelationReporter {
public:
    static ExternalCorrelationReporter* GetInstance();

    msptiResult ReportExternalCorrelationId(uint64_t correlationId);
    msptiResult PushExternalCorrelationId(msptiExternalCorrelationKind kind, uint64_t id);
    msptiResult PopExternalCorrelationId(msptiExternalCorrelationKind kind, uint64_t *lastId);

private:
    std::mutex mapMtx_;
    std::map<msptiExternalCorrelationKind, std::stack<uint64_t>> externalCorrelationMap;
};
}
}

#endif // EXTERNAL_CORRELATION_REPORTER_H