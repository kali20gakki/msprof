/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: Profcommon interface
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2025-04-22
 */
#include "profcommon_plugin.h"
#include "msprof_dlog.h"

using namespace analysis::dvvp::common::error;

namespace Collector {
namespace Dvvp {
namespace Plugin {
SHARED_PTR_ALIA<PluginHandle> ProfCommonPlugin::pluginHandle_ = nullptr;

void ProfCommonPlugin::GetAllFunction()
{
    pluginHandle_->GetFunction<void, MstxInitInjectionFunc, ProfModule>("ProfRegisteMstxFunc", profRegisteMstxFunc_);
    pluginHandle_->GetFunction<void, ProfModule>("EnableMstxFunc", enableMstxFunc_);
}

void ProfCommonPlugin::LoadProfCommonSo()
{
    if (pluginHandle_ == nullptr) {
        MSVP_MAKE_SHARED1_VOID(pluginHandle_, PluginHandle, soName_);
    }
    int32_t ret = PROFILING_SUCCESS;
    if (!pluginHandle_->HasLoad()) {
        bool isSoValid = true;
        if (pluginHandle_->OpenPluginFromEnv("LD_LIBRARY_PATH", isSoValid) != PROFILING_SUCCESS &&
            pluginHandle_->OpenPluginFromLdcfg(isSoValid) != PROFILING_SUCCESS) {
            return;
        }

        if (!isSoValid) {
            MSPROF_LOGW("Profcommon so it may not be secure because there are incorrect permissions");
        }
    }
    GetAllFunction();
}

bool ProfCommonPlugin::IsFuncExist(const std::string &funcName) const
{
    return pluginHandle_->IsFuncExist(funcName);
}

void ProfCommonPlugin::MsprofProfRegisteMstxFunc(MstxInitInjectionFunc injectFunc, ProfModule module)
{
    PthreadOnce(&loadFlag_, []()->void {ProfCommonPlugin::instance()->LoadProfCommonSo();});
    if (profRegisteMstxFunc_ == nullptr) {
        MSPROF_LOGW("ProfCommonPlugin ProfRegisteMstxFunc function is null.");
        return;
    }
    profRegisteMstxFunc_(injectFunc, module);
}

void ProfCommonPlugin::MsprofEnableMstxFunc(ProfModule module)
{
    PthreadOnce(&loadFlag_, []()->void {ProfCommonPlugin::instance()->LoadProfCommonSo();});
    if (enableMstxFunc_ == nullptr) {
        MSPROF_LOGW("ProfCommonPlugin MsprofEnableMstxFunc function is null.");
        return;
    }
    enableMstxFunc_(module);
}
} // namespace Plugin
} // namespace Dvvp
} // namespace Collector
