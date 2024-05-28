/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : metric_utest.cpp
 * Description        : metric UT
 * Author             : msprof team
 * Creation Date      : 2024/4/25
 * *****************************************************************************
 */

#include <gtest/gtest.h>
#include "analysis/csrc/domain/entities/metric/include/metric.h"

using namespace testing;
namespace Analysis {
namespace Domain {
class MetricUTest : public Test {
protected:
    void SetUp() override
    {}

    void TearDown() override
    {}
};

enum class InvalidType {
    TEST_INVALID = 0
};

TEST_F(MetricUTest, ShouldReturnRightHeaderStringWhenInputEnumType)
{
    ASSERT_EQ("mac_fp16_ratio", Metric::GetMetricHeaderString(ArithMetricIndex::MacFp16Ratio));
    ASSERT_EQ("INVALID", Metric::GetMetricHeaderString(InvalidType::TEST_INVALID));
}
}
}