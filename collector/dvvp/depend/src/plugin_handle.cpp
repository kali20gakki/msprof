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
#include "securec.h"

namespace Analysis {
namespace Dvvp {
namespace Plugin {
using namespace analysis::dvvp::common::config;

const std::string PluginHandle::GetSoName() const
{
    return soName_;
}

bool PluginHandle::IsSoftLink(const std::string &path) const
{
    struct stat buf1;
    int ret = stat(path.c_str(), &buf1);
    if (ret != 0) {
        return true;
    }

    struct stat buf2;
    ret = lstat(path.c_str(), &buf2);
    if (ret != 0) {
        return true;
    }

    if (buf1.st_ino != buf2.st_ino) {
        return true;     // soft-link
    }
    return false;   // not soft-link
}

std::string PluginHandle::RealPath(const std::string &path) const
{
    char resoved_path[MAX_PATH_LENGTH] = {0x00};
    std::string res = "";
    if (realpath(path.c_str(), resoved_path)) {
        res = resoved_path;
    }
    return res;
}

PluginStatus PluginHandle::OpenPlugin(const std::string envValue)
{
    if (envValue.empty() || envValue.size() >= MAX_PATH_LENGTH) {
        return PLUGIN_LOAD_FAILED;
    }
    std::string soPath = GetSoPath(envValue);
    if (soPath.empty()) {
        return PLUGIN_LOAD_FAILED;
    }
    std::string absoluteDir = RealPath(soPath);
    if (absoluteDir.empty() || IsSoftLink(absoluteDir)) {
        return PLUGIN_LOAD_FAILED;
    }
    handle_ = dlopen(absoluteDir.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (!handle_) {
        return PLUGIN_LOAD_FAILED;
    }
    load_ = true;
    return PLUGIN_LOAD_SUCCESS;
}

void PluginHandle::CloseHandle()
{
    if (!handle_ || !load_) { // nullptr
        return;
    }
    dlclose(handle_);
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
} // namespace Plugin
} // namespace Dvvp
} // namespace Analysis
