/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : op_summary_assembler_utest.cpp
 * Description        : op_summary_assembler UT
 * Author             : msprof team
 * Creation Date      : 2024/12/3
 * *****************************************************************************
 */

#include <iostream>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/application/summary/op_summary_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/communication_info_data.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/application/summary/summary_constant.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

using namespace Analysis::Application;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Domain::Environment;

namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./op_summary_test";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, OUTPUT_PATH});
}

class OpSummaryAssemblerUTest : public testing::Test {
    virtual void TearDown()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
        GlobalMockObject::verify();
    }
    virtual void SetUp()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(RESULT_PATH));
    }
};

static std::vector<AscendTaskData> GenerateTaskData()
{
    std::vector<AscendTaskData> res;
    AscendTaskData data;
    data.deviceId = 0;  // device id 0
    data.indexId = -1; // index_id -1
    data.streamId = 1; // streamId 1
    data.taskId = 10; // taskId 10
    data.contextId = 1; // contextId 1
    data.batchId = 1; // batchId 1
    data.connectionId = 2345; // connectionId 2345
    data.timestamp = 1717575960208020758; // start 1717575960208020758
    data.end = 1717575960208171758; // end 1717575960208171758
    data.duration = 151000.0; // dur 151000.0
    data.hostType = "KERNEL_AICORE";
    data.deviceType = "AI_CORE";
    res.push_back(data);
    data.timestamp = 1717575960208040758; // start 1717575960208040758
    data.end = 1717575960208050758; // start 1717575960208050758
    res.push_back(data);
    return res;
}

static std::vector<TaskInfoData> GenerateTaskInfoData()
{
    std::vector<TaskInfoData> res;
    TaskInfoData data;
    data.deviceId = 0; // deviceId 0
    data.streamId = 1; // streamId 1
    data.taskId = 10; // taskId 10
    data.contextId = 1; // contextId 1
    data.batchId = 1; // batchId 1
    data.opName = "MatMulV3";
    res.push_back(data);
    return res;
}

static std::vector<CommunicationOpData> GenerateOpData()
{
    std::vector<CommunicationOpData> res;
    CommunicationOpData data;
    data.deviceId = 0; // deviceId 0
    data.modelId = UINT32_MAX;
    data.opName = "hcom_broadcast__515_1_1";
    data.opType = NA;
    data.timestamp = 1717575960208030858; // start 1717575960208030858
    data.end = 1717575960208031858; // end 1717575960208031858
    res.push_back(data);
    return res;
}

static MetricSummary GenerateMetricSummary()
{
    MetricSummary summary;
    std::map<TaskId, std::vector<std::vector<std::string>>> data;
    std::vector<std::string> row{"140.106", "5043798", "33.09", "0.236", "7.419", "0.053", "81.861", "0.584", "83.023",
                                 "0.593", "17.645", "0.126", "0.006", "0.0", "0", "0.0", "0.0", "0.0", "0.0", "0.0",
                                 "0.0", "0.0", "0.0", "0.0"};
    data[{1, 1, 10, 1, 0}].emplace_back(row);
    summary.data = data;
    summary.labels = {"aicore_time(us)", "aic_total_cycles", "aic_mac_time(us)", "aic_mac_ratio",
                      "aic_scalar_time(us)", "aic_scalar_ratio", "aic_mte1_time(us)", "aic_mte1_ratio",
                      "aic_mte2_time(us)", "aic_mte2_ratio", "aic_fixpipe_time(us)", "aic_fixpipe_ratio",
                      "aic_icache_miss_rate", "aiv_time(us)", "aiv_total_cycles", "aiv_vec_time(us)", "aiv_vec_ratio",
                      "aiv_scalar_time(us)", "aiv_scalar_ratio", "aiv_mte2_time(us)", "aiv_mte2_ratio",
                      "aiv_mte3_time(us)", "aiv_mte3_ratio", "aiv_icache_miss_rate"};
    return summary;
}

TEST_F(OpSummaryAssemblerUTest, ShouldReturnTrueWhenDataNotExist)
{
    OpSummaryAssembler assembler(PROCESSOR_OP_SUMMARY, PROF_PATH);
    DataInventory dataInventory;
    EXPECT_TRUE(assembler.Run(dataInventory));
    auto files = File::GetOriginData(RESULT_PATH, {"op_summary"}, {});
    EXPECT_EQ(0ul, files.size());
}

// 非Stars芯片HCCL+Task（L0，L1一样）
TEST_F(OpSummaryAssemblerUTest, ShouldReturnTrueWhenTaskAndHcclExistWithNoStars)
{
    DataInventory dataInventory;
    std::shared_ptr<std::vector<AscendTaskData>> taskS;
    std::shared_ptr<std::vector<TaskInfoData>> infoS;
    std::shared_ptr<std::vector<CommunicationOpData>> opDataS;
    auto task = GenerateTaskData();
    auto info = GenerateTaskInfoData();
    auto opData = GenerateOpData();
    MAKE_SHARED_NO_OPERATION(taskS, std::vector<AscendTaskData>, task);
    MAKE_SHARED_NO_OPERATION(infoS, std::vector<TaskInfoData>, info);
    MAKE_SHARED_NO_OPERATION(opDataS, std::vector<CommunicationOpData>, opData);
    dataInventory.Inject(taskS);
    dataInventory.Inject(infoS);
    dataInventory.Inject(opDataS);
    OpSummaryAssembler assembler(PROCESSOR_OP_SUMMARY, PROF_PATH);
    EXPECT_TRUE(assembler.Run(dataInventory));
    auto files = File::GetOriginData(RESULT_PATH, {"op_summary"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_EQ(4ul, res.size());
    std::string expectHeader{"Device_id,Model ID,Task ID,Stream ID,Op Name,OP Type,OP State,Task Type,"
                             "Task Start Time(us),Task Duration(us),Task Wait Time(us),Block Dim,HF32 Eligible,"
                             "Input Shapes,Input Data Types,Input Formats,Output Shapes,Output Data Types,"
                             "Output Formats"};
    EXPECT_EQ(expectHeader, res[0]);
}

// Stars芯片HCCL（L0,L1一样）
TEST_F(OpSummaryAssemblerUTest, ShouldReturnTrueWhenOnlyHcclWithStars)
{
    DataInventory dataInventory;
    std::shared_ptr<std::vector<CommunicationOpData>> opDataS;
    auto opData = GenerateOpData();
    MAKE_SHARED_NO_OPERATION(opDataS, std::vector<CommunicationOpData>, opData);
    dataInventory.Inject(opDataS);
    OpSummaryAssembler assembler(PROCESSOR_OP_SUMMARY, PROF_PATH);
    MOCKER_CPP(&Context::IsStarsChip).stubs().will(returnValue(true));
    EXPECT_TRUE(assembler.Run(dataInventory));
    auto files = File::GetOriginData(RESULT_PATH, {"op_summary"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_EQ(2ul, res.size());
    std::string expectHeader{"Device_id,Model ID,Task ID,Stream ID,Op Name,OP Type,OP State,Task Type,"
                             "Task Start Time(us),Task Duration(us),Task Wait Time(us),Context ID"};
    EXPECT_EQ(expectHeader, res[0]);
}

// Stars芯片HCCL+Task+Pmu（最长的表头）
TEST_F(OpSummaryAssemblerUTest, ShouldReturnTrueWhenTaskAndHcclAndPmuExistWithStars)
{
    DataInventory dataInventory;
    std::shared_ptr<std::vector<AscendTaskData>> taskS;
    std::shared_ptr<std::vector<TaskInfoData>> infoS;
    std::shared_ptr<std::vector<CommunicationOpData>> opDataS;
    std::shared_ptr<MetricSummary> metricDataS;
    auto task = GenerateTaskData();
    auto info = GenerateTaskInfoData();
    auto opData = GenerateOpData();
    auto metricSummary = GenerateMetricSummary();
    MAKE_SHARED_NO_OPERATION(taskS, std::vector<AscendTaskData>, task);
    MAKE_SHARED_NO_OPERATION(infoS, std::vector<TaskInfoData>, info);
    MAKE_SHARED_NO_OPERATION(opDataS, std::vector<CommunicationOpData>, opData);
    MAKE_SHARED_NO_OPERATION(metricDataS, MetricSummary, metricSummary);
    dataInventory.Inject(taskS);
    dataInventory.Inject(infoS);
    dataInventory.Inject(opDataS);
    dataInventory.Inject(metricDataS);
    OpSummaryAssembler assembler(PROCESSOR_OP_SUMMARY, PROF_PATH);
    nlohmann::json record = {
        {"DeviceInfo", {{{"hwts_frequency", "1800"}, {"aic_frequency", "1800"}, {"ai_core_num", 20}}}},
        {"platform_version", "5"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    EXPECT_TRUE(assembler.Run(dataInventory));
    auto files = File::GetOriginData(RESULT_PATH, {"op_summary"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_EQ(4ul, res.size());
    std::string expectHeader{"Device_id,Model ID,Task ID,Stream ID,Op Name,OP Type,OP State,Task Type,"
                             "Task Start Time(us),Task Duration(us),Task Wait Time(us),Block Dim,Mix Block Dim,"
                             "HF32 Eligible,Input Shapes,Input Data Types,Input Formats,Output Shapes,"
                             "Output Data Types,Output Formats,Context ID,aicore_time(us),aic_total_cycles,"
                             "aic_mac_time(us),aic_mac_ratio,aic_scalar_time(us),aic_scalar_ratio,aic_mte1_time(us),"
                             "aic_mte1_ratio,aic_mte2_time(us),aic_mte2_ratio,aic_fixpipe_time(us),aic_fixpipe_ratio,"
                             "aic_icache_miss_rate,aiv_time(us),aiv_total_cycles,aiv_vec_time(us),aiv_vec_ratio,"
                             "aiv_scalar_time(us),aiv_scalar_ratio,aiv_mte2_time(us),aiv_mte2_ratio,aiv_mte3_time(us),"
                             "aiv_mte3_ratio,aiv_icache_miss_rate,cube_utilization(%)"};
    EXPECT_EQ(expectHeader, res[0]);
    std::string expectRowOne{"0,4294967295,10,1,MatMulV3,,,,1717575960208020.758\t,151.000,0,4294967295"
                             ",4294967295,,,,,,,,1,140.106,5043798,33.09,0.236,7.419,0.053,81.861,0.584,83.023,0.593,"
                             "17.645,0.126,0.006,0.0,0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,92.785"};
    EXPECT_EQ(expectRowOne, res[1]);
}

TEST_F(OpSummaryAssemblerUTest, ShouldReturnTrueWhenTaskAndHcclAndPmuExistWithStarsAndAddCubeUsuageFailed)
{
    DataInventory dataInventory;
    std::shared_ptr<std::vector<AscendTaskData>> taskS;
    std::shared_ptr<std::vector<TaskInfoData>> infoS;
    std::shared_ptr<std::vector<CommunicationOpData>> opDataS;
    std::shared_ptr<MetricSummary> metricDataS;
    auto task = GenerateTaskData();
    auto info = GenerateTaskInfoData();
    auto opData = GenerateOpData();
    auto metricSummary = GenerateMetricSummary();
    MAKE_SHARED_NO_OPERATION(taskS, std::vector<AscendTaskData>, task);
    MAKE_SHARED_NO_OPERATION(infoS, std::vector<TaskInfoData>, info);
    MAKE_SHARED_NO_OPERATION(opDataS, std::vector<CommunicationOpData>, opData);
    MAKE_SHARED_NO_OPERATION(metricDataS, MetricSummary, metricSummary);
    dataInventory.Inject(taskS);
    dataInventory.Inject(infoS);
    dataInventory.Inject(opDataS);
    dataInventory.Inject(metricDataS);
    OpSummaryAssembler assembler(PROCESSOR_OP_SUMMARY, PROF_PATH);
    nlohmann::json record = {
        {"DeviceInfo", {{{"hwts_frequency", "1800"}, {"aic_frequency", "1800"}, {"ai_core_num", 20}}}},
        {"platform_version", "5"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    uint16_t zeroCoreNum = 0;
    MOCKER_CPP(&Context::GetAiCoreNum).stubs().will(returnValue(zeroCoreNum));
    EXPECT_TRUE(assembler.Run(dataInventory));
    auto files = File::GetOriginData(RESULT_PATH, {"op_summary"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_EQ(4ul, res.size());
    std::string expectHeader{"Device_id,Model ID,Task ID,Stream ID,Op Name,OP Type,OP State,Task Type,"
                             "Task Start Time(us),Task Duration(us),Task Wait Time(us),Block Dim,Mix Block Dim,"
                             "HF32 Eligible,Input Shapes,Input Data Types,Input Formats,Output Shapes,"
                             "Output Data Types,Output Formats,Context ID,aicore_time(us),aic_total_cycles,"
                             "aic_mac_time(us),aic_mac_ratio,aic_scalar_time(us),aic_scalar_ratio,aic_mte1_time(us),"
                             "aic_mte1_ratio,aic_mte2_time(us),aic_mte2_ratio,aic_fixpipe_time(us),aic_fixpipe_ratio,"
                             "aic_icache_miss_rate,aiv_time(us),aiv_total_cycles,aiv_vec_time(us),aiv_vec_ratio,"
                             "aiv_scalar_time(us),aiv_scalar_ratio,aiv_mte2_time(us),aiv_mte2_ratio,aiv_mte3_time(us),"
                             "aiv_mte3_ratio,aiv_icache_miss_rate"};
    EXPECT_EQ(expectHeader, res[0]);
    std::string expectRowOne{"0,4294967295,10,1,MatMulV3,,,,1717575960208020.758\t,151.000,0,4294967295"
                             ",4294967295,,,,,,,,1,140.106,5043798,33.09,0.236,7.419,0.053,81.861,0.584,83.023,0.593,"
                             "17.645,0.126,0.006,0.0,0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0"};
    EXPECT_EQ(expectRowOne, res[1]);
    MOCKER_CPP(&Context::GetAiCoreNum).reset();
}
