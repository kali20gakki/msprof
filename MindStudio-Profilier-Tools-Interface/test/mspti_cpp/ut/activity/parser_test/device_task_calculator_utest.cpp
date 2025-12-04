/**
 * @file channel_utest.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 *
 */

#include "gtest/gtest.h"

#include "mockcpp/mockcpp.hpp"

#include "csrc/activity/ascend/parser/parser_manager.h"
#include "csrc/activity/ascend/parser/cann_hash_cache.h"
#include "csrc/activity/ascend/parser/mstx_parser.h"
#include "csrc/activity/ascend/parser/device_task_calculator.h"
#include "csrc/common/inject/runtime_inject.h"
#include "csrc/common/utils.h"
#include "securec.h"

namespace {
class DeviceTaskCalculatorUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
        GlobalMockObject::verify();
    }
    virtual void TearDown() {}
};

TEST_F(DeviceTaskCalculatorUtest, ShouldReturnTaskWhenSocLogReport)
{
    uint16_t deviceId = 1;
    uint16_t streamId = 1;
    uint16_t taskId = 1;
    auto &instance = Mspti::Parser::DeviceTaskCalculator::GetInstance();
    std::shared_ptr<Mspti::Parser::DeviceTask> firstTask =
        std::make_shared<Mspti::Parser::DeviceTask>(0, 0, streamId, taskId, deviceId);
    std::vector<std::shared_ptr<Mspti::Parser::DeviceTask>> assembleTasks{ firstTask, nullptr };
    instance.RegisterCallBack(assembleTasks, [deviceId](std::shared_ptr<Mspti::Parser::DeviceTask> task) {
        EXPECT_EQ(task->deviceId, 1);
        return MSPTI_SUCCESS;
    });

    StarsSocLog socLogStart;
    (void)memset_s(&socLogStart, sizeof(socLogStart), 0, sizeof(socLogStart));
    socLogStart.funcType = STARS_FUNC_TYPE_BEGIN;
    socLogStart.streamId = streamId;
    socLogStart.taskId = taskId;

    StarsSocLog socLogEnd;
    (void)memset_s(&socLogEnd, sizeof(socLogEnd), 0, sizeof(socLogEnd));
    socLogEnd.funcType = STARS_FUNC_TYPE_END;
    socLogEnd.streamId = streamId;
    socLogEnd.taskId = taskId;

    EXPECT_EQ(instance.ReportStarsSocLog(deviceId, reinterpret_cast<StarsSocHeader *>(&socLogStart)), MSPTI_SUCCESS);
    EXPECT_EQ(instance.ReportStarsSocLog(deviceId, reinterpret_cast<StarsSocHeader *>(&socLogEnd)), MSPTI_SUCCESS);
}

TEST_F(DeviceTaskCalculatorUtest, ShouldReturnTaskWhenFftsLogReport)
{
    uint16_t deviceId = 1;
    uint16_t streamId = 1;
    uint16_t taskId = 1;
    uint16_t subTaskId = 1;
    std::shared_ptr<Mspti::Parser::DeviceTask> firstTask =
        std::make_shared<Mspti::Parser::DeviceTask>(0, 0, streamId, taskId, deviceId);
    std::vector<std::shared_ptr<Mspti::Parser::DeviceTask>> assembleTasks{ firstTask };
    auto &instance = Mspti::Parser::DeviceTaskCalculator::GetInstance();
    instance.RegisterCallBack(assembleTasks, [deviceId](std::shared_ptr<Mspti::Parser::DeviceTask> task) {
        EXPECT_EQ(task->deviceId, deviceId);
        EXPECT_EQ(task->subTasks.size(), 1);
        return MSPTI_SUCCESS;
    });

    StarsSocLog socLogStart;
    (void)memset_s(&socLogStart, sizeof(socLogStart), 0, sizeof(socLogStart));
    socLogStart.funcType = STARS_FUNC_TYPE_BEGIN;
    socLogStart.streamId = streamId;
    socLogStart.taskId = taskId;

    StarsSocLog socLogEnd;
    (void)memset_s(&socLogEnd, sizeof(socLogEnd), 0, sizeof(socLogEnd));
    socLogEnd.funcType = STARS_FUNC_TYPE_END;
    socLogEnd.streamId = streamId;
    socLogEnd.taskId = taskId;

    FftsPlusLog fftsLogStart;
    (void)memset_s(&fftsLogStart, sizeof(fftsLogStart), 0, sizeof(fftsLogStart));
    fftsLogStart.funcType = FFTS_PLUS_TYPE_START;
    fftsLogStart.streamId = streamId;
    fftsLogStart.taskId = taskId;
    fftsLogStart.subTaskId = subTaskId;

    FftsPlusLog fftsLogEnd;
    (void)memset_s(&fftsLogEnd, sizeof(fftsLogEnd), 0, sizeof(fftsLogEnd));
    fftsLogEnd.funcType = FFTS_PLUS_TYPE_END;
    fftsLogEnd.streamId = streamId;
    fftsLogEnd.taskId = taskId;
    fftsLogEnd.subTaskId = subTaskId;

    instance.ReportStarsSocLog(deviceId, reinterpret_cast<StarsSocHeader *>(&socLogStart));
    instance.ReportStarsSocLog(deviceId, reinterpret_cast<StarsSocHeader *>(&fftsLogStart));
    instance.ReportStarsSocLog(deviceId, reinterpret_cast<StarsSocHeader *>(&fftsLogEnd));
    instance.ReportStarsSocLog(deviceId, reinterpret_cast<StarsSocHeader *>(&socLogEnd));
}

TEST_F(DeviceTaskCalculatorUtest, ShouldNotReturnTask)
{
    uint16_t deviceId = 1;
    uint16_t streamId = 1;
    uint16_t taskId = 1;
    uint16_t subTaskId = 1;
    std::shared_ptr<Mspti::Parser::DeviceTask> firstTask =
        std::make_shared<Mspti::Parser::DeviceTask>(0, 0, streamId, taskId, deviceId);
    std::vector<std::shared_ptr<Mspti::Parser::DeviceTask>> assembleTasks{ firstTask };
    auto &instance = Mspti::Parser::DeviceTaskCalculator::GetInstance();
    instance.RegisterCallBack(assembleTasks, [deviceId](std::shared_ptr<Mspti::Parser::DeviceTask> task) {
        EXPECT_TRUE(false);
        return MSPTI_SUCCESS;
    });


    StarsSocLog socLogStart;
    (void)memset_s(&socLogStart, sizeof(socLogStart), 0, sizeof(socLogStart));
    socLogStart.funcType = STARS_FUNC_TYPE_BEGIN;
    socLogStart.streamId = streamId;
    socLogStart.taskId = taskId;

    FftsPlusLog fftsLogEnd;
    (void)memset_s(&fftsLogEnd, sizeof(fftsLogEnd), 0, sizeof(fftsLogEnd));
    fftsLogEnd.funcType = FFTS_PLUS_TYPE_END;
    fftsLogEnd.streamId = streamId + 1;
    fftsLogEnd.taskId = taskId + 1;
    fftsLogEnd.subTaskId = subTaskId + 1;

    instance.ReportStarsSocLog(deviceId, reinterpret_cast<StarsSocHeader *>(&socLogStart));
    instance.ReportStarsSocLog(deviceId, reinterpret_cast<StarsSocHeader *>(&fftsLogEnd));
}
}