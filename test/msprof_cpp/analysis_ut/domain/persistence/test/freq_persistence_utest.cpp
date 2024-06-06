/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : freq_persistence_utest.cpp
 * Description        : freq数据持久化 UT
 * Author             : msprof team
 * Creation Date      : 2024/5/17
 * *****************************************************************************
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "analysis/csrc/domain/services/persistence/freq_persistence.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/entities/hal/include/hal_freq.h"
#include "analysis/csrc/dfx/error_code.h"

using namespace testing;
using namespace Analysis::Infra;
using namespace Analysis::Utils;

namespace Analysis {
namespace Domain {
namespace {
    const std::string DEVICE_PATH = "./device_0";
}
class FreqPersistenceUTest : public Test {
protected:
    void SetUp()
    {
        EXPECT_TRUE(File::CreateDir(DEVICE_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({DEVICE_PATH, "sqlite"})));
        auto data = std::make_shared<std::vector<HalFreqLpmData>>();
        dataInventory_.Inject(data);
    }

    void TearDown()
    {
        dataInventory_.RemoveRestData({});
        EXPECT_TRUE(File::RemoveDir(DEVICE_PATH, 0));
    }

protected:
    DataInventory dataInventory_;
};

static std::vector<HalFreqLpmData> CreateFreqData(uint64_t sysCnt, uint32_t freq)
{
    std::vector<HalFreqLpmData> res;
    HalFreqLpmData freqData{};
    freqData.sysCnt = sysCnt;
    freqData.freq = freq;
    res.emplace_back(freqData);
    res.emplace_back(freqData);
    return res;
}

TEST_F(FreqPersistenceUTest, ShouldSaveFreqDataSuccess)
{
    FreqPersistence persistence;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    auto freqDataS = dataInventory_.GetPtr<std::vector<HalFreqLpmData>>();
    auto freqData = CreateFreqData(1000, 1800);
    freqDataS->swap(freqData);
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context));
}
}
}