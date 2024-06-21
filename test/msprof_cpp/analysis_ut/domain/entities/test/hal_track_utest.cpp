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
const uint16_t DEFAULT_STREAM_ID = 1;

class HalTrackUTest : public testing::Test {
protected:
    DataInventory dataInventory_;
protected:
    void SetUp() override
    {
        std::vector<HalTrackData> vec{
            createHalTrackData(HalTrackType::TS_TASK_FLIP),
            createHalTrackData(HalTrackType::TS_TASK_FLIP),
            createHalTrackData(HalTrackType::TS_TASK_FLIP),
            createHalTrackData(HalTrackType::INVALID_TYPE),
        };
        std::shared_ptr<std::vector<HalTrackData>> data;
        MAKE_SHARED0_NO_OPERATION(data, std::vector<HalTrackData>, std::move(vec));
        dataInventory_.Inject(data);
    }

    void TearDown() override
    {
        dataInventory_.RemoveRestData({});
    }

    HalTrackData createHalTrackData(HalTrackType type)
    {
        HalTrackData ans = {};
        ans.hd.taskId.streamId = DEFAULT_STREAM_ID;
        ans.type = type;
        ans.flip = {};
        return ans;
    }
};

TEST_F(HalTrackUTest, ShouldReturnFlipData)
{
    auto data = dataInventory_.GetPtr<std::vector<HalTrackData>>();
    auto result = GetFlipData(*data);
    ASSERT_EQ(1ul, result.size());
    ASSERT_EQ(HalTrackType::TS_TASK_FLIP, result[DEFAULT_STREAM_ID][0]->type);
}

TEST_F(HalTrackUTest, ShouldReturnFlipBeanWhenInputTaskFlipBean)
{
    auto data = dataInventory_.GetPtr<std::vector<HalTrackData>>();
    auto result = GetTrackDataByType(*data, HalTrackType::TS_TASK_FLIP);
    ASSERT_EQ(3ul, result.size());
    ASSERT_EQ(HalTrackType::TS_TASK_FLIP, result[0].type);
}