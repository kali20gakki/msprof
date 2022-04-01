#include "common/util/platform_info.h"
#include "debug/ge_log.h"
#include "graph/def_types.h"

namespace fe {

PlatformInfoManager &PlatformInfoManager::Instance() {
    static PlatformInfoManager platformInfo;
    return platformInfo;
}

PlatformInfoManager::PlatformInfoManager() {}

PlatformInfoManager::~PlatformInfoManager() {}

uint32_t PlatformInfoManager::InitializePlatformInfo() {
    return 0;
}

uint32_t PlatformInfoManager::Finalize() {
    return 0;
}

thread_local std::string socVersion;

uint32_t PlatformInfoManager::GetPlatformInfos(const string SoCVersion,
                                              PlatFormInfos &platformInfo,
                                              OptionalInfos &optiCompilationInfo) {
    socVersion = SoCVersion;
    return 0;
}

bool PlatFormInfos::GetPlatformRes(const std::string &label, const std::string &key, std::string &val)
{
    if (label == "CPUCache" && key == "AICPUSyncBySW") {
        if (socVersion.find("Hi") != string::npos || socVersion.find("SD") != string::npos) {
            val = "1";
        } else {
            val = "0";
        }
        return true;
    }
    return false;
}

}

