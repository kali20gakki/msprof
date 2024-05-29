/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : device.h
 * Description        : Ascend Device.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#ifndef MSPTI_ACTIVITYT_ASCEND_DEVICE_H
#define MSPTI_ACTIVITYT_ASCEND_DEVICE_H

#include <condition_variable>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include "activity/ascend/dev_prof_job.h"
#include "common/inject/inject_base.h"
#include "external/mspti_activity.h"


namespace Mspti {
namespace Ascend {

const int32_t MAX_DEVICE_NUM = 64;

class Device {
public:
    explicit Device(uint32_t deviceId) : deviceId_(deviceId) {}
    ~Device() = default;
    msptiResult Start(const std::unordered_set<msptiActivityKind>& kinds);
    void Stop();

private:
    void Run();
    bool OnLine();
    static void GetDeviceList(uint32_t* deviceNum, uint32_t *deviceIdList);

private:
    static uint32_t deviceIdlist_[MAX_DEVICE_NUM];
    static uint32_t cur_device_num_;
    std::once_flag get_device_flag_;
    uint32_t deviceId_;
    std::thread t_;
    std::condition_variable cv_;
    std::mutex cv_mtx_;
    bool dev_stop_{false};
    std::unordered_map<msptiActivityKind, std::shared_ptr<DevProfJob>> job_map_;
    std::unordered_set<msptiActivityKind> kinds_;
}; // Device
}  // Ascend
}  // Mspti

#endif
