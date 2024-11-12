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

TEST_F(TimelineManagerUTest, ShouldReturnFalseWhenNoDataInDataInventory)
{
    TimelineManager manager(PROF_PATH, RESULT_PATH);
    EXPECT_FALSE(manager.Run(dataInventory_));
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
    TimelineManager manager(PROF_PATH, RESULT_PATH);
    EXPECT_TRUE(manager.Run(dataInventory_));
}