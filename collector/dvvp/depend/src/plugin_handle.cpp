/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: Dynamic library handle
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-04-15
 */
#include "plugin_handle.h"

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

std::string PluginHandle::GetAscendHalPath() const
{
    std::string ascendInstallInfoPath = "/etc/ascend_install.info";
    // max file size:1024 Byte
    if ((Utils::GetFileSize(ascendInstallInfoPath) > MAX_ASCEND_INSTALL_INFO_FILE_SIZE) ||
        (Utils::IsSoftLink(ascendInstallInfoPath))) {
        return "";
    }
    std::ifstream infoFile(ascendInstallInfoPath);
    if (!infoFile) {
        return "";
    }
    std::string line;
    std::string installPath;
    const int numPerInfo = 2; // Driver_Install_Path_Param=/usr/local/Ascend
    while (getline(infoFile, line)) {
        std::vector<std::string> installInfo = Utils::Split(line, false, "", "=");
        if (installInfo.size() == numPerInfo && installInfo[0].compare("Driver_Install_Path_Param") == 0) {
            installPath = installInfo[1];
            break;
        }
    }
    if (installPath.empty()) {
        return "";
    }
    std::string driverPath = installPath + MSVP_SLASH + "driver" + MSVP_SLASH;
    std::string driverInfo = driverPath + "version.info";
    int ret = MmAccess2(driverInfo.c_str(), M_R_OK);
    if (ret != PROFILING_SUCCESS) {
        return "";
    }
    std::string soName = "libascend_hal.so";
    std::string libPath = driverPath + "lib64" + MSVP_SLASH;
    if (MmAccess2((libPath + soName).c_str(), M_R_OK) == PROFILING_SUCCESS) {
        return libPath + soName;
    } else if (MmAccess2((libPath + "common" + MSVP_SLASH + soName).c_str(), M_R_OK) == PROFILING_SUCCESS) {
        return libPath + "common" + MSVP_SLASH + soName;
    } else if (MmAccess2((libPath + "driver" + MSVP_SLASH + soName).c_str(), M_R_OK) == PROFILING_SUCCESS) {
        return libPath + "driver" + MSVP_SLASH + soName;
    }
    return "";
}

std::string PluginHandle::GetSoPath(const std::string &envValue) const
{
    if (soName_.compare("libascend_hal.so") == 0) {
        std::string ascendHalPath = GetAscendHalPath();
        if (!ascendHalPath.empty()) {
            if (MmAccess2(ascendHalPath.c_str(), M_R_OK) == PROFILING_SUCCESS) {
                return ascendHalPath;
            }
        }
    }
    const char *env = std::getenv(envValue.c_str());
    if (env == nullptr) {
        return "";
    }
    std::string pathEnv = env;
    std::vector<std::string> pathVec = Utils::Split(pathEnv, false, "", ":");
    for (auto path : pathVec) {
        std::string ret = path + MSVP_SLASH + soName_;
        if (MmAccess2(ret, M_R_OK) == PROFILING_SUCCESS) {
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
