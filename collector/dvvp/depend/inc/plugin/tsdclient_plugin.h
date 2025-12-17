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