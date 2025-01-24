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
#include "analysis/csrc/infrastructure/utils/utils.h"

#include "gtest/gtest.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/domain/services/environment/context.h"


using namespace Analysis::Utils;
using namespace Analysis;
using namespace Domain::Environment;

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
    uint16_t expectRes = 1;
    ASSERT_EQ(expectRes, splitStr.size());
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

TEST_F(UtilsUTest, TestSplitShouldReturnVectorWithTwoElements)
{
    const int splitPosition = 1;
    auto res = Split("key1:value1:key2:value2", ":", splitPosition);
    uint16_t expectRes = 2;
    ASSERT_EQ(expectRes, res.size());
    EXPECT_EQ("key1", res[0]);
    EXPECT_EQ("value1:key2:value2", res[1]);
}

TEST_F(UtilsUTest, TestSplitShouldReturnVectorWithTwoElementsAndSplitPositionEquals2)
{
    const int splitPosition = 2;
    auto res = Split("key1:value1:key2:value2", ":", splitPosition);
    uint16_t expectRes = 2;
    ASSERT_EQ(expectRes, res.size());
    EXPECT_EQ("key1:value1", res[0]);
    EXPECT_EQ("key2:value2", res[1]);
}

TEST_F(UtilsUTest, TestSplitShouldReturnVectorWithTwoElementsAndSplitPositionEquals3)
{
    const int splitPosition = 3;
    auto res = Split("key1:value1:key2:value2", ":", splitPosition);
    uint16_t expectRes = 2;
    ASSERT_EQ(expectRes, res.size());
    EXPECT_EQ("key1:value1:key2", res[0]);
    EXPECT_EQ("value2", res[1]);
}

TEST_F(UtilsUTest, TestSplitShouldReturnVectorWithEmptyValueWhenDelimiterAtEndOfString)
{
    const int splitPosition = 1;
    auto res = Split("key1::", ":", splitPosition);
    uint16_t expectRes = 2;
    ASSERT_EQ(expectRes, res.size());
    EXPECT_EQ("key1", res[0]);
    EXPECT_EQ(":", res[1]);
}

TEST_F(UtilsUTest, TestSplitShouldReturnVectorWithEmptyKeyWhenDelimiterAtBeginningOfString)
{
    const int splitPosition = 1;
    auto res = Split("::value", ":", splitPosition);
    uint16_t expectRes = 2;
    ASSERT_EQ(expectRes, res.size());
    EXPECT_EQ("", res[0]);
    EXPECT_EQ(":value", res[1]);
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

TEST_F(UtilsUTest, TestStrToU32ShouldReturnOKWhenStrIsNumber)
{
    uint32_t dest;
    uint32_t expectRes = 11;
    EXPECT_EQ(StrToU32(dest, "11"), ANALYSIS_OK);
    ASSERT_EQ(dest, expectRes);
}

TEST_F(UtilsUTest, TestStrToU32ShouldReturnERRORWhenStrIsNotNumber)
{
    uint32_t dest;
    EXPECT_EQ(StrToU32(dest, "11tt"), ANALYSIS_ERROR);
}

TEST_F(UtilsUTest, TestStrToU32ShouldReturnERRORWhenStrIsBeyondThreshold)
{
    uint32_t dest;
    std::string errorStr = "999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999";
    EXPECT_EQ(StrToU32(dest, errorStr), ANALYSIS_ERROR);
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
    auto ret = Join("_", Join("_", testNum1, testNum2, testNum3, testNum4),
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
    double dest;
    EXPECT_EQ(StrToDouble(dest, "11tt.1"), ANALYSIS_ERROR);
}

TEST_F(UtilsUTest, TestStrToDoubleShouldReturnERRORWhenStrIsEmpty)
{
    double dest;
    EXPECT_EQ(StrToDouble(dest, ""), ANALYSIS_ERROR);
}

TEST_F(UtilsUTest, TestGetDeviceIdByDevicePathShouldReturnHostIdWhenPathIsHostOrNoDevice)
{
    uint16_t hostId = HOST_ID;
    std::string hostPath = "abc_123/efg_789/PROF_XXX/host";
    EXPECT_EQ(GetDeviceIdByDevicePath(hostPath), hostId);

    std::string errorPath = "abc_123/efg_789/PROF_XXX/jkl";
    EXPECT_EQ(GetDeviceIdByDevicePath(errorPath), INVALID_DEVICE_ID);

    std::string hostSlash = "abc_123/efg_789/PROF_XXX/host/////";
    EXPECT_EQ(GetDeviceIdByDevicePath(hostSlash), hostId);

    std::string strToU16FailPath = "abc_123/efg_789/PROF_XXX/device_a";
    EXPECT_EQ(GetDeviceIdByDevicePath(strToU16FailPath), INVALID_DEVICE_ID);
}

TEST_F(UtilsUTest, TestGetDeviceIdByDevicePathShouldReturnDeviceIdWhenPathIsDevice)
{
    uint16_t expectId = 5;
    std::string devPath = "abc_123/efg_789/PROF_XXX/device_5";
    EXPECT_EQ(GetDeviceIdByDevicePath(devPath), expectId);

    std::string deviceSlash = "abc_123/efg_789/PROF_XXX/device_5/////";
    EXPECT_EQ(GetDeviceIdByDevicePath(deviceSlash), expectId);
}

TEST_F(UtilsUTest, TestDivideByPowersOfTenWithPrecisionShouldReturnTrueValue)
{
    // 长度超过3位,移动3位，精度3位
    uint64_t value = 123456; // 入参123456
    EXPECT_EQ("123.456", DivideByPowersOfTenWithPrecision(value));
    // 长度低于3位，移动3位，精度3位
    value = 12;  // 入参12
    EXPECT_EQ("0.012", DivideByPowersOfTenWithPrecision(value));

    value = 23456;  // 入参23456
    EXPECT_EQ("23.4560", DivideByPowersOfTenWithPrecision(value, 4, 3)); // 长度高于3位，移动3位，精度4位

    value = 58;  // 入参58
    EXPECT_EQ("0.0580", DivideByPowersOfTenWithPrecision(value, 4, 3)); // 长度低于3位，移动3位，精度4位

    value = 1234567;  // 入参1234567
    EXPECT_EQ("1234.56", DivideByPowersOfTenWithPrecision(value, 2, 3));  // 长度高于3位，移动3位，精度2位

    value = 78; // 入参78
    EXPECT_EQ("0.07", DivideByPowersOfTenWithPrecision(value, 2, 3)); // 长度小于3位，移动3位，精度2位
}