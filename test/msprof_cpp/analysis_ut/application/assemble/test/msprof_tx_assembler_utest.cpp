/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : msprof_tx_assembler_utest.cpp
 * Description        : msprof_tx_assembler UT
 * Author             : msprof team
 * Creation Date      : 2024/8/31
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/application/timeline/msprof_tx_assembler.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/msprof_tx_host_data.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

using namespace Analysis::Application;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Domain::Environment;

namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./tx_test";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, OUTPUT_PATH});
}

class MsprofTxAssemblerUTest : public testing::Test {
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

static std::vector<MsprofTxHostData> GenerateTxAndExData()
{
    std::vector<MsprofTxHostData> res;
    MsprofTxHostData txData;
    txData.eventType = 0; // eventType 0
    txData.pid = 19988776;  // pid 19988776
    txData.tid = 1024; // tid 1024
    txData.category = 23438457; // category 23438457
    txData.payloadType = 0; // payloadType 0
    txData.messageType = 1; // messageType 1
    txData.payloadValue = 283728437; // payloadValue 283728437
    txData.start = 1717575960208020758; // start 1717575960208020758
    txData.end = 1717575960208026869; // start 1717575960208026869
    txData.message = "tx";
    res.push_back(txData);
    MsprofTxHostData exData;
    exData.eventType = 0; // eventType 0
    exData.pid = 19988776;  // pid 19988776
    exData.tid = 1024; // tid 1024
    exData.start = 1717575960208126869; // start 1717575960208126869
    exData.end = 1717575960208226869; // start 1717575960208226869
    exData.connectionId = 4000000001;  // connId 4000000001
    exData.domain = "domain";
    exData.message = "ex";
    res.push_back(exData);
    return res;
}

TEST_F(MsprofTxAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    MsprofTxAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(MsprofTxAssemblerUTest, ShouldReturnTrueWhenDataAssembleSuccess)
{
    MsprofTxAssembler assembler;
    std::shared_ptr<std::vector<MsprofTxHostData>> txDataS;
    auto txData = GenerateTxAndExData();
    MAKE_SHARED_NO_OPERATION(txDataS, std::vector<MsprofTxHostData>, txData);
    dataInventory_.Inject(txDataS);
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(2ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"tx\",\"pid\":223,\"tid\":1024,\"ts\":\"1717575960208020.758\","
                            "\"dur\":6.111,\"ph\":\"X\",\"args\":{\"Category\":\"23438457\",\"Payload_type\":0,"
                            "\"Payload_value\":283728437,\"Message_type\":1,\"event_type\":\"marker\"}},"
                            "{\"name\":\"ex\",\"pid\":223,\"tid\":1024,\"ts\":\"1717575960208126.869\",\"dur\":100.0,"
                            "\"ph\":\"X\",\"args\":{\"event_type\":\"marker\",\"domain\":\"domain\"}},"
                            "{\"name\":\"MsTx_4000000001\",\"pid\":223,\"tid\":1024,\"ph\":\"s\",\"cat\":\"MsTx\","
                            "\"id\":\"4000000001\",\"ts\":\"1717575960208126.869\"},{\"name\":\"process_name\","
                            "\"pid\":223,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"\"}},{\"name\":\"process_labels\","
                            "\"pid\":223,\"tid\":0,\"ph\":\"M\",\"args\":{\"labels\":\"CPU\"}},"
                            "{\"name\":\"process_sort_index\",\"pid\":223,\"tid\":0,\"ph\":\"M\",\"args\":"
                            "{\"sort_index\":6}},{\"name\":\"thread_name\",\"pid\":223,\"tid\":1024,\"ph\":\"M\","
                            "\"args\":{\"name\":\"Thread 1024\"}},{\"name\":\"thread_sort_index\",\"pid\":223,"
                            "\"tid\":1024,\"ph\":\"M\",\"args\":{\"sort_index\":1024}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(MsprofTxAssemblerUTest, ShouldReturnFalseWhenDataAssembleFail)
{
    MsprofTxAssembler assembler;
    std::shared_ptr<std::vector<MsprofTxHostData>> txDataS;
    auto txData = GenerateTxAndExData();
    MAKE_SHARED_NO_OPERATION(txDataS, std::vector<MsprofTxHostData>, txData);
    dataInventory_.Inject(txDataS);
    std::string name = "main";
    MOCKER_CPP(&Context::GetPidNameFromInfoJson).stubs().will(returnValue(name));
    MOCKER_CPP(&std::vector<std::shared_ptr<TraceEvent>>::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}
