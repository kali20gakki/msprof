/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : log_modeling_utest.cpp
 * Description        : LogModeling UT
 * Author             : msprof team
 * Creation Date      : 2024/4/29
 * *****************************************************************************
 */
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/infrastructure/context/include/device_context.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/domain/services/modeling/include/log_modeling.h"
#include "analysis/csrc/domain/services/parser/log/include/stars_soc_parser.h"
#include "analysis/csrc/domain/entities/hal/include/hal_log.h"
#include "analysis/csrc/domain/entities/hal/include/hal_track.h"
#include "analysis/csrc/domain/entities/hal/include/device_task.h"
#include "analysis/csrc/dfx/error_code.h"

using namespace testing;

namespace Analysis {

using namespace Infra;

namespace Domain {

class LogModelingUTest : public Test {
protected:
    void SetUp() override
    {
        auto data = std::make_shared<std::vector<HalLogData>>();
        dataInventory_.Inject(data);
        auto deviceData = std::make_shared<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
        dataInventory_.Inject(deviceData);
        auto flip = std::make_shared<std::vector<HalTrackData>>(std::vector<HalTrackData>{});
        dataInventory_.Inject(flip);
    }
    void TearDown() override
    {
        dataInventory_.RemoveRestData({});
    }

protected:
    Infra::DataInventory dataInventory_;
};


static std::vector<HalLogData> GetRepeatedDeviceTaskData()
{
    std::vector<HalLogData> logData;
    constexpr size_t SEQ_NUM = 16;
    static const uint16_t TASK_ID_SEQ[SEQ_NUM] = {1, 1, 2, 2, 65534, 65534, 1, 1, 1, 1, 2, 2, 65534, 65534, 1, 1};
    static const uint64_t TIMESTAMP_SEQ[SEQ_NUM] = {100, 200, 200, 300, 300, 380, 400, 500,
                                                    1100, 1200, 1200, 1300, 1300, 1380, 1400, 1500};

    HalLogData logIns{};
    logIns.acsq.isEndTimestamp = true;
    logIns.hd.taskId.streamId = 1;
    logIns.hd.taskId.batchId = 0;
    logIns.hd.taskId.contextId = 1;
    logIns.type = ACSQ_LOG;
    logIns.acsq.taskType = 3; // 任务测试类型为3
    for (size_t i = 0; i < SEQ_NUM; ++i) {
        logIns.hd.taskId.taskId = TASK_ID_SEQ[i];
        logIns.hd.timestamp = TIMESTAMP_SEQ[i];
        logIns.acsq.isEndTimestamp = !logIns.acsq.isEndTimestamp;
        logIns.acsq.timestamp = TIMESTAMP_SEQ[i];
        logData.push_back(logIns);
    }

    return logData;
}

static void CheckRepeatedDeviceTaskData(Infra::DataInventory& dataInventory)
{
    auto deviceData = dataInventory.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    ASSERT_TRUE(deviceData);
    EXPECT_EQ(deviceData->size(), 7ul);
    
    const auto& device1011 = (*deviceData)[{1, 0, 1, 1}];
    ASSERT_EQ(device1011.size(), 1ul);
    EXPECT_EQ(device1011[0].taskType, 3); // 任务测试类型为3
    EXPECT_EQ(device1011[0].taskStart, 100);  // 任务开始测试时间100
    EXPECT_EQ(device1011[0].taskEnd, 200); // 任务结束测试时间200
    const auto& device1021 = (*deviceData)[{1, 0, 2, 1}];
    ASSERT_EQ(device1021.size(), 1ul);
    EXPECT_EQ(device1021[0].taskType, 3); // 任务测试类型为3
    EXPECT_EQ(device1021[0].taskStart, 200); // 任务开始测试时间200
    EXPECT_EQ(device1021[0].taskEnd, 300); // 任务结束测试时间300
    const auto& device10655341 = (*deviceData)[{1, 0, 65534, 1}];
    ASSERT_EQ(device10655341.size(), 1ul);
    EXPECT_EQ(device10655341[0].taskType, 3); // 任务测试类型为3
    EXPECT_EQ(device10655341[0].taskStart, 300); // 任务开始测试时间300
    EXPECT_EQ(device10655341[0].taskEnd, 380); // 任务结束测试时间380

    const auto& device1111 = (*deviceData)[{1, 1, 1, 1}];
    ASSERT_EQ(device1111.size(), 2ul);
    EXPECT_EQ(device1111[0].taskType, 3); // 任务测试类型为3
    EXPECT_EQ(device1111[0].taskStart, 400); // 任务开始测试时间400
    EXPECT_EQ(device1111[0].taskEnd, 500); // 任务结束测试时间500
    EXPECT_EQ(device1111[1].taskType, 3); // 任务测试类型为3
    EXPECT_EQ(device1111[1].taskStart, 1100); // 任务开始测试时间1100
    EXPECT_EQ(device1111[1].taskEnd, 1200); // 任务结束测试时间1200

    const auto& device1121 = (*deviceData)[{1, 1, 2, 1}];
    ASSERT_EQ(device1121.size(), 1ul);
    EXPECT_EQ(device1121[0].taskType, 3); // 任务测试类型为3
    EXPECT_EQ(device1121[0].taskStart, 1200); // 任务开始测试时间1200
    EXPECT_EQ(device1121[0].taskEnd, 1300); // 任务结束测试时间1300
    const auto& device11655341 = (*deviceData)[{1, 1, 65534, 1}];
    ASSERT_EQ(device11655341.size(), 1ul);
    EXPECT_EQ(device11655341[0].taskType, 3); // 任务测试类型为3
    EXPECT_EQ(device11655341[0].taskStart, 1300); // 任务开始测试时间1300
    EXPECT_EQ(device11655341[0].taskEnd, 1380); // 任务结束测试时间1380

    const auto& device1211 = (*deviceData)[{1, 2, 1, 1}];
    ASSERT_EQ(device1211.size(), 1ul);
    EXPECT_EQ(device1211[0].taskType, 3); // 任务测试类型为3
    EXPECT_EQ(device1211[0].taskStart, 1400); // 任务开始测试时间1400
    EXPECT_EQ(device1211[0].taskEnd, 1500); // 任务结束测试时间1500
}

TEST_F(LogModelingUTest, ShouldGetRepeatedDeviceTaskData)
{
    std::vector<HalLogData> logData = GetRepeatedDeviceTaskData();
    auto logDataS = dataInventory_.GetPtr<std::vector<HalLogData>>();
    logDataS->swap(logData);

    auto flipS = dataInventory_.GetPtr<std::vector<HalTrackData>>();
    HalTrackData oneTrackData{};
    oneTrackData.hd.taskId.streamId = 1;
    oneTrackData.hd.taskId.contextId = 0;
    oneTrackData.hd.timestamp = 390; // 测试时间戳为390
    oneTrackData.type = TS_TASK_FLIP;
    oneTrackData.flip.flipNum = 1;
    oneTrackData.flip.timestamp = 390; // 测试时间戳为390
    flipS->push_back(oneTrackData);
    oneTrackData.hd.timestamp = 1390; // 测试时间戳为1390
    oneTrackData.flip.timestamp = 1390; // 测试时间戳为1390
    flipS->push_back(oneTrackData);

    // when
    Domain::LogModeling modeling;
    DeviceContext context;
    ASSERT_EQ(modeling.Run(dataInventory_, context), Analysis::ANALYSIS_OK);

    CheckRepeatedDeviceTaskData(dataInventory_);
}

static std::vector<HalLogData> GetRightDeviceTaskData()
{
    std::vector<HalLogData> logData{};
    HalLogData logIns{};
    constexpr size_t SEQ_NUM = 8;
    static const uint16_t STREAM_ID_SEQ[SEQ_NUM] = {1, 1, 2, 2, 1, 1, 1, 1};
    static const uint16_t BATCH_ID_SEQ[SEQ_NUM] = {1, 1, 1, 1, 1, 1, 2, 2};
    static const uint32_t CONTEXT_ID_SEQ[SEQ_NUM] = {2, 1, 1, 1, 2, 1, 1, 1};
    static const uint64_t TIMESTAMP_SEQ[SEQ_NUM] = {100, 100, 100, 150, 200, 300, 300, 500};
    static const HalLogType TYPE_SEQ[SEQ_NUM] = {ACSQ_LOG, FFTS_LOG, ACSQ_LOG, ACSQ_LOG,
                                                 ACSQ_LOG, FFTS_LOG, ACSQ_LOG, ACSQ_LOG};
    static const bool END_FLAG_SEQ[SEQ_NUM] = {false, false, false, true, true, true, false, true};
    for (size_t i = 0; i < SEQ_NUM; ++i) {
        logIns.hd.taskId.streamId = STREAM_ID_SEQ[i];
        logIns.hd.taskId.batchId = BATCH_ID_SEQ[i];
        logIns.hd.taskId.contextId = CONTEXT_ID_SEQ[i];
        logIns.hd.timestamp = TIMESTAMP_SEQ[i];
        logIns.type = TYPE_SEQ[i];
        logIns.acsq.isEndTimestamp = END_FLAG_SEQ[i];
        logIns.acsq.timestamp = TIMESTAMP_SEQ[i];
        if (logIns.type == ACSQ_LOG) {
            logIns.acsq.taskType = 3; // 任务测试类型为3
        } else {
            logIns.ffts.subTaskType = 10; // 子任务测试类型为10
            logIns.ffts.fftsType = 4; // 任务测试类型为4
            logIns.ffts.threadId = 1;
        }
        logData.push_back(logIns);
    }

    return logData;
}

TEST_F(LogModelingUTest, ShouldGetRightDeviceTaskData)
{
    std::vector<HalLogData> logData = GetRightDeviceTaskData();
    auto logDataS = dataInventory_.GetPtr<std::vector<HalLogData>>();
    logDataS->swap(logData);
    Domain::LogModeling modeling;
    DeviceContext context;
    ASSERT_EQ(modeling.Run(dataInventory_, context), Analysis::ANALYSIS_OK);

    auto deviceData = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    ASSERT_TRUE(deviceData);
    ASSERT_EQ(deviceData->size(), 4ul);
    const auto& task1011 = (*deviceData)[{1, 1, 0, 1}][0];
    EXPECT_EQ(task1011.taskType, 10); // 任务测试类型为10
    EXPECT_EQ(task1011.taskStart, 100); // 任务开始测试时间100
    EXPECT_EQ(task1011.taskEnd, 300); // 任务结束测试时间300

    const auto& task1012 = (*deviceData)[{1, 1, 0, 2}][0];
    EXPECT_EQ(task1012.taskType, 3); // 任务测试类型为3
    EXPECT_EQ(task1012.taskStart, 100); // 任务开始测试时间100
    EXPECT_EQ(task1012.taskEnd, 200); // 任务结束测试时间200

    const auto& task2011 = (*deviceData)[{2, 1, 0, 1}][0];
    EXPECT_EQ(task2011.taskType, 3); // 任务测试类型为3
    EXPECT_EQ(task2011.taskStart, 100); // 任务开始测试时间100
    EXPECT_EQ(task2011.taskEnd, 150); // 任务结束测试时间150

    const auto& task1021 = (*deviceData)[{1, 2, 0, 1}][0];
    EXPECT_EQ(task1021.taskType, 3); // 任务测试类型为3
    EXPECT_EQ(task1021.taskStart, 300); // 任务开始测试时间300
    EXPECT_EQ(task1021.taskEnd, 500); // 任务结束测试时间500

    EXPECT_EQ(deviceData->size(), 4ul); // 保证使用map[]方法时，没有新增ID，保持为4
}

static std::vector<HalLogData> GetRightDeviceTaskDataWhenInputFlippedTasks()
{
    constexpr size_t SEQ_NUM = 26;
    static const uint16_t STREAM_ID_SEQ[SEQ_NUM] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3};
    static const uint16_t TASK_ID_SEQ[SEQ_NUM] = {1, 1, 1, 1, 2, 2, 65534, 65534, 1, 1,
        1, 1, 1, 1, 65534, 65534, 2, 2, 1, 1, 1, 1, 3, 3, 2, 2};
    static const uint32_t CONTEXT_ID_SEQ[SEQ_NUM] = {1, 2, 1, 2, 1, 1, 1, 1, 1, 1,
        1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    static const uint64_t TIMESTAMP_SEQ[SEQ_NUM] = {100, 100, 200, 200, 200, 300, 300, 380, 400, 500,
        100, 100, 150, 200, 200, 280, 400, 500, 300, 400, 100, 200, 300, 400, 200, 300};
    static const HalLogType TYPE_SEQ[SEQ_NUM] = {
        ACSQ_LOG, FFTS_LOG, ACSQ_LOG, FFTS_LOG, ACSQ_LOG, ACSQ_LOG, ACSQ_LOG, ACSQ_LOG, ACSQ_LOG, ACSQ_LOG,
        ACSQ_LOG, FFTS_LOG, FFTS_LOG, ACSQ_LOG, ACSQ_LOG, ACSQ_LOG, ACSQ_LOG, ACSQ_LOG, ACSQ_LOG, ACSQ_LOG,
        ACSQ_LOG, ACSQ_LOG, ACSQ_LOG, ACSQ_LOG, FFTS_LOG, FFTS_LOG};
    static const bool END_FLAG_SEQ[SEQ_NUM] = {
        false, false, true, true, false, true, false, true, false, true,
        false, false, true, true, false, true, false, true, false, true,
        false, true, false, true, false, true
    };
    std::vector<HalLogData> logData;
    HalLogData logIns{};
    for (size_t i = 0; i < SEQ_NUM; ++i) {
        logIns.hd.taskId.streamId = STREAM_ID_SEQ[i];
        logIns.hd.taskId.taskId = TASK_ID_SEQ[i];
        logIns.hd.taskId.contextId = CONTEXT_ID_SEQ[i];
        logIns.hd.timestamp = TIMESTAMP_SEQ[i];
        logIns.type = TYPE_SEQ[i];
        logIns.acsq.isEndTimestamp = END_FLAG_SEQ[i];
        logIns.acsq.timestamp = TIMESTAMP_SEQ[i];
        if (logIns.type == ACSQ_LOG) {
            logIns.acsq.taskType = 3; // 任务测试类型为3
        } else {
            logIns.ffts.subTaskType = 10; // 子任务测试类型为10
            logIns.ffts.fftsType = 4; // 任务测试类型为4
            logIns.ffts.threadId = 1;
        }
        logData.push_back(logIns);
    }

    return logData;
}

static void CheckFilppedTasks(Infra::DataInventory& dataInventory)
{
    auto deviceData = dataInventory.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    ASSERT_TRUE(deviceData);
    EXPECT_EQ(deviceData->size(), 13ul);
    // 由于数据量太大，下面仅校验几个典型的值
    const auto& beforeFlipStream_1 = (*deviceData)[{1, 0, 65534, 1}][0];
    EXPECT_EQ(beforeFlipStream_1.taskType, 3); // 任务测试类型为3
    EXPECT_EQ(beforeFlipStream_1.taskStart, 300); // 任务开始测试时间300
    EXPECT_EQ(beforeFlipStream_1.taskEnd, 380); // 任务结束测试时间380
    const auto& afterFlipStream_1 = (*deviceData)[{1, 1, 1, 1}][0];
    EXPECT_EQ(afterFlipStream_1.taskType, 3); // 任务测试类型为3
    EXPECT_EQ(afterFlipStream_1.taskStart, 400); // 任务开始测试时间400
    EXPECT_EQ(afterFlipStream_1.taskEnd, 500); // 任务结束测试时间500
    const auto& task2011 = (*deviceData)[{2, 0, 1, 1}][0];
    EXPECT_EQ(task2011.taskType, 3); // 任务测试类型为3
    EXPECT_EQ(task2011.taskStart, 100); // 任务开始测试时间100
    EXPECT_EQ(task2011.taskEnd, 200); // 任务结束测试时间200
    const auto& task2012 = (*deviceData)[{2, 0, 1, 2}][0];
    EXPECT_EQ(task2012.taskType, 10); // 任务测试类型为10
    EXPECT_EQ(task2012.taskStart, 100); // 任务开始测试时间100
    EXPECT_EQ(task2012.taskEnd, 150); // 任务结束测试时间150
    const auto& beforeFlipStream_2 = (*deviceData)[{2, 0, 65534, 1}][0];
    EXPECT_EQ(beforeFlipStream_2.taskType, 3); // 任务测试类型为3
    EXPECT_EQ(beforeFlipStream_2.taskStart, 200); // 任务开始测试时间200
    EXPECT_EQ(beforeFlipStream_2.taskEnd, 280); // 任务结束测试时间280
    const auto& afterFlipStream_2 = (*deviceData)[{2, 1, 1, 1}][0];
    EXPECT_EQ(afterFlipStream_2.taskType, 3); // 任务测试类型为3
    EXPECT_EQ(afterFlipStream_2.taskStart, 300); // 任务开始测试时间300
    EXPECT_EQ(afterFlipStream_2.taskEnd, 400); // 任务结束测试时间400
    const auto& task3031 = (*deviceData)[{3, 0, 3, 1}][0];
    EXPECT_EQ(task3031.taskType, 3); // 任务测试类型为3
    EXPECT_EQ(task3031.taskStart, 300); // 任务开始测试时间300
    EXPECT_EQ(task3031.taskEnd, 400); // 任务结束测试时间400
    const auto& task3021 = (*deviceData)[{3, 0, 2, 1}][0];
    EXPECT_EQ(task3021.taskType, 10); // 任务测试类型为10
    EXPECT_EQ(task3021.taskStart, 200); // 任务开始测试时间200
    EXPECT_EQ(task3021.taskEnd, 300); // 任务结束测试时间300
    EXPECT_EQ(deviceData->size(), 13ul); // 保证使用map[]方法时，没有新增ID，保持为13
}

TEST_F(LogModelingUTest, ShouldGetRightDeviceTaskDataWhenInputFlippedTasks)
{
    std::vector<HalLogData> logData = GetRightDeviceTaskDataWhenInputFlippedTasks();
    auto logDataS = dataInventory_.GetPtr<std::vector<HalLogData>>();
    logDataS->swap(logData);

    auto flipS = dataInventory_.GetPtr<std::vector<HalTrackData>>();
    HalTrackData oneTrackData{};
    oneTrackData.hd.taskId.streamId = 1;
    oneTrackData.hd.taskId.contextId = 0;
    oneTrackData.hd.timestamp = 390; // 测试时间戳为390
    oneTrackData.type = TS_TASK_FLIP;
    oneTrackData.flip.flipNum = 1;
    oneTrackData.flip.timestamp = 390; // 测试时间戳为390
    flipS->push_back(oneTrackData);
    oneTrackData.hd.taskId.streamId = 2;  // 新一个streamId为2
    oneTrackData.hd.timestamp = 290; // 测试时间戳为290
    oneTrackData.flip.timestamp = 290; // 测试时间戳为290
    flipS->push_back(oneTrackData);

    // when
    Domain::LogModeling modeling;
    DeviceContext context;
    ASSERT_EQ(modeling.Run(dataInventory_, context), Analysis::ANALYSIS_OK);

    CheckFilppedTasks(dataInventory_);
}

}

}