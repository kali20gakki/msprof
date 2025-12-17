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

#ifndef PROFILER_MSPROFTXMANAGER_H
#define PROFILER_MSPROFTXMANAGER_H

#include <mutex>
#include <map>
#include <atomic>
#include "utils.h"
#include "common/singleton/singleton.h"
#include "msprof_tx_reporter.h"
#include "msprof_stamp_pool.h"
#include "runtime_plugin.h"

namespace Msprof {
namespace MsprofTx {
using namespace analysis::dvvp::common::utils;
using namespace Collector::Dvvp::Plugin;
using ACL_PROF_STAMP_PTR = MsprofStampInstance *;
using CONST_CHAR_PTR = const char *;

constexpr uint64_t MARKEX_MODEL_ID = 0xFFFFFFFFU;
constexpr uint16_t MARKEX_TAG_ID = 11;

enum class EventType {
    MARK = 0,
    PUSH_OR_POP,
    START_OR_STOP,
    MARK_EX
};

class MsprofTxManager : public analysis::dvvp::common::singleton::Singleton<MsprofTxManager> {
public:

    MsprofTxManager();
    virtual ~MsprofTxManager();

    // create stamp memory pool, init plugin and push stack
    int Init();
    // destroy resource
    void UnInit();

    // get stamp from memory pool
    ACL_PROF_STAMP_PTR CreateStamp() const;
    // destroy stamp
    void DestroyStamp(ACL_PROF_STAMP_PTR stamp) const;

    int SetStampTagName(ACL_PROF_STAMP_PTR stamp, const char *tagName, uint16_t len) const;

    //  save category and name relation
    int SetCategoryName(uint32_t category, std::string categoryName) const;

    // stamp message manage
    int SetStampCategory(ACL_PROF_STAMP_PTR stamp, uint32_t category) const;
    int SetStampPayload(ACL_PROF_STAMP_PTR stamp, const int32_t type, void *value) const;
    int SetStampTraceMessage(ACL_PROF_STAMP_PTR stamp, CONST_CHAR_PTR msg, uint32_t msgLen) const;

    // mark stamp
    int Mark(ACL_PROF_STAMP_PTR stamp) const;

    // markex
    int MarkEx(const char *msg, size_t msgLen, aclrtStream stream);

    // stamp stack manage
    int Push(ACL_PROF_STAMP_PTR stamp) const;
    int Pop() const;

    // stamp map manage
    int RangeStart(ACL_PROF_STAMP_PTR stamp, uint32_t *rangeId) const;
    int RangeStop(uint32_t rangeId) const;

    void SetReporterCallback(const MsprofAddiInfoReporterCallback func);

    void ReportData(MsprofTxInfo &data);

    uint64_t GetEventId();

private:
    int ReportStampData(MsprofStampInstance *stamp) const;

private:
    bool isInit_;
    std::mutex mtx_;
    std::shared_ptr<MsprofTxReporter> reporter_;
    std::shared_ptr<MsprofStampPool> stampPool_;
    std::map<uint32_t, std::string> categoryNameMap_;
    std::atomic<uint64_t> eventId_;
};
}
}
#endif // PROFILER_MSPROFTXMANAGER_H
