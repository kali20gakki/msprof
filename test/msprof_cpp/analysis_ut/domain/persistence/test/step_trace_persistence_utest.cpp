/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : step_trace_persistence_utest.cpp
 * Description        : step_trace.db相关表落盘 ut用例
 * Author             : msprof team
 * Creation Date      : 2024/6/3
 * *****************************************************************************
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/modeling/step_trace/include/step_trace_process.h"
#include "analysis/csrc/domain/services/persistence/device/step_trace_persistence.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"


namespace Analysis {
    using namespace Analysis;
    using namespace Analysis::Infra;
    using namespace Analysis::Domain;
    using namespace Analysis::Utils;
    namespace {
        const std::string DEVICE_PATH = "./device_0";
    }
    class StepTracePersistenceUtest : public testing::Test {
    protected:
        void SetUp() override
        {
            EXPECT_TRUE(File::CreateDir(DEVICE_PATH));
            EXPECT_TRUE(File::CreateDir(File::PathJoin({DEVICE_PATH, "sqlite"})));
        }
        void TearDown() override
        {
            dataInventory_.RemoveRestData({});
            EXPECT_TRUE(File::RemoveDir(DEVICE_PATH, 0));
        }
        HalTrackData CreateHalTrackData(uint16_t moduleId, uint16_t tagId, uint64_t timestamp, uint16_t streamId = 2)
        {
            HalTrackData log;
            log.type = STEP_TRACE;
            log.stepTrace.modelId = moduleId;
            log.stepTrace.tagId = tagId;
            log.stepTrace.timestamp = timestamp;
            log.hd.taskId.streamId = streamId;
            log.stepTrace.indexId = moduleId;
            return log;
        }

    protected:
        DataInventory dataInventory_;
    };


//  modelId     tagId   timestamp   streamId
//  1           0       10          2
//  1           2       18          2
//  1           3       19          2
//  1           4       21          2
//  1           1       25          2
//  2           0       30          2
//  2           20000   33          2
//  2           20001   36          2
//  2           2       40          2
//  2           20002   42          2
//  2           20003   44          2
//  2           10000   46          6
//  2           20005   47          2
//  2           10001   48          6
//  2           3       55          2
//  2           10002   56          8
//  2           10003   58          8
//  2           10004   59          6   异常数据，无结束时间
//  2           4       60          2
//  2           1       65          2
//  2           0       66          2
//  2           60000   67          2
//  2           60001   68          2
//  2           4       69          2
//  2           0       70          2
//  2           0       77          2   异常数据，重复的model start
//  2           1       80          2
TEST_F(StepTracePersistenceUtest, ShouldSaveStepTraceDBSuccess)
{
    DataInventory dataInventory_;
    auto data = std::make_shared<std::vector<HalTrackData>>();
    data->emplace_back(CreateHalTrackData(2, 20000, 33)); // 2, 20000, 33
    data->emplace_back(CreateHalTrackData(2, 20001, 36)); // 2, 20001, 36
    data->emplace_back(CreateHalTrackData(1, 0, 10)); // 1, 0, 10
    data->emplace_back(CreateHalTrackData(1, 2, 18)); // 1, 2, 18
    data->emplace_back(CreateHalTrackData(2, 20003, 44)); // 2, 20003, 44
    data->emplace_back(CreateHalTrackData(2, 10000, 46, 6)); // 2, 10000, 46, 6
    data->emplace_back(CreateHalTrackData(2, 10001, 48, 6)); // 2, 10001, 48, 6
    data->emplace_back(CreateHalTrackData(2, 10004, 59, 6)); // 2, 10004, 59, 6
    data->emplace_back(CreateHalTrackData(1, 3, 19)); // 1, 3, 19
    data->emplace_back(CreateHalTrackData(1, 4, 21)); // 1, 4, 21
    data->emplace_back(CreateHalTrackData(1, 1, 25)); // 1, 1, 25
    data->emplace_back(CreateHalTrackData(2, 1, 65)); // 2, 1, 65
    data->emplace_back(CreateHalTrackData(2, 0, 30)); // 2, 0, 30
    data->emplace_back(CreateHalTrackData(2, 2, 40)); // 2, 2, 40
    data->emplace_back(CreateHalTrackData(2, 20002, 42)); // 2, 20002, 42
    data->emplace_back(CreateHalTrackData(2, 20005, 47)); // 2, 20005, 47
    data->emplace_back(CreateHalTrackData(2, 3, 55)); // 2, 3, 55
    data->emplace_back(CreateHalTrackData(2, 10002, 56, 8)); // 2, 10002, 56, 8
    data->emplace_back(CreateHalTrackData(2, 10003, 58, 8)); // 2, 10003, 58, 8
    data->emplace_back(CreateHalTrackData(2, 4, 60)); // 2, 4, 60
    data->emplace_back(CreateHalTrackData(2, 0, 66)); // 2, 0, 66
    data->emplace_back(CreateHalTrackData(2, 4, 69)); // 2, 4, 69
    data->emplace_back(CreateHalTrackData(2, 0, 70)); // 2, 0, 70
    data->emplace_back(CreateHalTrackData(2, 0, 77)); // 2, 0, 77
    data->emplace_back(CreateHalTrackData(2, 1, 80)); // 2, 1, 80
    data->emplace_back(CreateHalTrackData(2, 20005, 47)); // 2, 20005, 47
    data->emplace_back(CreateHalTrackData(2, 60000, 67)); // 2, 60000, 67
    data->emplace_back(CreateHalTrackData(2, 60001, 68)); // 2, 60001, 68
    dataInventory_.Inject(data);
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;

    auto process = StepTraceProcess();
    ASSERT_EQ(ANALYSIS_OK, process.Run(dataInventory_, context));

    auto stepTraceProcess = StepTracePersistence();
    ASSERT_EQ(ANALYSIS_OK, stepTraceProcess.Run(dataInventory_, context));
}

TEST_F(StepTracePersistenceUtest, ShouldSaveStepTraceDBSuccessWhenDataIsAged)
{
    DataInventory dataInventory_;
    auto data = std::make_shared<std::vector<HalTrackData>>();
    data->emplace_back(CreateHalTrackData(0, 3, 3)); // 0, 3, 3
    data->emplace_back(CreateHalTrackData(1, 0, 10)); // 1, 0, 10
    data->emplace_back(CreateHalTrackData(1, 3, 19)); // 1, 3, 19
    data->emplace_back(CreateHalTrackData(2, 0, 30)); // 2, 0, 30
    data->emplace_back(CreateHalTrackData(2, 20001, 36)); // 2, 20001, 36
    data->emplace_back(CreateHalTrackData(2, 20002, 37, 12)); // 2, 20002, 37, 12
    data->emplace_back(CreateHalTrackData(2, 20003, 44)); // 2, 20003, 44
    data->emplace_back(CreateHalTrackData(2, 10001, 48, 6)); // 2, 10001, 48, 6
    data->emplace_back(CreateHalTrackData(2, 10002, 49, 9)); // 2, 10002, 49, 9
    data->emplace_back(CreateHalTrackData(2, 10003, 58, 10)); // 2, 10003, 58, 10
    data->emplace_back(CreateHalTrackData(2, 60001, 68)); // 2, 60001, 68
    dataInventory_.Inject(data);
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;

    auto process = StepTraceProcess();
    ASSERT_EQ(ANALYSIS_OK, process.Run(dataInventory_, context));

    auto stepTraceProcess = StepTracePersistence();
    ASSERT_EQ(ANALYSIS_OK, stepTraceProcess.Run(dataInventory_, context));
}

TEST_F(StepTracePersistenceUtest, ShouldRunSuccessWhenStepDataIsEmpty)
{
    DataInventory dataInventory_;
    auto data = std::make_shared<std::vector<HalTrackData>>();
    dataInventory_.Inject(data);
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;

    auto process = StepTraceProcess();
    ASSERT_EQ(ANALYSIS_OK, process.Run(dataInventory_, context));

    auto stepTraceProcess = StepTracePersistence();
    ASSERT_EQ(ANALYSIS_OK, stepTraceProcess.Run(dataInventory_, context));
}

}
