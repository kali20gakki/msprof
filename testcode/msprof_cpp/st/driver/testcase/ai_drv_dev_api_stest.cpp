#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "ai_drv_dev_api.h"
#include "errno/error_code.h"
#include "msprof_dlog.h"
#include "ascend_hal.h"

using namespace analysis::dvvp::driver;
using namespace analysis::dvvp::common::error;

class DRV_DEV_API_STEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(DRV_DEV_API_STEST, DrvGetPlatformInfo)
{
    GlobalMockObject::verify();
    MOCKER(drvGetPlatformInfo)
        .stubs()
        .will(returnValue(DRV_ERROR_NONE))
        .then(returnValue(MSPROF_HELPER_HOST))
        .then(returnValue(DRV_ERROR_INVALID_VALUE));
    uint32_t platformInfo;
    EXPECT_EQ(PROFILING_SUCCESS, DrvGetPlatformInfo(platformInfo));
    EXPECT_EQ(PROFILING_SUCCESS, DrvGetPlatformInfo(platformInfo));
    EXPECT_EQ(PROFILING_FAILED, DrvGetPlatformInfo(platformInfo));
}

TEST_F(DRV_DEV_API_STEST, DrvCheckIfHelperHost) {
    GlobalMockObject::verify();
    MOCKER(drvGetDevNum)
        .stubs()
        .will(returnValue(DRV_ERROR_NONE))
        .then(returnValue(MSPROF_HELPER_HOST));

    EXPECT_EQ(false, analysis::dvvp::driver::DrvCheckIfHelperHost());
    EXPECT_EQ(true, analysis::dvvp::driver::DrvCheckIfHelperHost());
}
