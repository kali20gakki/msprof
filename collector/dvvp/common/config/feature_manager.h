/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * Description: incompatible features manager
 * Create: 2024-04-19
 */
#ifndef ANALYSIS_DVVP_COMMON_CONFIG_FEATURE_MANAGER_H
#define ANALYSIS_DVVP_COMMON_CONFIG_FEATURE_MANAGER_H
#include <vector>
#include <algorithm>

#include "message/message.h"
#include "message/prof_params.h"
#include "singleton/singleton.h"

namespace Analysis {
namespace Dvvp {
namespace Common {
namespace Config {

struct FeatureInfo : analysis::dvvp::message::BaseInfo {
    char compatibility[16] = "\0";
    char featureVersion[16] = "\0";
    char affectedComponent[16] = "\0";
    char affectedComponentVersion[16] = "\0";
    char infoLog[128] = "\0";
    FeatureInfo() = default;
    virtual ~FeatureInfo() {}

    void ToObject(nlohmann::json &object) override
    {
        SET_VALUE(object, compatibility);
        SET_VALUE(object, featureVersion);
        SET_VALUE(object, affectedComponent);
        SET_VALUE(object, affectedComponentVersion);
        SET_VALUE(object, infoLog);
    }
    void FromObject(const nlohmann::json &object) override {}
};

struct FeatureRecord {
    char featureName[64] = "\0";
    FeatureInfo info;
    FeatureRecord() = default;
    FeatureRecord(char* tempFeatureName, char* tempCompatibility, char* tempFeatureVersion,
                  char* tempAffectedComponent, char* tempAffectedComponentVersion, char* tempInfoLog) noexcept
    {
        // 0 tempData, 1 structData, 2 structDataSize
        std::vector<std::tuple<const char*, char*, size_t>> copyList = {
            {tempFeatureName, featureName, sizeof(featureName)},
            {tempCompatibility, info.compatibility, sizeof(info.compatibility)},
            {tempFeatureVersion, info.featureVersion, sizeof(info.featureVersion)},
            {tempAffectedComponent, info.affectedComponent, sizeof(info.affectedComponent)},
            {tempAffectedComponentVersion, info.affectedComponentVersion, sizeof(info.affectedComponentVersion)},
            {tempInfoLog, info.infoLog, sizeof(info.infoLog)},
        };
        auto ret = std::all_of(copyList.begin(), copyList.end(), [](std::tuple<const char*, char*, size_t>& copyNode) {
            return strcpy_s(std::get<1>(copyNode), std::get<2>(copyNode), std::get<0>(copyNode)) == EOK;
        });
        if (!ret) {
            return;
        }
    }

    virtual ~FeatureRecord() {}

    void ToObject(nlohmann::json &object)
    {
        std::vector<FeatureInfo> infos = {info};
        SET_ARRAY_OBJECT_VALUE(object, infos, featureName);
    }
};


class FeatureManager : public analysis::dvvp::common::singleton::Singleton<FeatureManager> {
public:
    FeatureManager();
    virtual ~FeatureManager();
    int Init();
    void Uninit();
    int CreateIncompatibleFeatureJsonFile(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    FeatureRecord* GetIncompatibleFeatures(size_t* featuresSize);
private:
    int CheckCreateFeatures();
private:
    bool isInit_ = false;
};


}  // namespace Config
}  // namespace Comon
}  // namespace Dvvp
}  // namespace Analysis

#endif // ANALYSIS_DVVP_COMMON_CONFIG_FEATURE_MANAGER_H
