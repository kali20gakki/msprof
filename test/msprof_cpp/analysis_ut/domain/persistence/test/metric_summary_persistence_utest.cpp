/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : ascend_task_persistence_utest.cpp
 * Description        : PmuAssociation UT
 * Author             : msprof team
 * Creation Date      : 2024/5/17
 * *****************************************************************************
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "analysis/csrc/domain/services/persistence/metric_summary_persistence.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/entities/hal/include/device_task.h"
#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

using namespace testing;
using namespace Analysis::Infra;
using namespace Analysis::Utils;

namespace Analysis {
namespace Domain {
namespace {
    const std::string DEVICE_PATH = "./device_0";
}
class MetricSummaryPersistenceUTest : public Test {
protected:
    void SetUp()
    {
        EXPECT_TRUE(File::CreateDir(DEVICE_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({DEVICE_PATH, "sqlite"})));
        auto data = std::make_shared<std::map<TaskId, std::vector<DeviceTask>>>();
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

static std::map<TaskId, std::vector<DeviceTask>> GenerateDeviceTask()
{
    std::map<TaskId, std::vector<DeviceTask>> res;
    auto& res1 = res[{1, 1, 1, 1}];
    res1.emplace_back();
    res1.back().acceleratorType = MIX_AIC;
    PmuInfoMixAccelerator mixAccelerator;
    mixAccelerator.aiCoreTime = 100.0;  // aic time 100.0
    mixAccelerator.aivTime = 200.0;  // aic time 200.0
    mixAccelerator.aicTotalCycles = 100;  // aic totalCycle 100
    mixAccelerator.aivTotalCycles = 200;  // aiv totalCycle 200
    mixAccelerator.aicPmuResult = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0};  // aic pmu result 11个
    mixAccelerator.aivPmuResult = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0};  // aic 13个
    res1.back().pmuInfo = MAKE_UNIQUE_PTR<PmuInfoMixAccelerator>(mixAccelerator);
    res1.emplace_back();
    res1.back().acceleratorType = AIC;
    PmuInfoSingleAccelerator singleAccelerator1;
    singleAccelerator1.totalTime = 100.0;  // aic time 100.0
    singleAccelerator1.totalCycles = 100;  // aic totalCycle 100
    singleAccelerator1.pmuResult = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0};  // aic pmu result 11个
    res1.back().pmuInfo = MAKE_UNIQUE_PTR<PmuInfoSingleAccelerator>(singleAccelerator1);
    res1.emplace_back();
    res1.back().acceleratorType = AIV;
    PmuInfoSingleAccelerator singleAccelerator2;
    singleAccelerator2.totalTime = 100.0;  // aiv time 100.0
    singleAccelerator2.totalCycles = 100;  // aiv totalCycle 100
    singleAccelerator2.pmuResult = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0};  // aiv 13个
    res1.back().pmuInfo = MAKE_UNIQUE_PTR<PmuInfoSingleAccelerator>(singleAccelerator2);
    return res;
}

TEST_F(MetricSummaryPersistenceUTest, ShouldSavePmuDataSuccess)
{
    MetricSummaryPersistence persistence;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    context.deviceContextInfo.deviceInfo.chipID = CHIP_V4_1_0;
    context.deviceContextInfo.sampleInfo.aiCoreMetrics = AicMetricsEventsType::AIC_PIPE_UTILIZATION_EXCT;
    context.deviceContextInfo.sampleInfo.aivMetrics = AivMetricsEventsType::AIV_PIPE_UTILIZATION;
    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<DeviceTask>>>();
    auto deviceTask = GenerateDeviceTask();
    deviceTaskS->swap(deviceTask);
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context));
}

TEST_F(MetricSummaryPersistenceUTest, ShouldReturnNoPMUWhenNoMetric)
{
    MetricSummaryPersistence persistence;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    context.deviceContextInfo.deviceInfo.chipID = CHIP_V4_1_0;
    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<DeviceTask>>>();
    auto deviceTask = GenerateDeviceTask();
    deviceTaskS->swap(deviceTask);
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context));
}

TEST_F(MetricSummaryPersistenceUTest, TestRunShouldReturnErrorWhenDataIsNull)
{
    dataInventory_.RemoveRestData({});
    MetricSummaryPersistence persistence;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    context.deviceContextInfo.deviceInfo.chipID = CHIP_V4_1_0;
    ASSERT_EQ(ANALYSIS_ERROR, persistence.Run(dataInventory_, context));
}
}
}