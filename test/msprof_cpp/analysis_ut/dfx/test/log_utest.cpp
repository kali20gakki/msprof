/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : log_utest.cpp
 * Description        : Log UT
 * Author             : msprof team
 * Creation Date      : 2023/11/13
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "log.h"
#include "error_code.h"

using namespace Analysis;
using namespace Analysis::Utils;

class LogUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

TEST_F(LogUTest, TestFormatShouldReturnFormatStringWhenNoArgs)
{
    ASSERT_EQ("test format", Format("test format"));
}

TEST_F(LogUTest, TestFormatShouldReplacePercentSignWhenArgs)
{
    ASSERT_EQ("test format key: 1", Format("test format %: %", "key", 1));
}

TEST_F(LogUTest, TestFormatShouldReturnPercentSignWhenDoubelPercentSign)
{
    ASSERT_EQ("test format key: 1 %", Format("test format %: % %%", "key", 1));
}
