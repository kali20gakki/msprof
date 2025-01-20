/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : timeline_manager_utest.cpp
 * Description        : timeline_manager UT
 * Author             : msprof team
 * Creation Date      : 2024/11/2
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/application/timeline/timeline_manager.h"
#include "analysis/csrc/application/timeline/json_constant.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/step_trace_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/msprof_tx_host_data.h"
#include "analysis/csrc/application/timeline/timeline_factory.h"

using namespace Analysis::Application;
using namespace Analysis::Domain;
namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./timeline_test";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, OUTPUT_PATH});
}

class TimelineManagerUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        GlobalMockObject::verify();
    }
    static void SetUpTestCase()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(RESULT_PATH));
    }
    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
        GlobalMockObject::verify();
    }
protected:
    DataInventory dataInventory_;
};

static std::vector<TrainTraceData> GenerateStepTraceData()
{
    std::vector<TrainTraceData> res;
    TrainTraceData data;
    res.emplace_back(data);
    return res;
}

static std::vector<MsprofTxHostData> GenerateHostTxData()
{
    std::vector<MsprofTxHostData> res;
    MsprofTxHostData data;
    res.emplace_back(data);
    return res;
}

TEST_F(TimelineManagerUTest, ShouldReturnTrueWhenNoDataInDataInventory)
{
    TimelineManager manager(PROF_PATH, RESULT_PATH);
    std::vector<JsonProcess> jsonProcess;
    EXPECT_TRUE(manager.Run(dataInventory_, jsonProcess));
}

TEST_F(TimelineManagerUTest, ShouldReturnTrueWhenDataProcessSuccess)
{
    std::shared_ptr<std::vector<TrainTraceData>> traceS;
    auto trace = GenerateStepTraceData();
    MAKE_SHARED_NO_OPERATION(traceS, std::vector<TrainTraceData>, trace);
    dataInventory_.Inject(traceS);
    std::shared_ptr<std::vector<MsprofTxHostData>> txS;
    auto tx = GenerateHostTxData();
    MAKE_SHARED_NO_OPERATION(txS, std::vector<MsprofTxHostData>, tx);
    dataInventory_.Inject(txS);
    std::vector<JsonProcess> jsonProcess;
    TimelineManager manager(PROF_PATH, RESULT_PATH);
    EXPECT_TRUE(manager.Run(dataInventory_, jsonProcess));
}

TEST_F(TimelineManagerUTest, ShouldReturnTrueWhenDataProcessSuccessWithJsonProcessList)
{
    std::shared_ptr<std::vector<TrainTraceData>> traceS;
    auto trace = GenerateStepTraceData();
    MAKE_SHARED_NO_OPERATION(traceS, std::vector<TrainTraceData>, trace);
    dataInventory_.Inject(traceS);
    std::shared_ptr<std::vector<MsprofTxHostData>> txS;
    auto tx = GenerateHostTxData();
    MAKE_SHARED_NO_OPERATION(txS, std::vector<MsprofTxHostData>, tx);
    dataInventory_.Inject(txS);
    // jsonProcess is not empty
    std::vector<JsonProcess> jsonProcess = {JsonProcess::ASCEND, JsonProcess::CANN, JsonProcess::FREQ};
    TimelineManager manager(PROF_PATH, RESULT_PATH);
    EXPECT_TRUE(manager.Run(dataInventory_, jsonProcess));
}

TEST_F(TimelineManagerUTest, ShouldReturnFalseWhenProcessTimeLineGetAssemblerByNameFailed)
{
    std::shared_ptr<std::vector<TrainTraceData>> traceS;
    auto trace = GenerateStepTraceData();
    MAKE_SHARED_NO_OPERATION(traceS, std::vector<TrainTraceData>, trace);
    dataInventory_.Inject(traceS);
    std::shared_ptr<std::vector<MsprofTxHostData>> txS;
    auto tx = GenerateHostTxData();
    MAKE_SHARED_NO_OPERATION(txS, std::vector<MsprofTxHostData>, tx);
    dataInventory_.Inject(txS);
    std::vector<JsonProcess> jsonProcess = {JsonProcess::ASCEND, JsonProcess::CANN, JsonProcess::FREQ};
    TimelineManager manager(PROF_PATH, RESULT_PATH);
    std::shared_ptr<JsonAssembler> assembler{nullptr};
    MOCKER_CPP(&TimelineFactory::GetAssemblerByName).stubs().will(returnValue(assembler));
    EXPECT_FALSE(manager.Run(dataInventory_, jsonProcess));
    MOCKER_CPP(&TimelineFactory::GetAssemblerByName).reset();
}
