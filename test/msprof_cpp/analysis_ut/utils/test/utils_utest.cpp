/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : utils_utest.cpp
 * Description        : Utils UT
 * Author             : msprof team
 * Creation Date      : 2023/11/16
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "utils.h"
#include "error_code.h"

using namespace Analysis::Utils;
using namespace Analysis;

class UtilsUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

TEST_F(UtilsUTest, TestJoinShouldReturn)
{
    std::vector<std::string> str {
        "test",
        "join",
        "utils",
    };
    std::string joinedStr = Join(str, "./");
    EXPECT_EQ("test./join./utils", joinedStr);
}

TEST_F(UtilsUTest, TestSplitShouldNotSplitWhenDelimiterIsEmpty)
{
    auto splitStr = Split("test.split", "");
    ASSERT_EQ(1, splitStr.size());
    EXPECT_EQ("test.split", splitStr[0]);
}

TEST_F(UtilsUTest, TestSplitShouldReturn3SplitedStringWhenDelimiterNotEmpty)
{
    std::vector<std::string> splitStr = Split("test//split//utils", "//");
    const int splitSize = 3;
    ASSERT_EQ(splitSize, splitStr.size());
    const std::vector<std::string> res = {"test", "split", "utils"};
    EXPECT_EQ(res, splitStr);
}

TEST_F(UtilsUTest, TestStrToU16ShouldReturnOKWhenStrIsNumber)
{
    uint16_t dest;
    uint16_t expectRes = 11;
    EXPECT_EQ(StrToU16(dest, "11"), ANALYSIS_OK);
    ASSERT_EQ(dest, expectRes);
}

TEST_F(UtilsUTest, TestStrToU16ShouldReturnERRORWhenStrIsNotNumber)
{
    uint16_t dest;
    EXPECT_EQ(StrToU16(dest, "11tt"), ANALYSIS_ERROR);
}

TEST_F(UtilsUTest, TestStrToU16ShouldReturnERRORWhenStrIsBeyondThreshold)
{
    uint16_t dest;
    EXPECT_EQ(StrToU16(dest, "999999999999999999"), ANALYSIS_ERROR);
}

TEST_F(UtilsUTest, TestStrToU64ShouldReturnOKWhenStrIsNumber)
{
    uint64_t dest;
    uint64_t expectRes = 11;
    EXPECT_EQ(StrToU64(dest, "11"), ANALYSIS_OK);
    ASSERT_EQ(dest, expectRes);
}

TEST_F(UtilsUTest, TestStrToU64ShouldReturnERRORWhenStrIsNotNumber)
{
    uint64_t dest;
    EXPECT_EQ(StrToU64(dest, "11tt"), ANALYSIS_ERROR);
}

TEST_F(UtilsUTest, TestStrToU64ShouldReturnERRORWhenStrIsBeyondThreshold)
{
    uint64_t dest;
    std::string errorStr = "999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999";
    EXPECT_EQ(StrToU64(dest, errorStr), ANALYSIS_ERROR);
}

TEST_F(UtilsUTest, TestConvertToStringShouldReturnCorrectStrWhenInputVariousTypeItems)
{
    uint32_t testNum1 = 1000000;
    uint64_t testNum2 = 90000000000;
    double testNum3 = 3.14;
    uint64_t testNum4 = 70000000000;
    std::string testStr1 = "Mind";
    std::string testStr2 = "Studio";
    std::string expectStr = "1000000_90000000000_3.14_70000000000_Mind_Studio";

    // 测试拼接正确性 & 嵌套调用正确性(函数参数超过5个可以嵌套调用)
    auto ret = ConvertToString(ConvertToString(testNum1, testNum2, testNum3, testNum4),
                               testStr1, testStr2);

    EXPECT_EQ(expectStr, ret);
}

TEST_F(UtilsUTest, TestStrToDoubleShouldReturnOKWhenStrIsNumber)
{
    double dest;
    double expectRes = 12345678.123;
    EXPECT_EQ(StrToDouble(dest, "12345678.123"), ANALYSIS_OK);
    ASSERT_EQ(dest, expectRes);
}

TEST_F(UtilsUTest, TestStrToDoubleShouldReturnERRORWhenStrIsNotNumber)
{
    uint64_t dest;
    EXPECT_EQ(StrToU64(dest, "11tt.1"), ANALYSIS_ERROR);
}

TEST_F(UtilsUTest, TestStrToDoubleShouldReturnERRORWhenStrIsEmpty)
{
    uint64_t dest;
    EXPECT_EQ(StrToU64(dest, ""), ANALYSIS_ERROR);
}

TEST_F(UtilsUTest, TestStrToDoubleShouldReturnERRORWhenStrIsBeyondThreshold)
{
    uint64_t dest;
    std::string errorStr = "99999999999999999999999999999999999999999999988888888.111111111111111111111111";
    EXPECT_EQ(StrToU64(dest, errorStr), ANALYSIS_ERROR);
}

TEST_F(UtilsUTest, TestGetDeviceIdByDevicePathShouldReturnHostIdWhenPathIsHostOrNoDevice)
{
    uint16_t hostId = 64;
    std::string hostPath = "abc_123/efg_789/PROF_XXX/host";
    EXPECT_EQ(GetDeviceIdByDevicePath(hostPath), hostId);

    std::string errorPath = "abc_123/efg_789/PROF_XXX/jkl";
    EXPECT_EQ(GetDeviceIdByDevicePath(errorPath), hostId);

    std::string hostSlash = "abc_123/efg_789/PROF_XXX/host/////";
    EXPECT_EQ(GetDeviceIdByDevicePath(hostSlash), hostId);

    std::string strToU16FailPath = "abc_123/efg_789/PROF_XXX/device_a";
    EXPECT_EQ(GetDeviceIdByDevicePath(strToU16FailPath), hostId);
}

TEST_F(UtilsUTest, TestGetDeviceIdByDevicePathShouldReturnDeviceIdWhenPathIsDevice)
{
    uint16_t expectId = 5;
    std::string devPath = "abc_123/efg_789/PROF_XXX/device_5";
    EXPECT_EQ(GetDeviceIdByDevicePath(devPath), expectId);

    std::string deviceSlash = "abc_123/efg_789/PROF_XXX/device_5/////";
    EXPECT_EQ(GetDeviceIdByDevicePath(deviceSlash), expectId);
}