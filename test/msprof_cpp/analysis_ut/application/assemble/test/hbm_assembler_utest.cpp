/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hbm_assembler_utest.cpp
 * Description        : hbm_assembler UT
 * Author             : msprof team
 * Creation Date      : 2024/8/31
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/application/timeline/hbm_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/hbm_data.h"
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
const std::string BASE_PATH = "./hbm_assembler_utest";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string DEVICE_PATH = File::PathJoin({PROF_PATH, "device_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, OUTPUT_PATH});
}

class HBMAssemblerUTest : public testing::Test {
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
DataInventory HBMAssemblerUTest::dataInventory_;

static std::vector<HbmData> GenerateHbmData()
{
    std::vector<HbmData> res;
    HbmData data;
    data.deviceId = 0; // device 0
    data.eventType = "read";
    data.bandWidth = 15.31082505328486; // 带宽 15.31082505328486
    data.localTime = 1724405892226599429; // 本地时间 1724405892226599429
    res.push_back(data);
    return res;
}

TEST_F(HBMAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    HBMAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(HBMAssemblerUTest, ShouldReturnTrueWhenDataAssembleSuccess)
{
    HBMAssembler assembler;
    std::shared_ptr<std::vector<HbmData>> dataS;
    auto data = GenerateHbmData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<HbmData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"process_name\",\"pid\":2383960800,\"tid\":0,\"ph\":\"M\",\"args\":"
                            "{\"name\":\"NPU MEM\"}},{\"name\":\"process_labels\",\"pid\":2383960800,\"tid"
                            "\":0,\"ph\":\"M\",\"args\":{\"labels\":\"NPU\"}},{\"name\":\"process_sort_index"
                            "\",\"pid\":2383960800,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":23}},"
                            "{\"name\":\"HBM 255/Read\",\"pid\":2383960800,\"tid\":0,\"ts\":\""
                            "1724405892226599.500000\",\"ph\":\"C\",\"args\":{\"Read(MB/s)\":15}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(HBMAssemblerUTest, ShouldReturnFalseWhenDataAssembleFail)
{
    HBMAssembler assembler;
    std::shared_ptr<std::vector<HbmData>> dataS;
    auto data = GenerateHbmData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<HbmData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    MOCKER_CPP(&std::vector<std::shared_ptr<TraceEvent>>::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}
