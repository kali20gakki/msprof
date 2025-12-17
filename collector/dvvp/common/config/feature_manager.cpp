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
#include "feature_manager.h"
#include "config_manager.h"
#include "errno/error_code.h"
#include "utils/utils.h"
#include "transport/uploader_mgr.h"

namespace Analysis {
namespace Dvvp {
namespace Common {
namespace Config {

using namespace analysis::dvvp::common::error;

namespace {
const char NULL_CHAR = '\0';
const std::string FILE_NAME = "incompatible_features.json";
// Platform: featureName, compatibility, featureVersion, affectedComponent, affectedComponentVersion, infoLog
std::unordered_map<PlatformType, std::vector<FeatureRecord>> FEATURE_V2_TABLE = {
    {
        PlatformType::CHIP_V4_1_0, {
            {"ATTR\0", "1\0", "2\0", "all\0", "all\0", "It not support feature: ATTR!\0"},
            {"MemoryAccess\0", "1\0", "2\0", "all\0", "all\0", "It not support feature: MemoryAccess!\0"}
        }
    },
    {
        PlatformType::END_TYPE, {
            {"ATTR\0", "1\0", "2\0", "all\0", "all\0", "It not support feature: ATTR!\0"}
        }
    }
};

// 仅V1接口使用 适配PTA代码逻辑 保留维护ATTR特性
std::vector<FeatureRecord> FEATURE_V1 = {
    {"ATTR\0", "1\0", "2\0", "all\0", "all\0", "It not support feature: ATTR!\0"}
};

std::vector<FeatureRecord>& GetCurPlatformFeature(bool isV2 = true)
{
    if (!isV2) {
        // V1 接口返回 V1 列表
        MSPROF_LOGI("Get V1 features.");
        return FEATURE_V1;
    }
    PlatformType platform = ConfigManager::instance()->GetPlatformType();
    auto iter = FEATURE_V2_TABLE.find(platform);
    // 当 非CHIP_V4_1_0 统一返回END_TYPE中的特性列表
    return ((iter == FEATURE_V2_TABLE.end()) ? FEATURE_V2_TABLE[PlatformType::END_TYPE] : iter->second);
}
}

FeatureManager::FeatureManager() {}

FeatureManager::~FeatureManager()
{
    Uninit();
}

int FeatureManager::Init()
{
    if (isInit_) {
        MSPROF_LOGI("FeatureManager has been inited.");
        return PROFILING_SUCCESS;
    }
    if (CheckCreateFeatures() != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    isInit_ = true;
    return PROFILING_SUCCESS;
}

int FeatureManager::CheckCreateFeatures()
{
    // 统一对V2 特性校验
    for (const auto& platformFeatures : FEATURE_V2_TABLE) {
        for (const auto& feature : platformFeatures.second) {
            if (feature.featureName[0] == NULL_CHAR || feature.info.affectedComponent[0] == NULL_CHAR ||
                feature.info.affectedComponentVersion[0] == NULL_CHAR || feature.info.compatibility[0] == NULL_CHAR ||
                feature.info.featureVersion[0] == NULL_CHAR || feature.info.infoLog[0] == NULL_CHAR) {
                MSPROF_LOGE("V2 Feature init failed.");
                return PROFILING_ERROR;
            }
        }
    }

    // 对V1 特性校验
    for (const auto& feature : FEATURE_V1) {
        if (feature.featureName[0] == NULL_CHAR || feature.info.affectedComponent[0] == NULL_CHAR ||
            feature.info.affectedComponentVersion[0] == NULL_CHAR || feature.info.compatibility[0] == NULL_CHAR ||
            feature.info.featureVersion[0] == NULL_CHAR || feature.info.infoLog[0] == NULL_CHAR) {
            MSPROF_LOGE("V1 Feature init failed.");
            return PROFILING_ERROR;
        }
    }
    return PROFILING_SUCCESS;
}

FeatureRecord* FeatureManager::GetIncompatibleFeatures(size_t* featuresSize, bool isV2)
{
    if (featuresSize == nullptr) {
        MSPROF_LOGE("featuresSize for GetIncompatibleFeatures is nullptr.");
        return nullptr;
    }
    auto& features = GetCurPlatformFeature(isV2);
    *featuresSize = features.size();
    return features.data();
}

int FeatureManager::CreateIncompatibleFeatureJsonFile(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    if (!isInit_ && Init() != PROFILING_SUCCESS) {
        MSPROF_LOGE("Init failed.");
        return PROFILING_FAILED;
    }
    nlohmann::json root;
    auto& features = GetCurPlatformFeature();
    for (auto& feature : features) {
        feature.ToObject(root);
    }
    std::string featureStr = root.dump();
    MSPROF_LOGI("Incompatible Features is: %s.", featureStr.c_str());
    SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx = nullptr;
    MSVP_MAKE_SHARED0_RET(jobCtx, analysis::dvvp::message::JobContext, PROFILING_FAILED);
    jobCtx->job_id = params->job_id;
    analysis::dvvp::transport::FileDataParams fileDataParams(FILE_NAME, true,
        analysis::dvvp::common::config::FileChunkDataModule::PROFILING_IS_CTRL_DATA);

    MSPROF_LOGI("job_id: %s,fileName: %s", params->job_id.c_str(), FILE_NAME.c_str());
    int ret = analysis::dvvp::transport::UploaderMgr::instance()->UploadCtrlFileData(params->job_id,
        featureStr, fileDataParams, jobCtx);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to upload data for %s", FILE_NAME.c_str());
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

void FeatureManager::Uninit()
{
    isInit_ = false;
}

}  // namespace Config
}  // namespace Comon
}  // namespace Dvvp
}  // namespace Analysis
