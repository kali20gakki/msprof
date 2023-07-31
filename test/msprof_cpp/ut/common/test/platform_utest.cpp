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
    MOCKER(analysis::dvvp::driver::DrvGetHostFreq)
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