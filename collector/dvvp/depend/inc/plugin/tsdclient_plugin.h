/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: tsd client plugin header
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2025-07-22
 */
#ifndef TSDCLIENT_PLUGIN_H
#define TSDCLIENT_PLUGIN_H

#include "tsdclient/tsdclient_api.h"
#include "singleton/singleton.h"
#include "plugin_handle.h"

namespace Collector {
namespace Dvvp {
namespace Plugin {
using TsdCapabilityGetFunc = std::function<uint32_t(uint32_t, int32_t, uint64_t)>;
using TsdProcessOpenFunc = std::function<uint32_t(uint32_t, ProcOpenArgs *)>;
using TsdGetProcListStatusFunc = std::function<uint32_t(uint32_t, ProcStatusParam *, uint32_t)>;
using ProcessCloseSubProcListFunc = std::function<uint32_t(uint32_t, ProcStatusParam *, uint32_t)>;

class TsdClientPlugin : public analysis::dvvp::common::singleton::Singleton<TsdClientPlugin> {
public:
    TsdClientPlugin() : soName_("libtsdclient.so"), loadFlag_(0) {}
    bool IsFuncExist(const std::string &funcName) const;
    void GetAllFunction();

    int MsprofTsdCapabilityGet(uint32_t logicDeviceId, int32_t type, uint64_t ptr);
    int MsprofTsdProcessOpen(uint32_t logicDeviceId, ProcOpenArgs *openArgs);
    int MsprofTsdGetProcListStatus(uint32_t logicDeviceId, ProcStatusParam *pidInfo, uint32_t arrayLen);
    int MsprofProcessCloseSubProcList(uint32_t logicDeviceId, ProcStatusParam *closeList, uint32_t listSize);
private:
    std::string soName_;
    static SHARED_PTR_ALIA<PluginHandle> pluginHandle_;
    PTHREAD_ONCE_T loadFlag_;
    TsdCapabilityGetFunc tsdCapabilityGetFunc_ = nullptr;
    TsdProcessOpenFunc tsdProcessOpenFunc_ = nullptr;
    TsdGetProcListStatusFunc tsdGetProcListStatusFunc_ = nullptr;
    ProcessCloseSubProcListFunc processCloseSubProcListFunc_ = nullptr;
private:
    void LoadTsdClientSo();
};
}
}
}

#endif