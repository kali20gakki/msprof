/*
 * @Copyright: Copyright (c) Huawei Technologies Co., Ltd. 2012-2018. All rights reserved.
 * @Description: 
 * @Date: 2020-06-22 11:16:23
 */
#include <string>
#include <memory>
#include <fstream>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "config/config_manager.h"
#include "config/config.h"
#include "utils/utils.h"
#include "ai_drv_dev_api.h"
#include "driver_plugin.h"
using namespace Analysis::Dvvp::Common::Config;
using namespace analysis::dvvp::common::error;
using namespace Collector::Dvvp::Plugin;

const std::string TYPE_CONFIG = "type";
const std::string FRQ_CONFIG = "frq";
const std::string AIC_CONFIG = "aicFrq";

class COMMON_CONFIG_MANAGER_TEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }

};

TEST_F(COMMON_CONFIG_MANAGER_TEST, Init)
{
    GlobalMockObject::verify();
    auto configManger = Analysis::Dvvp::Common::Config::ConfigManager::instance();
    MOCKER(&DriverPlugin::MsprofHalGetDeviceInfo)
            .stubs()
            .will(returnValue(DRV_ERROR_NONE));
    EXPECT_EQ(PROFILING_SUCCESS, configManger->Init());

    // repeat init
    EXPECT_EQ(PROFILING_SUCCESS, configManger->Init());
}

TEST_F(COMMON_CONFIG_MANAGER_TEST, GetAicoreEvents)
{
    GlobalMockObject::verify();
    std::string aicoreMetricsType = "";
    std::string aicoreEvents = "";
    auto configManger = Analysis::Dvvp::Common::Config::ConfigManager::instance();
    EXPECT_EQ(PROFILING_FAILED, configManger->GetAicoreEvents(aicoreMetricsType, aicoreEvents));
    
    aicoreMetricsType = "PipeUtilization";
    EXPECT_EQ(PROFILING_SUCCESS, configManger->GetAicoreEvents(aicoreMetricsType, aicoreEvents));
    EXPECT_EQ("0x8,0xa,0x9,0xb,0xc,0xd,0x54,0x55", aicoreEvents);

    aicoreMetricsType = "invalid";
    EXPECT_EQ(PROFILING_FAILED, configManger->GetAicoreEvents(aicoreMetricsType, aicoreEvents));
}

TEST_F(COMMON_CONFIG_MANAGER_TEST, GetPlatformType)
{
    GlobalMockObject::verify();
    auto configManger = Analysis::Dvvp::Common::Config::ConfigManager::instance();
    EXPECT_EQ(PlatformType::MINI_TYPE, configManger->GetPlatformType());
    configManger->configMap_[TYPE_CONFIG] = "0";
    configManger->isInit_ = true;
    EXPECT_EQ(PlatformType::MINI_TYPE, configManger->GetPlatformType());
    configManger->Uninit();
    configManger->configMap_[TYPE_CONFIG] = "0";
    MOCKER(&DriverPlugin::MsprofHalGetDeviceInfo)
            .stubs()
            .will(returnValue(MSPROF_HELPER_HOST))
            .then(returnValue(DRV_ERROR_INVALID_VALUE));
    configManger->Init();
    EXPECT_EQ(PlatformType::MDC_TYPE, configManger->GetPlatformType());
    configManger->Uninit();
    configManger->configMap_[TYPE_CONFIG] = "0";

    configManger->Init();
    EXPECT_EQ(PlatformType::MINI_TYPE, configManger->GetPlatformType());
    configManger->Uninit();
}

TEST_F(COMMON_CONFIG_MANAGER_TEST, IsDriverSupportLlc)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetPlatformType)
            .stubs()
            .will(returnValue(PlatformType::CLOUD_TYPE))
            .then(returnValue(PlatformType::MINI_TYPE));
    auto configManger = Analysis::Dvvp::Common::Config::ConfigManager::instance();
    EXPECT_EQ(true, configManger->IsDriverSupportLlc());
    EXPECT_EQ(false, configManger->IsDriverSupportLlc());
}

TEST_F(COMMON_CONFIG_MANAGER_TEST, GetL2cacheEvents)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetPlatformType)
            .stubs()
            .will(returnValue(PlatformType::MINI_TYPE))
            .then(returnValue(PlatformType::CLOUD_TYPE));
    auto configManger = Analysis::Dvvp::Common::Config::ConfigManager::instance();
    std::string l2CacheEvents;
    EXPECT_EQ(-1, configManger->GetL2cacheEvents(l2CacheEvents));
    EXPECT_EQ(0, configManger->GetL2cacheEvents(l2CacheEvents));
    
}

TEST_F(COMMON_CONFIG_MANAGER_TEST, MsprofL2CacheAdapter)
{
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
		new analysis::dvvp::message::ProfileParams());
    auto configManger = Analysis::Dvvp::Common::Config::ConfigManager::instance();
    params->l2CacheTaskProfiling = "";
    configManger->MsprofL2CacheAdapter(params);
    EXPECT_STREQ("off", params->l2CacheTaskProfiling.c_str());
    GlobalMockObject::verify();
    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetL2cacheEvents)
            .stubs()
            .will(returnValue(0))
            .then(returnValue(-1));
    params->l2CacheTaskProfiling = "on";
    configManger->MsprofL2CacheAdapter(params);
    EXPECT_STREQ("on", params->l2CacheTaskProfiling.c_str());
    configManger->MsprofL2CacheAdapter(params);
    EXPECT_STREQ("off", params->l2CacheTaskProfiling.c_str());
}

TEST_F(COMMON_CONFIG_MANAGER_TEST, GetFrequency)
{
    GlobalMockObject::verify();
    auto configManger = Analysis::Dvvp::Common::Config::ConfigManager::instance();
    configManger->configMap_.clear();
    EXPECT_EQ("", configManger->GetFrequency());
    configManger->configMap_[FRQ_CONFIG] = "300";
    EXPECT_EQ("300", configManger->GetFrequency());
}

TEST_F(COMMON_CONFIG_MANAGER_TEST, GetAicDefFrequency)
{
    GlobalMockObject::verify();
    auto configManger = Analysis::Dvvp::Common::Config::ConfigManager::instance();
    configManger->configMap_.clear();
    EXPECT_EQ("", configManger->GetAicDefFrequency());
    configManger->configMap_[AIC_CONFIG] = "300";
    EXPECT_EQ("300", configManger->GetAicDefFrequency());
}

TEST_F(COMMON_CONFIG_MANAGER_TEST, GetChipIdStr)
{
    GlobalMockObject::verify();
    auto configManger = Analysis::Dvvp::Common::Config::ConfigManager::instance();
    configManger->configMap_.clear();
    EXPECT_EQ("0", configManger->GetChipIdStr());
    configManger->configMap_[TYPE_CONFIG] = "4";
    EXPECT_EQ("4", configManger->GetChipIdStr());
}
