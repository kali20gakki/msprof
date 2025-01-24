/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hccl_calculator_utest.cpp
 * Description        : hccl_calculator UT
 * Author             : msprof team
 * Creation Date      : 2024/5/28
 * *****************************************************************************
 */
#include <gtest/gtest.h>
#include "analysis/csrc/domain/services/association/calculator/hccl/include/hccl_calculator.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/domain/entities/hal/include/top_down_task.h"
#include "analysis/csrc/infrastructure/context/include/device_context.h"
#include "analysis/csrc/infrastructure/utils/utils.h"
#include "mockcpp/mockcpp.hpp"

using namespace Analysis::Domain;
using namespace Analysis::Utils;

// deviceId, modelId, indexId, threadId, opName, taskType, opType, timestamp, duration, isDynamic, connectionId,
// relay, retry, dataType, algType, count, groupName
using HcclOpDataFormat = std::vector<std::tuple<uint16_t, uint64_t, int32_t, uint32_t, std::string, std::string,
        std::string, uint64_t, uint64_t, std::string, int64_t, int32_t, int32_t, std::string, std::string,
        int32_t, std::string>>;
const HcclOpDataFormat HCCL_OP_DATA = {
    {1, 4294967295, -1, 2037145, "hcom_broadcast_", "HCCL", "hcom_broadcast_", 8864918572019, 63965, "1",
        17, 0, 0, "INT64", "MESH-RING", 5, "559228325453745108"},
    {1, 4294967295, -1, 2037145, "hcom_allReduce_", "HCCL", "hcom_allReduce_", 8865013596227, 31531, "1",
        9537, 0, 0, "FP32", "RING-HD", 1, "18121985196749930015"},
};

// modelId, indexId, name, groupName, planeId, timestamp, duration, streamId, taskId, contextId, batchId,
// deviceId, isMaster, localRank, remoteRank, transportType, size, dataType, linkType, notifyId, rdmaType
using HcclTaskDataFormat = std::vector<std::tuple<uint64_t, int32_t, std::string, std::string, int32_t, uint64_t,
        double, uint32_t, uint16_t, uint32_t, uint16_t, uint16_t, uint16_t, uint32_t, uint32_t, std::string,
        double, std::string, std::string, std::string, std::string, uint32_t>>;
const HcclTaskDataFormat HCCL_TASK_DATA = {
    {4294967295, -1, "Notify_Record", "559228325453745108", 0, 8864918631924, 1, 5, 5450, 0, 0, 1, 0, 1, 0,
        "SDMA", 0, "INVALID_TYPE", "INVALID_TYPE", "504", "INVALID_TYPE", 2037145},
    {4294967295, -1, "Notify_Record", "559228325453745108", 0, 8864918631924, 1, 5, 5450, 0, 0, 1, 1, 1, 0,
        "SDMA", 0, "INVALID_TYPE", "INVALID_TYPE", "504", "INVALID_TYPE", 2037145},
    {4294967295, -1, "Notify_Wait", "559228325453745108", 0, 8864918631924, 0.02, 5, 5450, 1, 0, 1, 1, 1, 0,
        "LOCAL", 0, "INVALID_TYPE", "INVALID_TYPE", "4294967748", "INVALID_TYPE", 2037145},
    {4294967295, -1, "Memcpy", "559228325453745108", 0, 8864918631924, 0.6, 5, 5450, 2, 0, 1, 1, 1, 0,
        "SDMA", 0, "INVALID_TYPE", "HCCS", "18446744073709551615", "INVALID_TYPE", 2037145},
    {4294967295, -1, "Notify_Record", "559228325453745108", 0, 8864918631924, 1, 5, 5450, 3, 0, 1, 1, 1, 0,
        "SDMA", 0, "INVALID_TYPE", "INVALID_TYPE", "504", "INVALID_TYPE", 2037145},
    {4294967295, -1, "Notify_Wait", "559228325453745108", 0, 8864918631924, 0.02, 5, 5450, 4, 0, 1, 1, 1, 0,
        "LOCAL", 0, "INVALID_TYPE", "INVALID_TYPE", "4294967756", "INVALID_TYPE", 2037145},
    {4294967295, -1, "Notify_Record", "559228325453745108", 0, 8864918631924, 1, 5, 5450, 5, 0, 1, 1, 1, 7,
        "SDMA", 0, "INVALID_TYPE", "INVALID_TYPE", "30064771576", "INVALID_TYPE", 2037145},
    {4294967295, -1, "Notify_Wait", "559228325453745108", 0, 8864918631924, 0.02, 5, 5450, 6, 0, 1, 1, 1, 7,
        "LOCAL", 0, "INVALID_TYPE", "INVALID_TYPE", "4294967800", "INVALID_TYPE", 2037145},
    {4294967295, -1, "Notify_Record", "559228325453745108", 0, 8864918631924, 1, 5, 5450, 7, 0, 1, 1, 1, 6,
        "SDMA", 0, "INVALID_TYPE", "INVALID_TYPE", "25769804280", "INVALID_TYPE", 2037145},
    {4294967295, -1, "Notify_Wait", "559228325453745108", 0, 8864918631924, 0.02, 5, 5450, 8, 0, 1, 1, 1, 6,
        "LOCAL", 0, "INVALID_TYPE", "INVALID_TYPE", "4294967784", "INVALID_TYPE", 2037145},
    {4294967295, -1, "Memcpy", "18121985196749930015", 0, 8865013624852, 0.60020725388601, 6, 34, 0, 0, 1, 1, 1, 1,
        "SDMA", 4, "INVALID_TYPE", "ON_CHIP", "18446744073709551615", "INVALID_TYPE", 2037145},
    {4294967295, -1, "Memcpy", "18121985196749930015", 0, 8865013624852, 0.6, 6, 34, 1, 0, 1, 1, 1, 1,
        "SDMA", 0, "INVALID_TYPE", "ON_CHIP", "18446744073709551615", "INVALID_TYPE", 2037145},
    {4294967295, -1, "Memcpy", "18121985196749930015", 0, 8865013624852, 0.6, 6, 34, 2, 0, 1, 1, 1, 1,
        "SDMA", 0, "INVALID_TYPE", "ON_CHIP", "18446744073709551615", "INVALID_TYPE", 2037145},
    {4294967295, -1, "RDMASend", "18121985196749930015", 0, 8865013624852, 7.00033333333333, 6, 34, 3, 0, 1, 1, 1, 0,
        "RDMA", 4, "INVALID_TYPE", "ROCE", "1432", "RDMA_SEND_NOTIFY", 2037145},
    {4294967295, -1, "Notify_Wait", "18121985196749930015", 0, 8865013624852, 0.02, 6, 34, 4, 0, 1, 1, 1, 0,
        "LOCAL", 0, "INVALID_TYPE", "INVALID_TYPE", "4294968712", "INVALID_TYPE", 2037145},
    {4294967295, -1, "RDMASend", "18121985196749930015", 0, 8865013624852, 7.00033333333333, 6, 34, 5, 0, 1, 1, 1, 0,
        "RDMA", 4, "INVALID_TYPE", "ROCE", "4294967295", "RDMA_SEND_PAYLOAD", 2037145},
    {4294967295, -1, "RDMASend", "18121985196749930015", 0, 8865013624852, 7.00033333333333, 6, 34, 6, 0, 1, 1, 1, 0,
        "RDMA", 4, "INVALID_TYPE", "ROCE", "1428", "RDMA_SEND_NOTIFY", 2037145},
    {4294967295, -1, "Notify_Wait", "18121985196749930015", 0, 8865013624852, 0.02, 6, 34, 7, 0, 1, 1, 1, 1,
        "LOCAL", 0, "INVALID_TYPE", "INVALID_TYPE", "4294968708", "INVALID_TYPE", 2037145},
    {4294967295, -1, "Memcpy", "18121985196749930015", 0, 8865013624852, 0.6, 6, 34, 8, 0, 1, 1, 1, 0,
        "SDMA", 0, "INVALID_TYPE", "ON_CHIP", "18446744073709551615", "INVALID_TYPE", 2037145},
    {4294967295, -1, "RDMASend", "18121985196749930015", 0, 8865013624852, 7.00033333333333, 6, 34, 9, 0, 1, 1, 1, 0,
        "RDMA", 4, "INVALID_TYPE", "ROCE", "1432", "RDMA_SEND_NOTIFY", 2037145},
    {4294967295, -1, "Notify_Wait", "18121985196749930015", 0, 8865013624852, 0.02, 6, 34, 10, 0, 1, 1, 1, 6,
        "LOCAL", 0, "INVALID_TYPE", "INVALID_TYPE", "4294968712", "INVALID_TYPE", 2037145},
    {4294967295, -1, "RDMASend", "18121985196749930015", 0, 8865013624852, 7, 6, 34, 11, 0, 1, 1, 1, 0,
     "RDMA", 300, "INVALID_TYPE", "ROCE", "4294968712", "RDMA_SEND_PAYLOAD", 2037145},
};

// modelId, indexId, streamId, taskId, contextId, batchId, startTime, duration,
// hostTaskType, deviceTaskType, connectionId
using AscendTaskDataFormat = std::vector<std::tuple<uint32_t, uint32_t, uint16_t, uint16_t, uint32_t, uint16_t,
        double, double, uint16_t, uint16_t, uint32_t>>;
const AscendTaskDataFormat ASCEND_TASK_DATA = {
    {4294967295, -1, 5, 5450, 0, 0, 88511176361580, 88511176362300, 0, 0, 17},
    {4294967295, -1, 5, 5450, 1, 0, 88511176364200, 88511176366660, 0, 0, 17},
    {4294967295, -1, 5, 5450, 2, 0, -1, -1, 0, 0, 17},
    {4294967295, -1, 5, 5450, 3, 0, 88511176367920, 88511176368620, 0, 0, 17},
    {4294967295, -1, 5, 5450, 4, 0, 88511176371560, 88511176395300, 0, 0, 17},
    {4294967295, -1, 5, 5450, 5, 0, 88511176368220, 88511176368920, 0, 0, 17},
    {4294967295, -1, 5, 5450, 6, 0, 88511176372460, 88511176397140, 0, 0, 17},
    {4294967295, -1, 5, 5450, 7, 0, 88511176368520, 88511176369220, 0, 0, 17},
    {4294967295, -1, 5, 5450, 8, 0, 88511176373420, 88511176391480, 0, 0, 17},
    {4294967295, -1, 6, 34, 0, 0, 88512126298900, 88512126299900, 0, 0, 9537},
    {4294967295, -1, 6, 34, 1, 0, -1, -1, 0, 0, 9537},
    {4294967295, -1, 6, 34, 2, 0, -1, -1, 0, 0, 9537},
    {4294967295, -1, 6, 34, 3, 0, 88512126302200, 88512126302520, 0, 0, 9537},
    {4294967295, -1, 6, 34, 4, 0, 88512126304340, 88512126304340, 0, 0, 9537},
    {4294967295, -1, 6, 34, 5, 0, 88512126306820, 88512126307140, 0, 0, 9537},
    {4294967295, -1, 6, 34, 6, 0, 88512126308560, 88512126308880, 0, 0, 9537},
    {4294967295, -1, 6, 34, 7, 0, 88512126310440, 88512126310460, 0, 0, 9537},
    {4294967295, -1, 6, 34, 8, 0, -1, -1, 0, 0, 9537},
    {4294967295, -1, 6, 34, 9, 0, 88512126312900, 88512126313220, 0, 0, 9537},
    {4294967295, -1, 6, 34, 10, 0, 88512126314920, 88512126317880, 0, 0, 9537},
    {4294967295, -1, 6, 34, 11, 0, 88512126316800, 88512126318800, 0, 0, 9537},
};


class HcclCalculatorUTest : public testing::Test {
protected:
    DataInventory dataInventory_;
protected:
    void SetUp() override
    {
        auto hcclOp = GenerateHcclOpData();
        std::shared_ptr<std::vector<HcclOp>> opData;
        MAKE_SHARED0_NO_OPERATION(opData, std::vector<HcclOp>, std::move(hcclOp));
        dataInventory_.Inject(opData);

        auto hcclTask = GenerateHcclTaskData();
        std::shared_ptr<std::vector<HcclTask>> hcclTaskData;
        MAKE_SHARED0_NO_OPERATION(hcclTaskData, std::vector<HcclTask>, std::move(hcclTask));
        dataInventory_.Inject(hcclTaskData);

        auto ascendTask = GenerateAscendTaskData();
        std::shared_ptr<std::vector<TopDownTask>> ascendTaskData;
        MAKE_SHARED0_NO_OPERATION(ascendTaskData, std::vector<TopDownTask>, std::move(ascendTask));
        dataInventory_.Inject(ascendTaskData);
    }

    void TearDown() override
    {
        dataInventory_.RemoveRestData({});
    }

    static std::vector<HcclOp> GenerateHcclOpData()
    {
        std::vector<HcclOp> opData;
        EXPECT_TRUE(Reserve(opData, HCCL_OP_DATA.size()));
        for (const auto& data : HCCL_OP_DATA) {
            HcclOp op;
            std::tie(op.deviceId, op.modelId, op.indexId, op.threadId, op.opName, op.taskType, op.opType, op.timestamp,
                     op.duration, op.isDynamic, op.connectionId, op.relay, op.retry, op.dataType, op.algType, op.count,
                     op.groupName) = data;
            opData.emplace_back(op);
        }
        return opData;
    }

    static std::vector<HcclTask> GenerateHcclTaskData()
    {
        std::vector<HcclTask> taskData;
        EXPECT_TRUE(Reserve(taskData, HCCL_TASK_DATA.size()));
        for (const auto& data : HCCL_TASK_DATA) {
            HcclTask task;
            std::tie(task.modelId, task.indexId, task.name, task.groupName, task.planeId, task.timestamp, task.duration,
                     task.streamId, task.taskId, task.contextId, task.batchId, task.deviceId, task.isMaster,
                     task.localRank, task.remoteRank, task.transportType, task.size, task.dataType, task.linkType,
                     task.notifyId, task.rdmaType, task.threadId) = data;
            taskData.emplace_back(task);
        }
        return taskData;
    }

    static std::vector<TopDownTask> GenerateAscendTaskData()
    {
        std::vector<TopDownTask> taskData;
        EXPECT_TRUE(Reserve(taskData, ASCEND_TASK_DATA.size()));
        for (const auto& data : ASCEND_TASK_DATA) {
            TopDownTask task;
            std::tie(task.modelId, task.indexId, task.streamId, task.taskId, task.contextId, task.batchId,
                     task.startTime, task.endTime, task.hostTaskType, task.deviceTaskType, task.connectionId) = data;
            taskData.emplace_back(task);
        }
        return taskData;
    }
};

TEST_F(HcclCalculatorUTest, TestProcessEntryWhenProcessSuccessThenReturnOK)
{
    HcclCalculator calculator;
    DeviceContext context;
    ASSERT_EQ(Analysis::ANALYSIS_OK, calculator.Run(dataInventory_, context));

    auto hcclOpData = dataInventory_.GetPtr<std::vector<HcclOp>>();
    EXPECT_EQ(HCCL_OP_DATA.size(), hcclOpData->size());

    auto hcclTaskData = dataInventory_.GetPtr<std::vector<DeviceHcclTask>>();
    // 去除4条时间为-1的非法数据
    EXPECT_EQ(HCCL_TASK_DATA.size() - 4, hcclTaskData->size());
    std::vector<double> expectBandwidth = {
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.004, 0.0125, 0.0, 0.00036166365280289331,
        0.0125, 0.0, 0.0125, 0.0, 0.15
    };
    size_t i = 0;
    for (const auto& data : *hcclTaskData) {
        EXPECT_DOUBLE_EQ(expectBandwidth[i], data.bandwidth);
        i++;
    }

    auto hcclStatisticsData = dataInventory_.GetPtr<std::vector<HcclStatistics>>();
    size_t expectStatisticsDataNum = 2;
    EXPECT_EQ(expectStatisticsDataNum, hcclStatisticsData->size());
}

TEST_F(HcclCalculatorUTest, TestAllTaskTimeIsEqualZeroThenReturnFalse)
{
    MOCKER(&Analysis::Utils::IsDoubleEqual).stubs().will(returnValue(false)).then(returnValue(true));
    HcclCalculator calculator;
    DeviceContext context;
    ASSERT_EQ(Analysis::ANALYSIS_OK, calculator.Run(dataInventory_, context));
    MOCKER(&Analysis::Utils::IsDoubleEqual).reset();
}