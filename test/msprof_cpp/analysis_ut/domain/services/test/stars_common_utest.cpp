/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : stars_common_utest.cpp
 * Description        : stars_common  UT
 * Author             : MSPROF TEAM
 * Creation Date      : 2024/07/12
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "analysis/csrc/domain/services/parser/parser_item/stars_common.h"

using namespace Analysis::Domain;

TEST(StarCommonUTest, ShouldStreamChangeLow12BitsWhenStreamBit13IsOne)
{
    auto res = StarsCommon::GetStreamId(8195, 8);
    EXPECT_EQ(8, res);
}

TEST(StarCommonUTest, ShouldReturnStreamWhenStreamBit13IsNotOne)
{
    auto res = StarsCommon::GetStreamId(3, 8);
    EXPECT_EQ(3, res);
}

TEST(StarCommonUTest, ShouldReturnStreamLow12BitsWhenStreamBit12IsOne)
{
    auto res = StarsCommon::GetStreamId(12291, 8);
    EXPECT_EQ(3, res);
}

TEST(StarCommonUTest, ShouldTaskChangeLow12BitsWhenStreamBit13IsOne)
{
    auto res = StarsCommon::GetTaskId(8195, 8);
    EXPECT_EQ(3, res);
}

TEST(StarCommonUTest, ShouldReturnTaskWhenStreamBit13AndBit12IsNotOne)
{
    auto res = StarsCommon::GetTaskId(3, 8);
    EXPECT_EQ(8, res);
}

TEST(StarCommonUTest, ShouldReturnTaskChangeHighThreeBitsWhenStreamBit12IsOne)
{
    auto res = StarsCommon::GetTaskId(20483, 8);
    EXPECT_EQ(16392, res);
}
