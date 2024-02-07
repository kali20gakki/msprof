/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : time_utils_utest.cpp
 * Description        : TimeUtils UT
 * Author             : msprof team
 * Creation Date      : 2024/01/03
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "analysis/csrc/utils/time_utils.h"

using namespace Analysis::Utils;

class TimeUtilsUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
    }

    static void TearDownTestCase()
    {
    }

    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }
};

TEST_F(TimeUtilsUTest, TestGetTimeFromSyscntShouldReturnOriSysCntWhenFreqIsDefault)
{
    SyscntConversionParams params{DEFAULT_FREQ, 3666503140109, 36471130547330};
    uint64_t taskSysCnt = 3666510676667;
    double expectRes = 3666510676667.000;
    EXPECT_DOUBLE_EQ(GetTimeFromSyscnt(taskSysCnt, params).Double(), expectRes);
}

TEST_F(TimeUtilsUTest, TestGetTimeFromSyscntShouldReturnTimestamp)
{
    SyscntConversionParams params{100.0, 3666503140109, 36471130547330};
    uint64_t taskSysCnt = 3666510676667;
    double expectRes = 36471205912910.000;
    EXPECT_DOUBLE_EQ(GetTimeFromSyscnt(taskSysCnt, params).Double(), expectRes);
}

TEST_F(TimeUtilsUTest, TestGetLocalTimeShouldReturnLocalTimestamp)
{
    SyscntConversionParams params{100.0, 3666503140109, 36471130547330};
    ProfTimeRecord record{1701069324370978000, 1701069338159976000, 36471129942580};
    uint64_t taskSysCnt = 3666510676667;
    std::string expectRes = "1701069324446948.33";
    auto timestamp = GetTimeFromSyscnt(taskSysCnt, params);
    EXPECT_EQ(GetLocalTime(timestamp, record).Str(), expectRes);
}