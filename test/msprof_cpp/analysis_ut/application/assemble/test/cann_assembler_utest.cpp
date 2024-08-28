/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : cann_assembler_utest.cpp
 * Description        : cann_assembler UT
 * Author             : msprof team
 * Creation Date      : 2024/8/26
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/application/timeline/cann_assembler.h"
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
const std::string BASE_PATH = "./cann_test";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, OUTPUT_PATH});
}

class CannAssemblerUTest : public testing::Test {
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

static std::vector<ApiData> GenerateApiData()
{
    std::vector<ApiData> res;
    ApiData data;
    data.apiName = "launch";
    data.connectionId = 2762; // connectionId 2762
    data.id = "0";
    data.start = 1717575960208020750; // start 1717575960208020750
    data.itemId = "hcom_broadcast_";
    data.level = MSPROF_REPORT_NODE_LEVEL;
    data.threadId = 87144; // threadId 87144
    data.end = 1717575960209010750; // end 1717575960209010750
    data.structType = "launch";
    res.push_back(data);
    return res;
}

TEST_F(CannAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    CannAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(CannAssemblerUTest, ShouldReturnTrueWhenDataAssembleSuccess)
{
    CannAssembler assembler;
    std::shared_ptr<std::vector<ApiData>> dataS;
    auto data = GenerateApiData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<ApiData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(1000)); // pid 1000
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"process_name\",\"pid\":1024255,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":"
                            "\"CANN\"}},{\"name\":\"process_labels\",\"pid\":1024255,\"tid\":0,\"ph\":\"M\",\"args\":{"
                            "\"labels\":\"CPU\"}},{\"name\":\"process_sort_index\",\"pid\":1024255,\"tid\":0,\"ph\":"
                            "\"M\",\"args\":{\"sort_index\":7}},{\"name\":\"thread_name\",\"pid\":1024255,\"tid\":"
                            "87144,\"ph\":\"M\",\"args\":{\"name\":\"Thread 87144\"}},{\"name\":\"thread_sort_index\","
                            "\"pid\":1024255,\"tid\":87144,\"ph\":\"M\",\"args\":{\"sort_index\":87144}},{\"name\":"
                            "\"Node@launch\",\"pid\":1024255,\"tid\":87144,\"ts\":\"1717575960208020.750000\",\"dur\":"
                            "990.0,\"ph\":\"X\",\"args\":{\"Thread Id\":87144,\"Mode\":\"launch\",\"level\":\"node\","
                            "\"id\":\"0\",\"item_id\":\"hcom_broadcast_\",\"connection_id\":2762}},{\"name\":"
                            "\"HostToDevice11862699671552\",\"pid\":1024255,\"tid\":87144,\"ph\":\"S\",\"cat\":"
                            "\"HostToDevice\",\"id\":\"11862699671552\",\"ts\":\"1717575960208020.750000\"},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(CannAssemblerUTest, ShouldReturnFalseWhenDataAssembleFail)
{
    CannAssembler assembler;
    std::shared_ptr<std::vector<ApiData>> dataS;
    auto data = GenerateApiData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<ApiData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(1000)); // pid 1000
    MOCKER_CPP(&std::vector<std::shared_ptr<TraceEvent>>::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}
