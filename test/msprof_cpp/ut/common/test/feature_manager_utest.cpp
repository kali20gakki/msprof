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
#include <iostream>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "config_manager.h"
#include "feature_manager.h"
#include "error_code.h"
#include "uploader_mgr.h"

using namespace Analysis::Dvvp::Common::Config;
using namespace analysis::dvvp::common::error;

class FeatureManagerUtest : public testing::Test {
protected:
    virtual void SetUp() {}
    virtual void TearDown()
    {
        GlobalMockObject::verify();
        FeatureManager::instance()->Uninit();
    }
};

TEST_F(FeatureManagerUtest, TestCreateIncompatibleFeatureJsonFileWhenNoInitAndInitFailedThenReturnFailed)
{
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
        new analysis::dvvp::message::ProfileParams);
    params->job_id = "1";
    auto featureManger = FeatureManager::instance();
    MOCKER_CPP(&analysis::dvvp::transport::UploaderMgr::UploadCtrlFileData)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&FeatureManager::CheckCreateFeatures).stubs().will(returnValue(PROFILING_ERROR));
    EXPECT_EQ(PROFILING_FAILED, featureManger->CreateIncompatibleFeatureJsonFile(params));
}

TEST_F(FeatureManagerUtest, TestCreateIncompatibleFeatureJsonFileWhenCreateSuccessReturnSuccess)
{
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
        new analysis::dvvp::message::ProfileParams);
    params->job_id = "1";
    auto featureManger = FeatureManager::instance();
    EXPECT_EQ(PROFILING_SUCCESS, featureManger->Init());
    MOCKER_CPP(&analysis::dvvp::transport::UploaderMgr::UploadCtrlFileData)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_SUCCESS, featureManger->CreateIncompatibleFeatureJsonFile(params));
}

TEST_F(FeatureManagerUtest, TestCreateIncompatibleFeatureJsonFileWhenCreateFailedReturnFailed)
{
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
        new analysis::dvvp::message::ProfileParams);
    params->job_id = "1";
    auto featureManger = FeatureManager::instance();
    EXPECT_EQ(PROFILING_SUCCESS, featureManger->Init());
    MOCKER_CPP(&analysis::dvvp::transport::UploaderMgr::UploadCtrlFileData)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    EXPECT_EQ(PROFILING_FAILED, featureManger->CreateIncompatibleFeatureJsonFile(params));
}

TEST_F(FeatureManagerUtest, TestInitWhenInitSuccessThenReturnSuccess)
{
    auto featureManger = FeatureManager::instance();
    EXPECT_EQ(PROFILING_SUCCESS, featureManger->Init());

    // repeat init
    EXPECT_EQ(PROFILING_SUCCESS, featureManger->Init());
}

TEST_F(FeatureManagerUtest, TestInitWhenCheckFeaturesFailedThenReturnFailed)
{
    auto featureManger = FeatureManager::instance();
    MOCKER_CPP(&FeatureManager::CheckCreateFeatures).stubs().will(returnValue(PROFILING_ERROR));
    EXPECT_EQ(PROFILING_FAILED, featureManger->Init());
}

TEST_F(FeatureManagerUtest, TestGetIncompatibleFeaturesSuccessWhenPointerNotNullptr)
{
    auto featureManger = FeatureManager::instance();
    MOCKER_CPP(&ConfigManager::GetPlatformType)
    .stubs()
    .will(returnValue(PlatformType::CHIP_V4_1_0))
    .then(returnValue(PlatformType::CHIP_V4_2_0));
    size_t dataSize = 0;
    size_t expectDataSize = 2;
    auto chipV410Feature = featureManger->GetIncompatibleFeatures(&dataSize);
    EXPECT_EQ(expectDataSize, dataSize);

    size_t expectDataSizeForOther = 1;
    auto otherFeature = featureManger->GetIncompatibleFeatures(&dataSize);
    EXPECT_EQ(expectDataSizeForOther, dataSize);

    size_t expectDataSizeForV1Func = 1;
    auto v1Feature = featureManger->GetIncompatibleFeatures(&dataSize, false);
    EXPECT_EQ(expectDataSizeForOther, dataSize);
}

TEST_F(FeatureManagerUtest, TestGetIncompatibleFeaturesFailedWhenPointerIsNullptr)
{
    auto featureManger = FeatureManager::instance();
    size_t* dataSizeptr = nullptr;
    EXPECT_EQ(nullptr, featureManger->GetIncompatibleFeatures(dataSizeptr));
}

TEST_F(FeatureManagerUtest, TestMemoryAccessCompatibleOnlyOnCHIPV410)
{
    auto featureManger = FeatureManager::instance();
    MOCKER_CPP(&ConfigManager::GetPlatformType)
        .stubs()
        .will(returnValue(PlatformType::CHIP_V4_1_0))
        .then(returnValue(PlatformType::CHIP_V4_2_0));
    size_t dataSize = 0;
    size_t expectDataSize = 2;
    auto chipV410Feature = featureManger->GetIncompatibleFeatures(&dataSize);
    EXPECT_EQ(expectDataSize, dataSize);
    size_t expectDataSizeForOther = 1;
    auto otherFeature = featureManger->GetIncompatibleFeatures(&dataSize);
    EXPECT_EQ(expectDataSizeForOther, dataSize);
}
