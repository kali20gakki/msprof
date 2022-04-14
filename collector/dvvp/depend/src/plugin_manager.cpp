#include "plugin_manager.h"
#include "config.h"
#include <cstdlib>

namespace Analysis {
namespace Dvvp {
namespace Plugin {

using namespace analysis::dvvp::common::config;

const std::string PluginManager::GetSoName() const
{
    return so_name_;
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
    if (!dlclose(handle_)) {
        const char *error = dlerror();
        if (!error) {
            MSPROF_LOGE("[CloseHandle]failed to get error msg.");
            return;
        }
        MSPROF_LOGE("[CloseHandle]failed to close [%s]so.msg:[]", so_name_.c_str(), error);
        return;
    }
}

bool PluginManager::HasLoad()
{
    return load_;
}

void PluginManager::SplitPath(const std::string &mutil_path, std::vector<std::string> &path_vec) const
{
    const std::string tmp_string = mutil_path + ":";
    std::string::size_type start_pos = 0U;
    std::string::size_type cur_pos = tmp_string.find(':', 0U);
    while (cur_pos != std::string::npos) {
        const std::string path = tmp_string.substr(start_pos, cur_pos - start_pos);
        if (!path.empty()) {
        path_vec.push_back(path);
        }
        start_pos = cur_pos + 1U;
        cur_pos = tmp_string.find(':', start_pos);
    }
}

std::string PluginManager::GetSoPath(const std::string &envValue) const
{
    char path_env[MAX_PATH_LENGTH]{};
    const char *env = getenv(envValue.c_str());
    strncpy(path_env, env, MAX_PATH_LENGTH);
    std::vector<std::string> path_vec;
    SplitPath(std::string(path_env), path_vec);
    for (auto path : path_vec) {
        std::string ret = path + "/" + so_name_;
        if (access(ret.c_str(), F_OK) != -1) {
            return ret;
        }
    }
    return "";
}

} // namespace Plugin
} // namespace Dvvp
} // namespace Analysis
