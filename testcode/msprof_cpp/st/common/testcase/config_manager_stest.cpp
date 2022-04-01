#include <string>
#include <memory>
#include <fstream>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "config/config_manager.h"
#include "config/config.h"
#include "utils/utils.h"
#include "ai_drv_dev_api.h"
using namespace Analysis::Dvvp::Common::Config;
using namespace analysis::dvvp::common::error;
static const std::string TYPE_CONFIG = "type";

class COMMON_CONFIG_MANAGER_STEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }

};

TEST_F(COMMON_CONFIG_MANAGER_STEST, GetPlatformType)
{
    GlobalMockObject::verify();
    auto configManger = Analysis::Dvvp::Common::Config::ConfigManager::instance();
    EXPECT_EQ(PlatformType::MINI_TYPE, configManger->GetPlatformType());
    configManger->configMap_[TYPE_CONFIG] = "0";
    configManger->isInit_ = true;
    EXPECT_EQ(PlatformType::MINI_TYPE, configManger->GetPlatformType());
    configManger->Uninit();
    configManger->configMap_[TYPE_CONFIG] = "0";
    MOCKER(halGetDeviceInfo)
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
