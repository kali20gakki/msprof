/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * Description: incompatible features manager
 * Create: 2024-04-22
 */
#include "feature_manager.h"

#include "errno/error_code.h"
#include "utils/utils.h"
#include "transport/uploader_mgr.h"

namespace Analysis {
namespace Dvvp {
namespace Common {
namespace Config {

using namespace analysis::dvvp::common::error;

namespace {
// featureName, compatibility, featureVersion, affectedComponent, affectedComponentVersion, infoLog
static FeatureRecord g_features[] = {
    {"ATTR\0", "0\0", "1\0", "all\0", "all\0", "It not support feature: ATTR!\0"},
};
static const std::string FILE_NAME = "incompatible_features.json";
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
    for (const auto& feature : g_features) {
        if (feature.featureName[0] == '\0' || feature.info.affectedComponent[0] == '\0' ||
            feature.info.affectedComponentVersion[0] == '\0' || feature.info.compatibility[0] == '\0' ||
            feature.info.featureVersion[0] == '\0' || feature.info.infoLog[0] == '\0') {
                MSPROF_LOGE("Feature init failed.");
                return PROFILING_ERROR;
        }
    }
    return PROFILING_SUCCESS;
}

FeatureRecord* FeatureManager::GetIncompatibleFeatures(size_t* featuresSize)
{
    if (featuresSize == nullptr) {
        MSPROF_LOGE("featuresSize for GetIncompatibleFeatures is nullptr.");
        return nullptr;
    }
    *featuresSize = sizeof(g_features) / sizeof(FeatureRecord);
    return &g_features[0];
}

int FeatureManager::CreateIncompatibleFeatureJsonFile(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    if (!isInit_ && Init() != PROFILING_SUCCESS) {
        MSPROF_LOGE("Init failed.");
        return PROFILING_FAILED;
    }
    nlohmann::json root;
    for (auto& feature : g_features) {
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