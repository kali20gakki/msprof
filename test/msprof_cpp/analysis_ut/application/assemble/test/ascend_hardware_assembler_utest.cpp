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
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/memcpy_info_data.h"
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
    data.contextId = UINT32_MAX;
    res.push_back(data);
    data.connectionId = 1234; // connectionId 1234
    data.hostType = "EVENT_RECORD";
    data.deviceType = "EVENT_RECORD_SQE";
    data.taskId = 11; // taskId 11
    res.push_back(data);
    data.taskId = 0;
    data.batchId = 0;
    data.start = 1717575960208021758;  // start 1717575960208021758
    data.duration = 4241260;  // duration 4241260
    data.hostType = "MEMCPY_ASYNC";
    data.deviceType = "PCIE_DMA_SQE";
    data.connectionId = 181;  // connectionId 181
    res.push_back(data);
    data.taskId = 1;
    data.start = 1717575960213021758;  // start 1717575960213021758
    data.duration = 1202500; // duration 1202500
    res.push_back(data);
    data.taskId = 2;  // taskId 2
    data.start = 1717575960215021758;  // start 1717575960215021758
    data.duration = 3036900;  // duration 3036900
    data.connectionId = 189;  // connectionId 189
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
    data.contextId = UINT32_MAX;
    res.push_back(data);
    return res;
}

static std::vector<KfcTurnData> GenerateKfcTurnData()
{
    std::vector<KfcTurnData> res;
    KfcTurnData data;
    data.opName = "WaitExecute";
    data.deviceId = 0; // device id 0
    data.streamId = 1; // streamId 1
    data.taskId = 15; // taskId 15
    data.startTime = 1717575960208020758; // start 1717575960208020758
    res.push_back(data);
    return res;
}

static std::vector<ApiData> GenerateApiData()
{
    std::vector<ApiData> res;
    ApiData data;
    data.structType = "ACL_RTS";
    data.id = "aclrtRecordEvent";
    data.level = 20000; // level 20000
    data.threadId = 113991; // threadId 113991
    data.apiName = "aclrtRecordEvent";
    data.itemId = "0";
    data.connectionId = 1234; // connectionId 1234
    res.push_back(data);
    return res;
}

static std::vector<MemcpyInfoData> GenerateMemcpyInfoData()
{
    std::vector<MemcpyInfoData> res;
    MemcpyInfoData data;
    TaskId taskId1{1, 0, 0, UINT32_MAX, 0};  // streamId:1, taskId:0
    TaskId taskId2{1, 0, 1, UINT32_MAX, 0};  // streamId:1, taskId:1
    TaskId taskId3{1, 0, 2, UINT32_MAX, 0};  // streamId:1, taskId:2
    data.taskId = taskId1;
    data.dataSize = 67108864;  // dataSize 67108864
    data.memcpyOperation = 1;
    res.push_back(data);
    data.taskId = taskId2;
    data.dataSize = 16777216;  // dataSize 16777216
    res.push_back(data);
    data.taskId = taskId3;
    data.dataSize = 35651584;  // dataSize 35651584
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
    std::shared_ptr<std::vector<TaskInfoData>> infoS;
    std::shared_ptr<std::vector<KfcTurnData>> kfcDatas;
    std::shared_ptr<std::vector<ApiData>> apiS;
    std::shared_ptr<std::vector<MemcpyInfoData>> memcpyS;
    auto api = GenerateApiData();
    auto task = GenerateTaskData();
    auto info = GenerateTaskInfoData();
    auto kfcData = GenerateKfcTurnData();
    auto memcpyInfoData = GenerateMemcpyInfoData();
    MAKE_SHARED_NO_OPERATION(taskS, std::vector<AscendTaskData>, task);
    MAKE_SHARED_NO_OPERATION(infoS, std::vector<TaskInfoData>, info);
    MAKE_SHARED_NO_OPERATION(kfcDatas, std::vector<KfcTurnData>, kfcData);
    MAKE_SHARED_NO_OPERATION(apiS, std::vector<ApiData>, api);
    MAKE_SHARED_NO_OPERATION(memcpyS, std::vector<MemcpyInfoData>, memcpyInfoData);
    dataInventory_.Inject(taskS);
    dataInventory_.Inject(infoS);
    dataInventory_.Inject(kfcDatas);
    dataInventory_.Inject(apiS);
    dataInventory_.Inject(memcpyS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10086)); // pid 10086
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"MatMulV3\",\"pid\":10328480,\"tid\":1,\"ts\":\"1717575960208020.758\",\"dur\""
                            ":0.45077999999999996,\"ph\":\"X\",\"args\":{\"Model Id\":4294967295,\"Task Type\":\""
                            "AI_CORE\",\"Physic Stream Id\":1,\"Task Id\":10,\"Batch Id\":1,\"Subtask Id\":1,\""
                            "connection_id\":2345}},{\"name\":\"HostToDevice10071698309120\",\"pid\":10328480,\"tid\":"
                            "1,\"ph\":\"f\",\"cat\":\"HostToDevice\",\"id\":\"10071698309120\",\"ts\":\""
                            "1717575960208020.758\",\"bp\":\"e\"},{\"name\":\"EVENT_RECORD\",\"pid\":10328480,\"tid\":"
                            "1,\"ts\":\"1717575960208020.758\",\"dur\":0.45077999999999996,\"ph\":\"X\",\"args\":{\""
                            "Model Id\":4294967295,\"Task Type\":\"EVENT_RECORD_SQE\",\"Physic Stream Id\":1,\"Task Id"
                            "\":11,\"Batch Id\":1,\"Subtask Id\":4294967295,\"connection_id\":1234}},{\"name\":\""
                            "HostToDevice5299989643264\",\"pid\":10328480,\"tid\":1,\"ph\":\"f\",\"cat\":\""
                            "HostToDevice\",\"id\":\"5299989643264\",\"ts\":\"1717575960208020.758\",\"bp\":\"e\"},{\""
                            "name\":\"MEMCPY_ASYNC\",\"pid\":10328480,\"tid\":1,\"ts\":\"1717575960208021.758\",\""
                            "dur\":4241.26,\"ph\":\"X\",\"args\":{\"Model Id\":4294967295,\"Task Type\":\"PCIE_DMA_SQE"
                            "\",\"Physic Stream Id\":1,\"Task Id\":0,\"Batch Id\":0,\"Subtask Id\":4294967295,\""
                            "connection_id\":181,\"size(B)\":67108864,\"bandwidth(GB/s)\":15.822860187774388,"
                            "\"operation\":\"host to device\"}},{\"name\":\"HostToDevice777389080576\",\"pid\":10328480"
                            ",\"tid\":1,\"ph\":\"f\",\"cat\":\"HostToDevice\",\"id\":\"777389080576\",\"ts\":"
                            "\"1717575960208021.758\",\"bp\":\"e\"},{\"name\":\"MEMCPY_ASYNC\",\"pid\":10328480,\"tid"
                            "\":1,\"ts\":\"1717575960213021.758\",\"dur\":1202.5,\"ph\":\"X\",\"args\":{\"Model Id\":"
                            "4294967295,\"Task Type\":\"PCIE_DMA_SQE\",\"Physic Stream Id\":1,\"Task Id\":1,\"Batch "
                            "Id\":0,\"Subtask Id\":4294967295,\"connection_id\":181,\"size(B)\":16777216,"
                            "\"bandwidth(GB/s)\":13.951946777546777,\"operation\":\"host to device\"}},{\"name\":"
                            "\"HostToDevice777389080576\",\"pid\":10328480,\"tid\":1,\"ph\":\"f\",\"cat\":"
                            "\"HostToDevice\",\"id\":\"777389080576\",\"ts\":\"1717575960213021.758\",\"bp\":\"e\"},{"
                            "\"name\":\"MEMCPY_ASYNC\",\"pid\":10328480,\"tid\":1,\"ts\":\"1717575960215021.758\","
                            "\"dur\":3036.9,\"ph\":\"X\",\"args\":{\"Model Id\":4294967295,\"Task Type\":\"PCIE_DMA_SQE"
                            "\",\"Physic Stream Id\":1,\"Task Id\":2,\"Batch Id\":0,\"Subtask Id\":4294967295,"
                            "\"connection_id\":189,\"size(B)\":35651584,\"bandwidth(GB/s)\":11.739465902729757,"
                            "\"operation\":\"host to device\"}},{\"name\":\"HostToDevice811748818944\",\"pid\":10328480"
                            ",\"tid\":1,\"ph\":\"f\",\"cat\":\"HostToDevice\",\"id\":\"811748818944\",\"ts\":"
                            "\"1717575960215021.758\",\"bp\":\"e\"},{\""
                            "name\":\"WaitExecute\",\"pid\":10328480,\"tid\":1,\"ts\":\"1717575960208020.758\",\""
                            "dur\":0.0,\"ph\":\"X\",\"args\":{\"Physic Stream Id\":1,\"Task Id\":15}},{\"name\":\""
                            "process_name\",\"pid\":10328480,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\""
                            "Ascend Hardware\"}},{\"name\":\"process_labels\",\"pid\":10328480,\"tid\":0,\"ph\":\"M\","
                            "\"args\":{\"labels\":\"NPU\"}},{\"name\":\"process_sort_index\",\"pid\":10328480,\"tid\":"
                            "0,\"ph\":\"M\",\"args\":{\"sort_index\":13}},{\"name\":\"thread_name\",\"pid\":10328480,"
                            "\"tid\":1,\"ph\":\"M\",\"args\":{\"name\":\"Stream 1\"}},{\"name\":\"thread_sort_index\","
                            "\"pid\":10328480,\"tid\":1,\"ph\":\"M\",\"args\":{\"sort_index\":1}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(AscendHardwareAssemblerUTest, ShouldReturnFalseWhenDataAssembleFail)
{
    AscendHardwareAssembler assembler;
    std::shared_ptr<std::vector<AscendTaskData>> taskS;
    std::shared_ptr<std::vector<TaskInfoData>> infoS;
    std::shared_ptr<std::vector<KfcTurnData>> kfcDatas;
    auto task = GenerateTaskData();
    auto info = GenerateTaskInfoData();
    auto kfcData = GenerateKfcTurnData();
    MAKE_SHARED_NO_OPERATION(taskS, std::vector<AscendTaskData>, task);
    MAKE_SHARED_NO_OPERATION(infoS, std::vector<TaskInfoData>, info);
    MAKE_SHARED_NO_OPERATION(kfcDatas, std::vector<KfcTurnData>, kfcData);
    dataInventory_.Inject(taskS);
    dataInventory_.Inject(infoS);
    dataInventory_.Inject(kfcDatas);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10087)); // pid 10087
    MOCKER_CPP(&std::vector<std::shared_ptr<TraceEvent>>::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(AscendHardwareAssemblerUTest, ShouldReturnTrueWhenDataAssembleWithoutApi)
{
    AscendHardwareAssembler assembler;
    std::shared_ptr<std::vector<AscendTaskData>> taskS;
    std::shared_ptr<std::vector<TaskInfoData>> infoS;
    std::shared_ptr<std::vector<KfcTurnData>> kfcDatas;
    auto api = GenerateApiData();
    auto task = GenerateTaskData();
    auto info = GenerateTaskInfoData();
    auto kfcData = GenerateKfcTurnData();
    MAKE_SHARED_NO_OPERATION(taskS, std::vector<AscendTaskData>, task);
    MAKE_SHARED_NO_OPERATION(infoS, std::vector<TaskInfoData>, info);
    MAKE_SHARED_NO_OPERATION(kfcDatas, std::vector<KfcTurnData>, kfcData);
    dataInventory_.Inject(taskS);
    dataInventory_.Inject(infoS);
    dataInventory_.Inject(kfcDatas);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10086)); // pid 10086
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"MatMulV3\",\"pid\":10328480,\"tid\":1,\"ts\":\"1717575960208020.758\",\"dur\""
                            ":0.45077999999999996,\"ph\":\"X\",\"args\":{\"Model Id\":4294967295,\"Task Type\":\""
                            "AI_CORE\",\"Physic Stream Id\":1,\"Task Id\":10,\"Batch Id\":1,\"Subtask Id\":1,\""
                            "connection_id\":2345}},{\"name\":\"HostToDevice10071698309120\",\"pid\":10328480,\"tid\":"
                            "1,\"ph\":\"f\",\"cat\":\"HostToDevice\",\"id\":\"10071698309120\",\"ts\":\""
                            "1717575960208020.758\",\"bp\":\"e\"},{\"name\":\"EVENT_RECORD\",\"pid\":10328480,\"tid\":"
                            "1,\"ts\":\"1717575960208020.758\",\"dur\":0.45077999999999996,\"ph\":\"X\",\"args\":{\""
                            "Model Id\":4294967295,\"Task Type\":\"EVENT_RECORD_SQE\",\"Physic Stream Id\":1,\"Task Id"
                            "\":11,\"Batch Id\":1,\"Subtask Id\":4294967295,\"connection_id\":1234}},{\"name\":\""
                            "MEMCPY_ASYNC\",\"pid\":10328480,\"tid\":1,\"ts\":\"1717575960208021.758\",\"dur\":4241.26,"
                            "\"ph\":\"X\",\"args\":{\"Model Id\":4294967295,\"Task Type\":\"PCIE_DMA_SQE\",\"Physic "
                            "Stream Id\":1,\"Task Id\":0,\"Batch Id\":0,\"Subtask Id\":4294967295,\"connection_id\":"
                            "181}},{\"name\":\"HostToDevice777389080576\",\"pid\":10328480,\"tid\":1,\"ph\":\"f\",\""
                            "cat\":\"HostToDevice\",\"id\":\"777389080576\",\"ts\":\"1717575960208021.758\",\"bp\":\""
                            "e\"},{\"name\":\"MEMCPY_ASYNC\",\"pid\":10328480,\"tid\":1,\"ts\":\"1717575960213021.758"
                            "\",\"dur\":1202.5,\"ph\":\"X\",\"args\":{\"Model Id\":4294967295,\"Task Type\":"
                            "\"PCIE_DMA_SQE\",\"Physic Stream Id\":1,\"Task Id\":1,\"Batch Id\":0,\"Subtask Id\":"
                            "4294967295,\"connection_id\":181}},{\"name\":\"HostToDevice777389080576\",\"pid\":"
                            "10328480,\"tid\":1,\"ph\":\"f\",\"cat\":\"HostToDevice\",\"id\":\"777389080576\",\"ts\":"
                            "\"1717575960213021.758\",\"bp\":\"e\"},{\"name\":\"MEMCPY_ASYNC\",\"pid\":10328480,\"tid"
                            "\":1,\"ts\":\"1717575960215021.758\",\"dur\":3036.9,\"ph\":\"X\",\"args\":{\"Model Id\":"
                            "4294967295,\"Task Type\":\"PCIE_DMA_SQE\",\"Physic Stream Id\":1,\"Task Id\":2,\"Batch Id"
                            "\":0,\"Subtask Id\":4294967295,\"connection_id\":189}},{\"name\":\"HostToDevice811748"
                            "818944\",\"pid\":10328480,\"tid\":1,\"ph\":\"f\",\"cat\":\"HostToDevice\",\"id\":\""
                            "811748818944\",\"ts\":\"1717575960215021.758\",\"bp\":\"e\"},{\"name\":\""
                            "WaitExecute\",\"pid\":10328480,\"tid\":1,\"ts\":\"1717575960208020.758\",\"dur\":0.0,"
                            "\"ph\":\"X\",\"args\":{\"Physic Stream Id\":1,\"Task Id\":15}},{\"name\":\"process_name\""
                            ",\"pid\":10328480,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"Ascend Hardware\"}},{\"name"
                            "\":\"process_labels\",\"pid\":10328480,\"tid\":0,\"ph\":\"M\",\"args\":{\"labels\":\"NPU"
                            "\"}},{\"name\":\"process_sort_index\",\"pid\":10328480,\"tid\":0,\"ph\":\"M\",\"args\":{"
                            "\"sort_index\":13}},{\"name\":\"thread_name\",\"pid\":10328480,\"tid\":1,\"ph\":\"M\",\""
                            "args\":{\"name\":\"Stream 1\"}},{\"name\":\"thread_sort_index\",\"pid\":10328480,\"tid\":"
                            "1,\"ph\":\"M\",\"args\":{\"sort_index\":1}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(AscendHardwareAssemblerUTest, ShouldReturnTrueWhenDataAssembleWithLogicStreamSuccess)
{
    AscendHardwareAssembler assembler;
    using LogicStreamFormat = std::unordered_map<uint32_t, uint32_t>;
    std::shared_ptr<std::vector<AscendTaskData>> taskS;
    std::shared_ptr<std::vector<TaskInfoData>> infoS;
    std::shared_ptr<std::vector<KfcTurnData>> kfcDatas;
    std::shared_ptr<std::vector<ApiData>> apiS;
    std::shared_ptr<std::vector<MemcpyInfoData>> memcpyS;
    std::shared_ptr<LogicStreamFormat> logicStreamS;
    auto api = GenerateApiData();
    auto task = GenerateTaskData();
    auto info = GenerateTaskInfoData();
    auto kfcData = GenerateKfcTurnData();
    auto memcpyInfoData = GenerateMemcpyInfoData();
    LogicStreamFormat logicStream = {{1, 1337}};
    MAKE_SHARED_NO_OPERATION(taskS, std::vector<AscendTaskData>, task);
    MAKE_SHARED_NO_OPERATION(infoS, std::vector<TaskInfoData>, info);
    MAKE_SHARED_NO_OPERATION(kfcDatas, std::vector<KfcTurnData>, kfcData);
    MAKE_SHARED_NO_OPERATION(apiS, std::vector<ApiData>, api);
    MAKE_SHARED_NO_OPERATION(memcpyS, std::vector<MemcpyInfoData>, memcpyInfoData);
    MAKE_SHARED_NO_OPERATION(logicStreamS, LogicStreamFormat, logicStream);
    dataInventory_.Inject(taskS);
    dataInventory_.Inject(infoS);
    dataInventory_.Inject(kfcDatas);
    dataInventory_.Inject(apiS);
    dataInventory_.Inject(memcpyS);
    dataInventory_.Inject(logicStreamS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10086)); // pid 10086
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expStr = "{\"name\":\"MatMulV3\",\"pid\":10328480,\"tid\":1337,\"ts\":\"1717575960208020.758\","
                         "\"dur\":0.45077999999999996,\"ph\":\"X\",\"args\":{\"Model Id\":4294967295,"
                         "\"Task Type\":\"AI_CORE\",\"Physic Stream Id\":1,\"Task Id\":10,\"Batch Id\":1,"
                         "\"Subtask Id\":1,\"connection_id\":2345}},{\"name\":\"HostToDevice10071698309120\","
                         "\"pid\":10328480,\"tid\":1337,\"ph\":\"f\",\"cat\":\"HostToDevice\","
                         "\"id\":\"10071698309120\",\"ts\":\"1717575960208020.758\",\"bp\":\"e\"},"
                         "{\"name\":\"EVENT_RECORD\",\"pid\":10328480,\"tid\":1337,\"ts\":\"1717575960208020.758\","
                         "\"dur\":0.45077999999999996,\"ph\":\"X\",\"args\":{\"Model Id\":4294967295,"
                         "\"Task Type\":\"EVENT_RECORD_SQE\",\"Physic Stream Id\":1,\"Task Id\":11,\"Batch Id\":1,"
                         "\"Subtask Id\":4294967295,\"connection_id\":1234}},{\"name\":\"HostToDevice5299989643264\","
                         "\"pid\":10328480,\"tid\":1337,\"ph\":\"f\",\"cat\":\"HostToDevice\","
                         "\"id\":\"5299989643264\",\"ts\":\"1717575960208020.758\",\"bp\":\"e\"},"
                         "{\"name\":\"MEMCPY_ASYNC\",\"pid\":10328480,\"tid\":1337,\"ts\":\"1717575960208021.758\","
                         "\"dur\":4241.26,\"ph\":\"X\",\"args\":{\"Model Id\":4294967295,"
                         "\"Task Type\":\"PCIE_DMA_SQE\",\"Physic Stream Id\":1,\"Task Id\":0,\"Batch Id\":0,"
                         "\"Subtask Id\":4294967295,\"connection_id\":181,\"size(B)\":67108864,"
                         "\"bandwidth(GB/s)\":15.822860187774388,\"operation\":\"host to device\"}},"
                         "{\"name\":\"HostToDevice777389080576\",\"pid\":10328480,\"tid\":1337,\"ph\":\"f\","
                         "\"cat\":\"HostToDevice\",\"id\":\"777389080576\",\"ts\":\"1717575960208021.758\","
                         "\"bp\":\"e\"},{\"name\":\"MEMCPY_ASYNC\",\"pid\":10328480,\"tid\":1337,"
                         "\"ts\":\"1717575960213021.758\",\"dur\":1202.5,\"ph\":\"X\","
                         "\"args\":{\"Model Id\":4294967295,\"Task Type\":\"PCIE_DMA_SQE\","
                         "\"Physic Stream Id\":1,\"Task Id\":1,\"Batch Id\":0,\"Subtask Id\":4294967295,"
                         "\"connection_id\":181,\"size(B)\":16777216,\"bandwidth(GB/s)\":13.951946777546777,"
                         "\"operation\":\"host to device\"}},{\"name\":\"HostToDevice777389080576\","
                         "\"pid\":10328480,\"tid\":1337,\"ph\":\"f\",\"cat\":\"HostToDevice\","
                         "\"id\":\"777389080576\",\"ts\":\"1717575960213021.758\",\"bp\":\"e\"},"
                         "{\"name\":\"MEMCPY_ASYNC\",\"pid\":10328480,\"tid\":1337,\"ts\":\"1717575960215021.758\","
                         "\"dur\":3036.9,\"ph\":\"X\",\"args\":{\"Model Id\":4294967295,"
                         "\"Task Type\":\"PCIE_DMA_SQE\",\"Physic Stream Id\":1,\"Task Id\":2,\"Batch Id\":0,"
                         "\"Subtask Id\":4294967295,\"connection_id\":189,\"size(B)\":35651584,"
                         "\"bandwidth(GB/s)\":11.739465902729757,\"operation\":\"host to device\"}},"
                         "{\"name\":\"HostToDevice811748818944\",\"pid\":10328480,\"tid\":1337,\"ph\":\"f\","
                         "\"cat\":\"HostToDevice\",\"id\":\"811748818944\","
                         "\"ts\":\"1717575960215021.758\",\"bp\":\"e\"},{\"name\":\"WaitExecute\","
                         "\"pid\":10328480,\"tid\":1337,\"ts\":\"1717575960208020.758\",\"dur\":0.0,\"ph\":\"X\","
                         "\"args\":{\"Physic Stream Id\":1,\"Task Id\":15}},{\"name\":\"process_name\","
                         "\"pid\":10328480,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"Ascend Hardware\"}},"
                         "{\"name\":\"process_labels\",\"pid\":10328480,\"tid\":0,\"ph\":\"M\","
                         "\"args\":{\"labels\":\"NPU\"}},{\"name\":\"process_sort_index\",\"pid\":10328480,\"tid\":0,"
                         "\"ph\":\"M\",\"args\":{\"sort_index\":13}},{\"name\":\"thread_name\",\"pid\":10328480,"
                         "\"tid\":1337,\"ph\":\"M\",\"args\":{\"name\":\"Stream 1337\"}},"
                         "{\"name\":\"thread_sort_index\",\"pid\":10328480,\"tid\":1337,\"ph\":\"M\","
                         "\"args\":{\"sort_index\":1337}},";
    EXPECT_EQ(expStr, res.back());
}