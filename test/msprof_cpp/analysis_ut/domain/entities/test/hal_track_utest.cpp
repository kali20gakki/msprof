/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hal_track_utest.cpp
 * Description        : hal_track UT
 * Author             : msprof team
 * Creation Date      : 2024/4/25
 * *****************************************************************************
 */

#include <vector>
#include <gtest/gtest.h>
#include "analysis/csrc/domain/entities/hal/include/hal_track.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"

using namespace testing;
using namespace Analysis;
using namespace Analysis::Infra;
using namespace Analysis::Domain;

class HalTrackUTest : public testing::Test {
protected:
    DataInventory dataInventory_;
protected:
    void SetUp() override
    {
        std::vector<HalTrackData> vec{
            {{}, HalTrackType::TASK_FLIP_BEAN, {}},
            {{}, HalTrackType::TASK_FLIP_BEAN, {}},
            {{}, HalTrackType::TASK_FLIP_BEAN, {}},
            {{}, HalTrackType::INVALID_BEAN, {}},
        };
        std::shared_ptr<std::vector<HalTrackData>> data;
        MAKE_SHARED0_NO_OPERATION(data, std::vector<HalTrackData>, std::move(vec));
        dataInventory_.Inject(data);
    }

    void TearDown() override
    {
        dataInventory_.RemoveRestData({});
    }
};

TEST_F(HalTrackUTest, ShouldSplitHalTrackDataByTrackType)
{
    auto data = dataInventory_.GetPtr<std::vector<HalTrackData>>();
    auto result = ClassifyTrackData(*data);
    ASSERT_EQ(result.size(), 2ul);
    ASSERT_EQ(result[HalTrackType::TASK_FLIP_BEAN].size(), 3ul);
    ASSERT_EQ(result[HalTrackType::INVALID_BEAN].size(), 1ul);
}

TEST_F(HalTrackUTest, ShouldReturnFlipBeanWhenInputTaskFlipBean)
{
    auto data = dataInventory_.GetPtr<std::vector<HalTrackData>>();
    auto result = GetTrackDataByType(*data, HalTrackType::TASK_FLIP_BEAN);
    ASSERT_EQ(result.size(), 3ul);
    ASSERT_EQ(result[0]->type, HalTrackType::TASK_FLIP_BEAN);
}