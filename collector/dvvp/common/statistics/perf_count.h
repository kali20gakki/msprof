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
#ifndef ANALYSIS_DVVP_COMMON_PERF_STATISTICS_COUNT_H
#define ANALYSIS_DVVP_COMMON_PERF_STATISTICS_COUNT_H

#include <iostream>
#include <mutex>
#include "utils/utils.h"

namespace Analysis {
namespace Dvvp {
namespace Common {
namespace Statistics {
class PerfCount {
public:
    explicit PerfCount(const std::string &moduleName);
    PerfCount(const std::string &moduleName, const uint64_t printFrequency);
    ~PerfCount();

    /**
    * @brief UpdatePerfInfo: update the perf data according the received data info
    * @param [in] startTime: data received time(ns)
    * @param [in] endTime: The time at which the data has been dealt with
    * @param [in] dataLen: the length of the received data
    */
    void UpdatePerfInfo(uint64_t startTime, uint64_t endTime, size_t dataLen);

    /**
    * @brief OutPerfInfo: output the perf info with module name and device id
    * @param [in] tag: the module tag
    */
    void OutPerfInfo(const std::string &tag);

private:
    void PrintPerfInfo(const std::string &moduleName) const;
    void ResetPerfInfo();

private:
    uint64_t overHeadMin_; // the Report data min overhead time(ns)
    uint64_t overHeadMax_; // the Report data max overhead time(ns)

    // the sum time of Report overhead(ns), UINT64_T can record 18446744073 seconds, it's means 584 years
    uint64_t overHeadSum_;
    uint64_t packetNums_; // Report data numbers, used to calculate overhead average
    uint64_t firstReportClock_; // the raw clock(ns) of first report data
    uint64_t lastReportClock_; // the raw clock(ns) of the last report data
    uint64_t throughPut_; // UINT64_T can record 17179869184 GB data package
    std::string moduleName_;
    uint64_t printFrequency_;
    std::mutex mtx_;
    uint64_t overHeadWaiterLine_;
    uint64_t exceedWaterLineCounter_;
};
}  // namespace Statistics
}  // namespace Common
}  // namespace Dvvp
}  // namespace Analysis
#endif
