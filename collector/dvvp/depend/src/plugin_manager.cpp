#include "plugin_manager.h"
#include "utils.h"
#include "config.h"


namespace Analysis {
namespace Dvvp {
namespace Plugin {

using namespace analysis::dvvp::common::config;

const string PluginManager::GetSoName() const
{
    return so_name_;
}

Status PluginManager::OpenPlugin(const std::string& path)
{
    if (path.empty() || path.size() >= MAX_PATH_LENGTH) {
        return PLUGIN_LOAD_FAILED;
    }
    std::string absoluteDir = Utils::RelativePathToAbsolutePath(path);
    if (absoluteDir.empty()) {
        return PLUGIN_LOAD_FAILED;
    }

    handle_ = mmDlopen(absoluteDir.c_str(),
        static_cast<int32_t>(static_cast<uint32_t>(MMPA_RTLD_NOW) |
        static_cast<uint32_t>(MMPA_RTLD_GLOBAL)));
    if (!handle) {
        return PLUGIN_LOAD_FAILED;
    }
    return PLUGIN_LOAD_SUCCESS;
}

Status PluginManager::CloseHandle()
{
    if (!handle_) {
        // [To add message]
        return PLUGIN_LOAD_FAILED;
    }
    if (!mmDlclose(handle)) {
        // [To add message]
        return PLUGIN_LOAD_FAILED;
    }
    return PLUGIN_LOAD_SUCCESS;
}


} // namespace Plugin
} // namespace Dvvp
} // namespace Analysis
