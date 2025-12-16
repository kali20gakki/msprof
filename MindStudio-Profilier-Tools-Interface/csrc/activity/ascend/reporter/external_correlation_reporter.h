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
 * -------------------------------------------------------------------------
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