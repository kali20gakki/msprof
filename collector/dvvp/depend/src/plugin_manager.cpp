#include "plugin_manager.h"
#include "config.h"
#include <cstdlib>
#include <fstream>

namespace Analysis {
namespace Dvvp {
namespace Plugin {

using namespace analysis::dvvp::common::config;

const std::string PluginManager::GetSoName() const
{
    return soName_;
}

std::string PluginManager::RealPath(const std::string &path) const
{
    char resoved_path[MAX_PATH_LENGTH] = {0x00};
    std::string res = "";
    if (realpath(path.c_str(), resoved_path)) {
        res = resoved_path;
    } else {
        MSPROF_LOGE("[RealPath]Get realpath failed.");
    }
    return res;
}

Status PluginManager::OpenPlugin(const std::string envValue)
{
    if (envValue.empty() || envValue.size() >= MAX_PATH_LENGTH) {
        MSPROF_LOGE("[OpenPlugin]envValue wrong");
        return PLUGIN_LOAD_FAILED;
    }
    std::string soPath = GetSoPath(envValue);
    if (soPath.empty()) {
        MSPROF_LOGE("[OpenPlugin]Get so path failed");
        return PLUGIN_LOAD_FAILED;
    }
    std::string absoluteDir = RealPath(soPath);
    if (absoluteDir.empty()) {
        MSPROF_LOGE("[OpenPlugin]Get realpath failed.");
        return PLUGIN_LOAD_FAILED;
    }

    handle_ = dlopen(absoluteDir.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (!handle_) {
        MSPROF_LOGE("[OpenPlugin]dlopen failed.");
        return PLUGIN_LOAD_FAILED;
    }
    load_ = true;
    return PLUGIN_LOAD_SUCCESS;
}

void PluginManager::CloseHandle()
{
    if (!handle_ || !load_) { // nullptr
        MSPROF_LOGI("[CloseHandle]Do not need to close so.");
        return;
    }
    if (dlclose(handle_) != 0) {
        MSPROF_LOGE("[CloseHandle]failed to close [%s]so.msg:[]", soName_.c_str(), dlerror());
    }
}

bool PluginManager::HasLoad()
{
    return load_;
}

void PluginManager::SplitPath(const std::string &mutilPath, std::vector<std::string> &pathVec) const
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

void PluginManager::ReadDriverConf(std::vector<std::string> &pathVec) const
{
    const char *drvConfPth = "/etc/ld.so.conf.d/ascend_driver_so.conf";
    if (access(drvConfPth, F_OK) == -1) {
        MSPROF_LOGI("[ReadDriverConf]ascend_driver_so.conf not exist.");
        return;
    }
    std::fstream drvConfReader(drvConfPth, std::fstream::in);
    if (!drvConfReader.is_open()) {
        MSPROF_LOGE("[ReadDriverConf]Fail to read drvConf file.");
        return;
    }
    char path[MAX_PATH_LENGTH] = {0};
    while (drvConfReader.getline(path, MAX_PATH_LENGTH)) {
        pathVec.push_back(path);
    }
    drvConfReader.close();
}

std::string PluginManager::GetSoPath(const std::string &envValue) const
{
    MSPROF_LOGI("[GetSoPath]envValue:%s", envValue.c_str());
    char pathEnv[MAX_PATH_LENGTH] = {0};
    const char *env = getenv(envValue.c_str());
    strncpy(pathEnv, env, MAX_PATH_LENGTH);
    MSPROF_LOGI("[GetSoPath]pathEnv:%s", pathEnv);
    std::vector<std::string> pathVec;
    SplitPath(std::string(pathEnv), pathVec);
    ReadDriverConf(pathVec);
    for (auto path : pathVec) {
        std::string ret = path + "/" + soName_;
        MSPROF_LOGI("[GetSoPath]so path:%s", ret.c_str());
        if (access(ret.c_str(), F_OK) != -1) {
            return ret;
        }
    }
    return "";
}

} // namespace Plugin
} // namespace Dvvp
} // namespace Analysis
