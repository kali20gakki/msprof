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
#include "profcommon_plugin.h"
#include "msprof_dlog.h"

using namespace analysis::dvvp::common::error;

namespace Collector {
namespace Dvvp {
namespace Plugin {
SHARED_PTR_ALIA<PluginHandle> ProfCommonPlugin::pluginHandle_ = nullptr;

void ProfCommonPlugin::GetAllFunction()
{
    pluginHandle_->GetFunction<void, MstxInitInjectionFunc, ProfModule>("ProfRegisterMstxFunc", profRegisterMstxFunc_);
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

void ProfCommonPlugin::MsprofProfRegisterMstxFunc(MstxInitInjectionFunc injectFunc, ProfModule module)
{
    PthreadOnce(&loadFlag_, []()->void {ProfCommonPlugin::instance()->LoadProfCommonSo();});
    if (profRegisterMstxFunc_ == nullptr) {
        MSPROF_LOGW("ProfCommonPlugin ProfRegisterMstxFunc function is null.");
        return;
    }
    profRegisterMstxFunc_(injectFunc, module);
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
