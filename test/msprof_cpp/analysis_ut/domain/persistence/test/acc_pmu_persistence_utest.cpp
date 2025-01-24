/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : acc_pmu_persistence_utest.cpp
 * Description        : acc pmu数据持久化 UT
 * Author             : msprof team
 * Creation Date      : 2024/5/17
 * *****************************************************************************
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "analysis/csrc/domain/services/persistence/device/acc_pmu_persistence.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/entities/hal/include/hal_log.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

using namespace testing;
using namespace Analysis::Infra;
using namespace Analysis::Utils;
using namespace Analysis::Domain;

namespace Analysis {
namespace Domain {
namespace {
const std::string DEVICE_PATH = "./device_0";
const int ACC_PMU_SIZE = 2;
}
class AccPmuPersistenceUTest : public Test {
protected:
    void SetUp()
    {
        EXPECT_TRUE(File::CreateDir(DEVICE_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({DEVICE_PATH, "sqlite"})));
        auto data = std::make_shared<std::vector<HalLogData>>();
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

static std::vector<HalLogData> CreateAccPmuData(int accId, int timestamp, int ost)
{
    std::vector<HalLogData> res;
    HalLogData logData{};
    logData.accPmu.accId = accId;
    logData.accPmu.timestamp = timestamp;
    for (int i = 0; i < ACC_PMU_SIZE; ++i) {
        logData.accPmu.bandwidth[i] = i * ost;
        logData.accPmu.ost[i] = i * ost;
    }
    res.push_back(logData);
    return res;
}

TEST_F(AccPmuPersistenceUTest, ShouldSaveAccPmuDataSuccess)
{
    AccPmuPersistence persistence;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    HostStartLog hostStartLog;
    hostStartLog.cntVctDiff = 1;
    context.deviceContextInfo.hostStartLog = hostStartLog;
    auto accPmuS = dataInventory_.GetPtr<std::vector<HalLogData>>();
    auto accPmu = CreateAccPmuData(1, 10000, 5000);
    accPmuS->swap(accPmu);
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context));
}

TEST_F(AccPmuPersistenceUTest, TestRunShouldReturnErrorWhenDataIsNull)
{
    dataInventory_.RemoveRestData({});
    AccPmuPersistence persistence;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    ASSERT_EQ(ANALYSIS_ERROR, persistence.Run(dataInventory_, context));
}
}
}