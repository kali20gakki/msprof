/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : npu_mem_assembler_utest.cpp
 * Description        : npu_mem_assembler UT
 * Author             : msprof team
 * Creation Date      : 2024/8/31
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/application/timeline/npu_mem_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_mem_data.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

using namespace Analysis::Application;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Domain::Environment;

namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./npu_mem_assembler_utest";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string DEVICE_PATH = File::PathJoin({PROF_PATH, "device_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, OUTPUT_PATH});
}

class NpuMemAssemblerUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(DEVICE_PATH));
        EXPECT_TRUE(File::CreateDir(RESULT_PATH));
    }
    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
        dataInventory_.RemoveRestData({});
        GlobalMockObject::verify();
    }
    virtual void SetUp()
    {
        GlobalMockObject::verify();
    }
protected:
    static DataInventory dataInventory_;
};
DataInventory NpuMemAssemblerUTest::dataInventory_;

static std::vector<NpuMemData> GenerateNpuMemData()
{
    std::vector<NpuMemData> res;
    NpuMemData data;
    data.deviceId = 0; // device 0
    data.event = "0";
    data.hbm = 106848256; // hbm 106848256
    data.ddr = 0; // ddr 0
    data.timestamp = 1724405892226599429; // 本地时间 1724405892226599429
    res.push_back(data);
    return res;
}

TEST_F(NpuMemAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    NpuMemAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(NpuMemAssemblerUTest, ShouldReturnTrueWhenDataAssembleSuccess)
{
    NpuMemAssembler assembler;
    std::shared_ptr<std::vector<NpuMemData>> dataS;
    auto data = GenerateNpuMemData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<NpuMemData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"process_name\",\"pid\":2383960576,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":"
                            "\"NPU MEM\"}},{\"name\":\"process_labels\",\"pid\":2383960576,\"tid\":0,\"ph\":\"M\","
                            "\"args\":{\"labels\":\"NPU\"}},{\"name\":\"process_sort_index\",\"pid\":2383960576,\"tid\""
                            ":0,\"ph\":\"M\",\"args\":{\"sort_index\":16}},{\"name\":\"APP/DDR\",\"pid\":2383960576,"
                            "\"tid\":0,\"ts\":\"1724405892226599.429\",\"ph\":\"C\",\"args\":{\"KB\":0.0}},{\"name"
                            "\":\"APP/HBM\",\"pid\":2383960576,\"tid\":0,\"ts\":\"1724405892226599.429\",\"ph\""
                            ":\"C\",\"args\":{\"KB\":104344.0}},{\"name\":\"APP/Memory\",\"pid\":2383960576,\"tid\":"
                            "0,\"ts\":\"1724405892226599.429\",\"ph\":\"C\",\"args\":{\"KB\":0.0}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(NpuMemAssemblerUTest, ShouldReturnFalseWhenDataAssembleFail)
{
    NpuMemAssembler assembler;
    std::shared_ptr<std::vector<NpuMemData>> dataS;
    auto data = GenerateNpuMemData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<NpuMemData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    MOCKER_CPP(&std::vector<std::shared_ptr<TraceEvent>>::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}
