/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : ascend_hardware_assembler_utest.cpp
 * Description        : ascend_hardware_assembler UT
 * Author             : msprof team
 * Creation Date      : 2024/8/28
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/application/timeline/ascend_hardware_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/api_data.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/dfx/error_code.h"

using namespace Analysis::Application;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Parser::Environment;
namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./ascend_test";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, OUTPUT_PATH});
}
class AscendHardwareAssemblerUTest : public testing::Test {
protected:
    virtual void TearDown()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
        dataInventory_.RemoveRestData({});
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
protected:
    DataInventory dataInventory_;
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
    data.start = 1717575960208020758; // start 1717575960208020758
    data.duration = 450.78; // dur 450.78
    data.hostType = "KERNEL_AICORE";
    data.deviceType = "AI_CORE";
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
static std::vector<MsprofTxDeviceData> GenerateDeviceTxData()
{
    std::vector<MsprofTxDeviceData> res;
    MsprofTxDeviceData data;
    data.deviceId = 0; // device id 0
    data.indexId = 2147483647; // indexId 2147483647
    data.streamId = 1; // streamId 1
    data.taskId = 13; // taskId 13
    data.connectionId = 7890; // connectionId 7890
    res.push_back(data);
    return res;
}
TEST_F(AscendHardwareAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    AscendHardwareAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}
TEST_F(AscendHardwareAssemblerUTest, ShouldReturnTrueWhenDataAssembleSuccess)
{
    AscendHardwareAssembler assembler;
    std::shared_ptr<std::vector<AscendTaskData>> taskS;
    std::shared_ptr<std::vector<MsprofTxDeviceData>> txS;
    std::shared_ptr<std::vector<TaskInfoData>> infoS;
    auto task = GenerateTaskData();
    auto tx = GenerateDeviceTxData();
    auto info = GenerateTaskInfoData();
    MAKE_SHARED_NO_OPERATION(taskS, std::vector<AscendTaskData>, task);
    MAKE_SHARED_NO_OPERATION(txS, std::vector<MsprofTxDeviceData>, tx);
    MAKE_SHARED_NO_OPERATION(infoS, std::vector<TaskInfoData>, info);
    dataInventory_.Inject(taskS);
    dataInventory_.Inject(txS);
    dataInventory_.Inject(infoS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10086)); // pid 10086
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"MatMulV3\",\"pid\":10328480,\"tid\":1,\"ts\":\"1717575960208020.750000\","
                            "\"dur\":0.45077999999999996,\"ph\":\"X\",\"args\":{\"Model Id\":4294967295,\"Task Type\":"
                            "\"AI_CORE\",\"Physic Stream Id\":1,\"Task Id\":10,\"Batch Id\":1,\"Subtask Id\":1,"
                            "\"connection_id\":2345}},{\"name\":\"HostToDevice10071698309120\",\"pid\":10328480,"
                            "\"tid\":1,\"ph\":\"f\",\"cat\":\"HostToDevice\",\"id\":\"10071698309120\",\"ts\":"
                            "\"1717575960208020.750000\",\"bp\":\"e\"},{\"name\":\"MsTx_2147483647\",\"pid\":10328480,"
                            "\"tid\":1,\"ts\":\"18446744073709552.000000\",\"dur\":0.0,\"ph\":\"X\",\"args\":"
                            "{\"Physic Stream Id\":1,\"Task Id\":13}},{\"name\":\"MsTx_7890\",\"pid\":10328480,\"tid"
                            "\":1,\"ph\":\"f\",\"cat\":\"MsTx\",\"id\":\"7890\",\"ts\":\"18446744073709552.000000\","
                            "\"bp\":\"e\"},{\"name\":\"process_name\",\"pid\":10328480,\"tid\":0,\"ph\":\"M\",\"args"
                            "\":{\"name\":\"Ascend Hardware\"}},{\"name\":\"process_labels\",\"pid\":10328480,\"tid\""
                            ":0,\"ph\":\"M\",\"args\":{\"labels\":\"NPU\"}},{\"name\":\"process_sort_index\",\"pid\""
                            ":10328480,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":13}},{\"name\":\"thread_name\","
                            "\"pid\":10328480,\"tid\":1,\"ph\":\"M\",\"args\":{\"name\":\"Stream 1\"}},{\"name\":"
                            "\"thread_sort_index\",\"pid\":10328480,\"tid\":1,\"ph\":\"M\",\"args\":"
                            "{\"sort_index\":1}},";
    EXPECT_EQ(expectStr, res.back());
}
TEST_F(AscendHardwareAssemblerUTest, ShouldReturnFalseWhenDataAssembleFail)
{
    AscendHardwareAssembler assembler;
    std::shared_ptr<std::vector<AscendTaskData>> taskS;
    std::shared_ptr<std::vector<MsprofTxDeviceData>> txS;
    std::shared_ptr<std::vector<TaskInfoData>> infoS;
    auto task = GenerateTaskData();
    auto tx = GenerateDeviceTxData();
    auto info = GenerateTaskInfoData();
    MAKE_SHARED_NO_OPERATION(taskS, std::vector<AscendTaskData>, task);
    MAKE_SHARED_NO_OPERATION(txS, std::vector<MsprofTxDeviceData>, tx);
    MAKE_SHARED_NO_OPERATION(infoS, std::vector<TaskInfoData>, info);
    dataInventory_.Inject(taskS);
    dataInventory_.Inject(txS);
    dataInventory_.Inject(infoS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10087)); // pid 10087
    MOCKER_CPP(&std::vector<std::shared_ptr<TraceEvent>>::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}