/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : data_inventory_utest.cpp
 * Description        : DataInventory UT
 * Author             : msprof team
 * Creation Date      : 2025/07/24
 * *****************************************************************************
 */
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "analysis/csrc/domain/services/modeling/step_trace/state.h"
#include "analysis/csrc/domain/services/modeling/step_trace/step_trace_constant.h"
namespace Analysis {
namespace Domain {
namespace {
}
class StateUtest : public testing::Test {
public:
    static HalTrackData getHalTrackData()
    {
        HalTrackData track_data;
        HalStepTrace stepTrace;
        stepTrace.tagId = 0;
        track_data.stepTrace = stepTrace;
        return track_data;
    }
protected:
    void SetUp() override
    {
    }
    void TearDown() override
    {
    }
    State state;
    PreEndState preEndState;
    StartState startState;
    EndState endState;
};
TEST_F(StateUtest, TestInnerProcessEventWhenTagIdInvalid)
{
    HalTrackData track_data = StateUtest::getHalTrackData();
    uint32_t index = 0;
    StepTraceTasks task1;
    std::vector<StepTraceTasks> tasks = {task1};
    state.InnerProcessEvent(index, track_data, tasks);
    EXPECT_EQ(track_data.stepTrace.tagId, 0);
}
TEST_F(StateUtest, TestStepEndEventSuccess)
{
    uint32_t index = 1;
    StepTraceTasks task1;
    std::vector<StepTraceTasks> tasks = {task1};
    State& result = state.StepEndEvent(index, StateUtest::getHalTrackData(), tasks);
    EXPECT_EQ(&result, &state);
}
TEST_F(StateUtest, TestPreEndStateModelStartWhenBaseStepEmpty)
{
    uint32_t index = 0;
    std::vector<StepTraceTasks> tasks = {};
    State& result = preEndState.ModelStartEvent(index, StateUtest::getHalTrackData(), tasks);
    EXPECT_EQ(&result, &preEndState);
}
TEST_F(StateUtest, TestPreEndStateStepEndEventSuccess)
{
    uint32_t index = 0;
    StepTraceTasks task1;
    std::vector<StepTraceTasks> tasks = {task1};
    State& result = preEndState.StepEndEvent(index, StateUtest::getHalTrackData(), tasks);
    EXPECT_EQ(tasks.size(), 2ul);
    EXPECT_NE(&result, &preEndState);
}
TEST_F(StateUtest, TestPreEndStateStepEndEventWhenBaseStepEmpty)
{
    uint32_t index = 0;
    std::vector<StepTraceTasks> tasks = {};
    State& result = preEndState.StepEndEvent(index, StateUtest::getHalTrackData(), tasks);
    EXPECT_EQ(&result, &preEndState);
}
TEST_F(StateUtest, TestEndStateStepEndEventWhenBaseStepEmpty)
{
    uint32_t index = 0;
    std::vector<StepTraceTasks> tasks = {};
    State& result = endState.StepEndEvent(index, StateUtest::getHalTrackData(), tasks);
    EXPECT_EQ(&result, &endState);
}
TEST_F(StateUtest, TestEndStateModelEndEventSuccess)
{
    uint32_t index = 0;
    StepTraceTasks task1;
    std::vector<StepTraceTasks> tasks = {task1};
    State& result = endState.ModeLEndEvent(index, StateUtest::getHalTrackData(), tasks);
    EXPECT_NE(&result, &endState);
}
TEST_F(StateUtest, TestEndStateModelEndEventWhenBaseStepEmpty)
{
    uint32_t index = 0;
    std::vector<StepTraceTasks> tasks = {};
    State& result = endState.ModeLEndEvent(index, StateUtest::getHalTrackData(), tasks);
    EXPECT_EQ(&result, &endState);
}
}
}