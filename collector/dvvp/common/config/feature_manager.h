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
    FeatureRecord(const char* tempFeatureName, const char* tempCompatibility, const char* tempFeatureVersion,
                  const char* tempAffectedComponent, const char* tempAffectedComponentVersion, const char* tempInfoLog)
                  noexcept
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
    FeatureRecord* GetIncompatibleFeatures(size_t* featuresSize, bool isV2 = true);
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
