/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle perf data
 * Author: dml
 * Create: 2018-06-13
 */
#include "perf_count.h"
#include "msprof_dlog.h"

namespace Analysis {
namespace Dvvp {
namespace Common {
namespace Statistics {
PerfCount::PerfCount(const std::string &moduleName)
    : overHeadMin_(UINT64_MAX), overHeadMax_(0), overHeadSum_(0),
      packetNums_(0), firstReportClock_(0), lastReportClock_(0),
      throughPut_(0), moduleName_(moduleName), printFrequency_(0),
      overHeadWaiterLine_(0), exceedWaterLineCounter_(0)
{
}

PerfCount::PerfCount(const std::string &moduleName, const uint64_t printFrequency)
    : overHeadMin_(UINT64_MAX),
      overHeadMax_(0),
      overHeadSum_(0),
      packetNums_(0),
      firstReportClock_(0),
      lastReportClock_(0),
      throughPut_(0),
      moduleName_(moduleName),
      printFrequency_(printFrequency),
      overHeadWaiterLine_(0),
      exceedWaterLineCounter_(0)
{
}

PerfCount::~PerfCount()
{
}

/**
* @brief UpdatePerfInfo: update the perf data according the received data info
* @param [in] startTime: data received time(ns)
* @param [in] endTime: the time of data has been dealed
* @param [in] dataLen: the length of the received data
*/
void PerfCount::UpdatePerfInfo(uint64_t startTime, uint64_t endTime, size_t dataLen)
{
    if (startTime > endTime) {
        MSPROF_LOGE("[UpdatePerfInfo] startTime:%llu is larger than endTime:%llu", startTime, endTime);
        return;
    }
    std::lock_guard<std::mutex> lk(mtx_);
    lastReportClock_ = endTime;
    if (firstReportClock_ == 0) {
        firstReportClock_ = startTime;
    }

    uint64_t timeInterval = endTime - startTime;
    if (timeInterval > overHeadWaiterLine_) {
        exceedWaterLineCounter_++;
    }

    overHeadMin_ = timeInterval  < overHeadMin_ ? timeInterval : overHeadMin_;
    overHeadMax_ = timeInterval > overHeadMax_ ? timeInterval : overHeadMax_;
    overHeadSum_ += timeInterval;
    packetNums_ += 1;
    throughPut_ += dataLen;
    if ((printFrequency_ != 0) && ((packetNums_ % printFrequency_) == 0)) {
        PrintPerfInfo(moduleName_);
        ResetPerfInfo();
    }
}

/*
 * @brief PrintPerfInfo: print the perf info with module name
 * @param [in] moduleName: the module name
 */
void PerfCount::PrintPerfInfo(const std::string &moduleName) const
{
    const uint64_t perfMsec = 1000000;
    uint64_t nsToMSec = overHeadSum_ / perfMsec;

    if ((packetNums_ > 0) && (nsToMSec > 0)) {
        MSPROF_LOGI("moduleName: %s, overhead Min: %llu ns, overhead Max: %llu ns, overhead Avg: %llu ns,"
            "overhead Sum_: %llu ns, package nums: %llu, package size: %llu, throughput: %llu.%llu B/ms",
            moduleName.c_str(), overHeadMin_, overHeadMax_, overHeadSum_ / packetNums_, overHeadSum_,
            packetNums_, throughPut_, throughPut_ / nsToMSec, throughPut_ % nsToMSec);
        MSPROF_LOGI("moduleName: %s, overHeadWaiterLine: %llu, exceedWaterLineCounter: %llu",
            moduleName.c_str(), overHeadWaiterLine_, exceedWaterLineCounter_);
    }
}

/*
 * @brief OutPerfInfo: output the perf info with module name and device id
 * @param [in] tag: the module tag
 */
void PerfCount::OutPerfInfo(const std::string &tag)
{
    std ::string moduleName = tag.empty() ? moduleName_ : tag;
    std::lock_guard<std::mutex> lk(mtx_);
    PrintPerfInfo(moduleName);
}

/**
* @brief ResetPerfInfo: reset perf info
*/
void PerfCount::ResetPerfInfo()
{
    overHeadMin_ = UINT64_MAX;
    overHeadMax_ = 0;
    overHeadSum_ = 0;
    packetNums_ = 0;
    firstReportClock_ = 0;
    lastReportClock_ = 0;
    throughPut_ = 0;
    exceedWaterLineCounter_ = 0;
}
}  // namespace Statistics
}  // namespace Common
}  // namespace Dvvp
}  // namespace Analysis