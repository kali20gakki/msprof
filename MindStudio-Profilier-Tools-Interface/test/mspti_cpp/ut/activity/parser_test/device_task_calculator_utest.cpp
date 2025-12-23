/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
*/

#include "gtest/gtest.h"

#include "mockcpp/mockcpp.hpp"

#include "csrc/activity/ascend/parser/parser_manager.h"
#include "csrc/activity/ascend/parser/cann_hash_cache.h"
#include "csrc/activity/ascend/parser/mstx_parser.h"
#include "csrc/activity/ascend/parser/device_task_calculator.h"
#include "csrc/common/inject/acl_inject.h"
#include "csrc/common/utils.h"
#include "securec.h"

namespace {
using namespace Mspti;
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
    Mspti::Parser::DeviceTask firstTask(0, 0, streamId, taskId, deviceId);
    instance.RegisterCallBack(firstTask, [deviceId](const Mspti::Parser::DeviceTask& task) {
        EXPECT_EQ(task.deviceId, 1);
        return MSPTI_SUCCESS;
    });

    HalLogData socLogStart;
    (void)memset_s(&socLogStart, sizeof(socLogStart), 0, sizeof(socLogStart));
    socLogStart.acsq.funcType = STARS_FUNC_TYPE_BEGIN;
    socLogStart.acsq.streamId = streamId;
    socLogStart.acsq.taskId = taskId;

    HalLogData socLogEnd;
    (void)memset_s(&socLogEnd, sizeof(socLogEnd), 0, sizeof(socLogEnd));
    socLogEnd.acsq.funcType = STARS_FUNC_TYPE_END;
    socLogEnd.acsq.streamId = streamId;
    socLogEnd.acsq.taskId = taskId;

    EXPECT_EQ(instance.ReportStarsSocLog(deviceId, socLogStart), MSPTI_SUCCESS);
    EXPECT_EQ(instance.ReportStarsSocLog(deviceId, socLogEnd), MSPTI_SUCCESS);
}

TEST_F(DeviceTaskCalculatorUtest, ShouldReturnTaskWhenFftsLogReport)
{
    uint16_t deviceId = 1;
    uint16_t streamId = 1;
    uint16_t taskId = 1;
    uint16_t subTaskId = 1;
    Mspti::Parser::DeviceTask firstTask(0, 0, streamId, taskId, deviceId);
    auto &instance = Mspti::Parser::DeviceTaskCalculator::GetInstance();
    instance.RegisterCallBack(firstTask, [deviceId](const Mspti::Parser::DeviceTask& task) {
        EXPECT_EQ(task.deviceId, deviceId);
        EXPECT_EQ(task.subTasks.size(), 1);
        return MSPTI_SUCCESS;
    });

    HalLogData socLogStart;
    (void)memset_s(&socLogStart, sizeof(socLogStart), 0, sizeof(socLogStart));
    socLogStart.type = ACSQ_LOG;
    socLogStart.acsq.funcType = STARS_FUNC_TYPE_BEGIN;
    socLogStart.acsq.streamId = streamId;
    socLogStart.acsq.taskId = taskId;

    HalLogData socLogEnd;
    (void)memset_s(&socLogEnd, sizeof(socLogEnd), 0, sizeof(socLogEnd));
    socLogEnd.type = ACSQ_LOG;
    socLogEnd.acsq.funcType = STARS_FUNC_TYPE_END;
    socLogEnd.acsq.streamId = streamId;
    socLogEnd.acsq.taskId = taskId;

    HalLogData fftsLogStart;
    (void)memset_s(&fftsLogStart, sizeof(fftsLogStart), 0, sizeof(fftsLogStart));
    fftsLogStart.type = FFTS_LOG;
    fftsLogStart.ffts.funcType = FFTS_PLUS_TYPE_START;
    fftsLogStart.ffts.streamId = streamId;
    fftsLogStart.ffts.taskId = taskId;
    fftsLogStart.ffts.subTaskId = subTaskId;

    HalLogData fftsLogEnd;
    (void)memset_s(&fftsLogEnd, sizeof(fftsLogEnd), 0, sizeof(fftsLogEnd));
    fftsLogEnd.type = FFTS_LOG;
    fftsLogEnd.ffts.funcType = FFTS_PLUS_TYPE_END;
    fftsLogEnd.ffts.streamId = streamId;
    fftsLogEnd.ffts.taskId = taskId;
    fftsLogEnd.ffts.subTaskId = subTaskId;

    instance.ReportStarsSocLog(deviceId, socLogStart);
    instance.ReportStarsSocLog(deviceId, fftsLogStart);
    instance.ReportStarsSocLog(deviceId, fftsLogEnd);
    instance.ReportStarsSocLog(deviceId, socLogEnd);
}

TEST_F(DeviceTaskCalculatorUtest, ShouldNotReturnTask)
{
    uint16_t deviceId = 1;
    uint16_t streamId = 1;
    uint16_t taskId = 1;
    uint16_t subTaskId = 1;
    Mspti::Parser::DeviceTask firstTask(0, 0, streamId, taskId, deviceId);
    auto &instance = Mspti::Parser::DeviceTaskCalculator::GetInstance();
    instance.RegisterCallBack(firstTask, [deviceId](const Mspti::Parser::DeviceTask& task) {
        EXPECT_TRUE(false);
        return MSPTI_SUCCESS;
    });

    HalLogData socLogStart;
    (void)memset_s(&socLogStart, sizeof(socLogStart), 0, sizeof(socLogStart));
    socLogStart.type = ACSQ_LOG;
    socLogStart.acsq.funcType = STARS_FUNC_TYPE_BEGIN;
    socLogStart.acsq.streamId = streamId;
    socLogStart.acsq.taskId = taskId;

    HalLogData fftsLogEnd;
    (void)memset_s(&fftsLogEnd, sizeof(fftsLogEnd), 0, sizeof(fftsLogEnd));
    fftsLogEnd.type = FFTS_LOG;
    fftsLogEnd.ffts.funcType = FFTS_PLUS_TYPE_END;
    fftsLogEnd.ffts.streamId = streamId + 1;
    fftsLogEnd.ffts.taskId = taskId + 1;
    fftsLogEnd.ffts.subTaskId = subTaskId + 1;

    instance.ReportStarsSocLog(deviceId, socLogStart);
    instance.ReportStarsSocLog(deviceId, fftsLogEnd);
}
}