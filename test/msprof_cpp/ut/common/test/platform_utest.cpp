/*
 * @Copyright: Copyright (c) Huawei Technologies Co., Ltd. 2012-2018. All rights reserved.
 * @Description:
 * @Date: 2023-07-28 15:12:23
 */

#include <memory>
#include <fstream>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "platform/platform.h"
#include "ai_drv_dev_api.h"

using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::driver;
using namespace Analysis::Dvvp::Common::Platform;

class PlatformUtest : public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(PlatformUtest, Init)
{
    GlobalMockObject::verify();
    auto platform = Analysis::Dvvp::Common::Platform::Platform::instance();
    uint32_t platformType = 0;
    MOCKER(analysis::dvvp::driver::DrvGetApiVersion)
            .stubs()
            .will(returnValue(SUPPORT_OSC_FREQ_API_VERSION));
    MOCKER(analysis::dvvp::driver::DrvGetDeviceFreq)
            .stubs()
            .will(returnValue(true));
    MOCKER(analysis::dvvp::driver::DrvGetPlatformInfo)
            .stubs()
            .with(outBound(platformType))
            .will(returnValue(PROFILING_SUCCESS));
    MOCKER(analysis::dvvp::driver::DrvCheckIfHelperHost)
            .stubs()
            .will(returnValue(true));
    platform->Init();
    EXPECT_EQ(true, platform->PlatformHostFreqIsEnable());
    EXPECT_EQ(true, platform->RunSocSide());
    EXPECT_EQ(true, platform->PlatformIsHelperHostSide());
}

TEST_F(PlatformUtest, Uninit)
{
    // for coverage
    auto platform = Analysis::Dvvp::Common::Platform::Platform::instance();
    EXPECT_EQ(PROFILING_SUCCESS, platform->Uninit());
}

TEST_F(PlatformUtest, PlatformGetDeviceOscFreq)
{
    GlobalMockObject::verify();
    auto platform = Analysis::Dvvp::Common::Platform::Platform::instance();
    std::string deviceOscFreq = "1000";
    std::string freq;
    MOCKER(&DrvGetApiVersion)
            .stubs()
            .will(returnValue(SUPPORT_OSC_FREQ_API_VERSION));
    MOCKER(&DrvGetDeviceFreq)
            .stubs()
            .with(any(), outBound(deviceOscFreq))
            .will(returnValue(true));
    EXPECT_EQ(deviceOscFreq, platform->PlatformGetDeviceOscFreq(0, freq));
}

TEST_F(PlatformUtest, PlatformIsSocSideWillReturnTrueWhenPlatformIsDeviceType)
{
    GlobalMockObject::verify();
    auto platform = Analysis::Dvvp::Common::Platform::Platform::instance();
    platform->platformType_ = SysPlatformType::DEVICE;
    EXPECT_EQ(true, platform->PlatformIsSocSide());
}

TEST_F(PlatformUtest, PlatformIsSocSideWillReturnTrueWhenPlatformIsLhisiType)
{
    GlobalMockObject::verify();
    auto platform = Analysis::Dvvp::Common::Platform::Platform::instance();
    platform->platformType_ = SysPlatformType::LHISI;
    EXPECT_EQ(true, platform->PlatformIsSocSide());
}

TEST_F(PlatformUtest, PlatformIsSocSideWillReturnFalseWhenPlatformIsNotDeviceOrLhisiType)
{
    GlobalMockObject::verify();
    auto platform = Analysis::Dvvp::Common::Platform::Platform::instance();
    platform->platformType_ = SysPlatformType::HOST;
    EXPECT_EQ(false, platform->PlatformIsSocSide());
}

TEST_F(PlatformUtest, PlatformIsRpcSideWillReturnTrueWhenPlatformIsHostType)
{
    GlobalMockObject::verify();
    auto platform = Analysis::Dvvp::Common::Platform::Platform::instance();
    platform->platformType_ = SysPlatformType::HOST;
    EXPECT_EQ(true, platform->PlatformIsRpcSide());
}

TEST_F(PlatformUtest, PlatformIsRpcSideWillReturnFalseWhenPlatformIsNotHostType)
{
    GlobalMockObject::verify();
    auto platform = Analysis::Dvvp::Common::Platform::Platform::instance();
    platform->platformType_ = SysPlatformType::DEVICE;
    EXPECT_EQ(false, platform->PlatformIsRpcSide());
}
