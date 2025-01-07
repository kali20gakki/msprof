/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : sync_utils_utest.cpp
 * Description        : sync_utils UT
 * Author             : msprof team
 * Creation Date      : 2025/1/2
 * *****************************************************************************
 */

#include <gtest/gtest.h>
#include "analysis/csrc/infrastructure/dump_tools/include/sync_utils.h"
using namespace Analysis::Infra;
using namespace testing;

TEST(SyncUtilsUTest, ShouldGetSameTimeStrWhenMultiCallFunc)
{
    // 首次调用
    auto resStr1 = GetTimeStampStr();
    EXPECT_EQ(14ul, resStr1.size());

    // 再次调用
    auto resStr2 = GetTimeStampStr();
    EXPECT_EQ(resStr1, resStr2);
}