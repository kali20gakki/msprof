/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: Dynamic library handle
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-04-15
 */
#include "plugin_handle.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "config.h"
#include "mmpa/mmpa_api.h"
#include "securec.h"
#include "utils/utils.h"

namespace Collector {
namespace Dvvp {
namespace Plugin {
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::common::utils;
using namespace Collector::Dvvp::Mmpa;
PluginHandle::~PluginHandle()
{
}

const std::string PluginHandle::GetSoName() const
{
    return soName_;
}

int32_t PluginHandle::OpenPlugin(const std::string envValue)
{
    if (envValue.empty() || envValue.size() >= MAX_PATH_LENGTH) {
        return PROFILING_FAILED;
    }
    std::string soPath = GetSoPath(envValue);
    if (soPath.empty()) {
        return PROFILING_FAILED;
    }
    char trustedPath[MMPA_MAX_PATH] = {'\0'};
    int32_t ret = MmRealPath(soPath.c_str(), trustedPath, sizeof(trustedPath));
    if (ret != PROFILING_SUCCESS || Utils::IsSoftLink(std::string(trustedPath))) {
        return PROFILING_FAILED;
    }
    handle_ = dlopen(trustedPath, RTLD_NOW | RTLD_GLOBAL);
    if (!handle_) {
        return PROFILING_FAILED;
    }
    load_ = true;
    return PROFILING_SUCCESS;
}

void PluginHandle::CloseHandle()
{
    if (!handle_ || !load_) { // nullptr
        return;
    }
    dlclose(handle_);
    handle_ = nullptr;
}

bool PluginHandle::HasLoad()
{
    return load_;
}

void PluginHandle::SplitPath(const std::string &mutilPath, std::vector<std::string> &pathVec) const
{
    const std::string tmpString = mutilPath + ":";
    std::string::size_type startPos = 0U;
    std::string::size_type curPos = tmpString.find(':', 0U);
    while (curPos != std::string::npos) {
        const std::string path = tmpString.substr(startPos, curPos - startPos);
        if (!path.empty()) {
        pathVec.push_back(path);
        }
        startPos = curPos + 1U;
        curPos = tmpString.find(':', startPos);
    }
}

std::string PluginHandle::GetSoPath(const std::string &envValue) const
{
    const char *env = std::getenv(envValue.c_str());
    if (env == nullptr) {
        return "";
    }
    std::string pathEnv = env;
    std::vector<std::string> pathVec;
    SplitPath(std::string(pathEnv), pathVec);
    for (auto path : pathVec) {
        std::string ret = path + "/" + soName_;
        if (access(ret.c_str(), F_OK) != -1) {
            return ret;
        }
    }
    return "";
}

bool PluginHandle::IsFuncExist(const std::string funcName) const
{
    if (funcName.empty()) {
        return false;
    }
    void *func = dlsym(handle_, funcName.c_str());
    if (!func) {
        return false;
    }
    return true;
}
} // Plugin
} // Dvvp
} // Collector
