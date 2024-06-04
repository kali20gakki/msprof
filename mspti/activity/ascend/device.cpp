/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : device.cpp
 * Description        : Ascend Device.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/
#include "activity/ascend/device.h"

#include "activity/ascend/channel/channel_pool_manager.h"
#include "common/context_manager.h"
#include "common/inject/driver_inject.h"
#include "common/plog_manager.h"
#include "external/mspti_activity.h"
#include "external/mspti_base.h"


namespace Mspti {
namespace Ascend {

uint32_t Device::cur_device_num_ = 0;
uint32_t Device::deviceIdlist_[MAX_DEVICE_NUM] = {1};

msptiResult Device::Start(const std::unordered_set<msptiActivityKind>& kinds)
{
    kinds_ = kinds;
    if (!OnLine()) {
        MSPTI_LOGE("The device: %u is offline.", deviceId_);
        return MSPTI_ERROR_DEVICE_OFFLINE;
    }

    if (!t_.joinable()) {
        t_ = std::thread(std::bind(&Device::Run, this));
    }
    return MSPTI_SUCCESS;
}

void Device::Stop()
{
    {
        std::unique_lock<std::mutex> lck(cv_mtx_);
        dev_stop_ = true;
        cv_.notify_one();
    }
    if (t_.joinable()) {
        t_.join();
    }
    kinds_.clear();
}

void Device::Run()
{
    Mspti::Common::ContextManager::GetInstance()->InitDevTimeInfo(deviceId_);
    Mspti::Ascend::Channel::ChannelPoolManager::GetInstance()->Init();
    // Create ProfJob And Start
    for (auto iter = kinds_.begin(); iter != kinds_.end(); iter++) {
        if (job_map_.find(*iter) != job_map_.end()) {
            MSPTI_LOGW("The prof task of kind: %d is already running.", *iter);
            continue;
        }
        auto job_ptr = CollectionJobFactory::CreateJob(deviceId_, *iter);
        if (!job_ptr) {
            MSPTI_LOGE("Create prof job failed.");
            return;
        }
        job_ptr->Start();
        job_map_.insert({*iter, job_ptr});
    }
    // Wait Device Stop
    {
        std::unique_lock<std::mutex> lk(cv_mtx_);
        cv_.wait(lk, [&] () {
            return dev_stop_;
        });
    }
    // Stop All DevProfJob
    for (auto iter = job_map_.begin(); iter != job_map_.end(); iter++) {
        iter->second->Stop();
    }
    job_map_.clear();
    Mspti::Ascend::Channel::ChannelPoolManager::GetInstance()->UnInit();
}

void Device::GetDeviceList(uint32_t* deviceNum, uint32_t *deviceIdList)
{
    auto ret = DrvGetDevNum(deviceNum);
    if (ret != DRV_ERROR_NONE) {
        MSPTI_LOGE("Get device num failed.");
        return;
    }
    ret = DrvGetDevIDs(deviceIdList, *deviceNum);
    if (ret != DRV_ERROR_NONE) {
        MSPTI_LOGE("Get device id list failed.");
        return;
    }
}

bool Device::OnLine()
{
    std::call_once(get_device_flag_, [this] () {
        this->GetDeviceList(&this->cur_device_num_, this->deviceIdlist_);
    });
    if (cur_device_num_ == 0) {
        return false;
    }
    for (uint32_t i = 0; i < cur_device_num_; ++i) {
        if (deviceIdlist_[i] == deviceId_) {
            return true;
        }
    }
    return false;
}

}  // Ascend
}  // Mspti
