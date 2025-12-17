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
#include "plugin_handle.h"

#include <iostream>
#include <cstdio>
#include <sys/stat.h>
#include "config.h"
#include "mmpa/mmpa_api.h"
#include "securec.h"

namespace Collector {
namespace Dvvp {
namespace Plugin {
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::common::utils;
using namespace Collector::Dvvp::Mmpa;

const int MAX_BUF_SIZE = 1024;
const int READ_CMD_RESULT_WAIT_US = 500000;

PluginHandle::~PluginHandle()
{
}

const std::string PluginHandle::GetSoName() const
{
    return soName_;
}

int32_t PluginHandle::DlopenSo(const std::string& soPath)
{
    if (soPath.empty()) {
        return PROFILING_FAILED;
    }
    int flag = GetSoName() == "libtsdclient.so" ? (RTLD_LAZY | RTLD_NODELETE) : (RTLD_NOW | RTLD_GLOBAL);
    handle_ = dlopen(soPath.c_str(), flag);
    if (!handle_) {
        return PROFILING_FAILED;
    }
    load_ = true;
    return PROFILING_SUCCESS;
}

bool PluginHandle::CheckSoValid(const std::string& soPath) const
{
    if (soPath.empty()) {
        return false;
    }
    struct stat fileStat;
    if (stat(soPath.c_str(), &fileStat) == 0) {
        if (fileStat.st_mode & S_IWOTH) {
            return false;
        } else {
            if (fileStat.st_uid == 0 || fileStat.st_uid == static_cast<uint32_t>(MmGetUid())) {
                return true;
            }
            return false;
        }
    }
    return false;
}

int32_t PluginHandle::OpenPluginFromEnv(const std::string envValue, bool &isSoValid)
{
    if (envValue.empty() || envValue.size() >= MAX_PATH_LENGTH) {
        return PROFILING_FAILED;
    }
    std::string soPath = GetSoPath(envValue);
    if (soPath.empty()) {
        return PROFILING_FAILED;
    }
    char realPath[MMPA_MAX_PATH] = {'\0'};
    int32_t ret = MmRealPath(soPath.c_str(), realPath, sizeof(realPath));
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    std::string path = std::string(realPath, sizeof(realPath));
    if (path.empty() || Utils::IsSoftLink(path)) {
        return PROFILING_FAILED;
    }
    if (!CheckSoValid(realPath)) {
        isSoValid = false;
    }
    return DlopenSo(realPath);
}

void PluginHandle::GetSoPaths(std::vector<std::string> &soPaths)
{
    std::string cmd = "ldconfig -p | grep " + soName_;
    FILE* fp = popen(cmd.c_str(), "r");
    if (fp == nullptr) {
        return;
    }
    int32_t fd = fileno(fp);
    if (fd < 0) {
        pclose(fp);
        fp = nullptr;
        return;
    }
    char pathBuff[MAX_BUF_SIZE] = {0};
    struct timeval timeout = {0, READ_CMD_RESULT_WAIT_US};
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    if (select(fd + 1, &readfds, nullptr, nullptr, &timeout) <= 0) {
        pclose(fp);
        fp = nullptr;
        MmClose(fd);
        return;
    }
    size_t recvNum = fread(pathBuff, sizeof(char), MAX_BUF_SIZE, fp);
    if (ferror(fp) || (recvNum != MAX_BUF_SIZE && !feof(fp))) { // read err
        pclose(fp);
        fp = nullptr;
        MmClose(fd);
        return;
    }
    std::vector<std::string> paths = Utils::Split(std::string(pathBuff, recvNum), false, "", "\n");
    for (auto &path : paths) {
        size_t pos = path.find("=> ");
        if (pos == std::string::npos) {
            continue;
        }
        std::string soPath = path.substr(pos + 3); // 3 : start pos for so path
        if (MmAccess2(soPath, M_R_OK) != PROFILING_SUCCESS) {
            continue;
        }
        soPaths.push_back(soPath);
    }
    pclose(fp);
    fp = nullptr;
    MmClose(fd);
    return;
}

int32_t PluginHandle::OpenPluginFromLdcfg(bool &isSoValid)
{
    GetSoPaths(soPaths_);
    if (soPaths_.empty()) {
        return PROFILING_FAILED;
    }
    for (size_t i = 0; i < soPaths_.size(); i++) {
        char realPath[MMPA_MAX_PATH] = {'\0'};
        int32_t ret = MmRealPath(soPaths_[i].c_str(), realPath, sizeof(realPath));
        if (ret != PROFILING_SUCCESS) {
            return PROFILING_FAILED;
        }
        std::string path = std::string(realPath, sizeof(realPath));
        if (path.empty() || Utils::IsSoftLink(path)) {
            return PROFILING_FAILED;
        }
        if (!CheckSoValid(realPath)) {
            isSoValid = false;
        }
    }
    handle_ = dlopen(soName_.c_str(), RTLD_NOW | RTLD_GLOBAL);
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
    if (!infoFile.is_open()) {
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
    infoFile.close();
    if (installPath.empty()) {
        return "";
    }
    std::string driverPath = installPath + MSVP_SLASH + "driver" + MSVP_SLASH;
    std::string driverInfo = driverPath + "version.info";
    int ret = MmAccess2(driverInfo.c_str(), M_R_OK);
    if (ret != PROFILING_SUCCESS) {
        return "";
    }

    std::string libPath = driverPath + "lib64" + MSVP_SLASH;
    if (MmAccess2((libPath + soName_).c_str(), M_R_OK) == PROFILING_SUCCESS) {
        return libPath + soName_;
    } else if (MmAccess2((libPath + "common" + MSVP_SLASH + soName_).c_str(), M_R_OK) == PROFILING_SUCCESS) {
        return libPath + "common" + MSVP_SLASH + soName_;
    } else if (MmAccess2((libPath + "driver" + MSVP_SLASH + soName_).c_str(), M_R_OK) == PROFILING_SUCCESS) {
        return libPath + "driver" + MSVP_SLASH + soName_;
    }
    return "";
}

std::string PluginHandle::GetSoPath(const std::string &envValue) const
{
    // get so path from set_env.sh
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

    // get driver so path from install path when driver path is not set in set_env.sh
    if (soName_.compare("libascend_hal.so") == 0) {
        std::string ascendHalPath = GetAscendHalPath();
        if (!ascendHalPath.empty()) {
            if (MmAccess2(ascendHalPath.c_str(), M_R_OK) == PROFILING_SUCCESS) {
                return ascendHalPath;
            }
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
