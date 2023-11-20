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

