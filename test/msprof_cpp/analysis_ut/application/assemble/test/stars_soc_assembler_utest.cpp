/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : stars_soc_assembler_utest.cpp
 * Description        : stars_soc_assembler UT
 * Author             : msprof team
 * Creation Date      : 2024/8/31
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/application/timeline/stars_soc_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/soc_bandwidth_data.h"
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
const std::string BASE_PATH = "./stars_soc_assembler_utest";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string DEVICE_PATH = File::PathJoin({PROF_PATH, "device_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, OUTPUT_PATH});
}

class StarsSocAssemblerUTest : public testing::Test {
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
DataInventory StarsSocAssemblerUTest::dataInventory_;

static std::vector<SocBandwidthData> GenerateSocBandwidthData()
{
    std::vector<SocBandwidthData> res;
    SocBandwidthData data;
    data.deviceId = 0; // device 0
    data.timestamp = 1724405892226599429; // 本地时间 1724405892226599429
    data.l2_buffer_bw_level = 7; // l2_buffer_bw_level 7
    data.mata_bw_level = 4; // mata_bw_level 4
    res.push_back(data);
    data.timestamp = 1724405892226699429; // 本地时间 1724405892226699429
    data.l2_buffer_bw_level = 4; // l2_buffer_bw_level 4
    data.mata_bw_level = 3; // mata_bw_level 3
    res.push_back(data);
    data.timestamp = 1724405892226799429; // 本地时间 1724405892226799429
    data.l2_buffer_bw_level = 5; // l2_buffer_bw_level 5
    data.mata_bw_level = 1; // mata_bw_level 1
    res.push_back(data);
    return res;
}

TEST_F(StarsSocAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    StarsSocAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(StarsSocAssemblerUTest, ShouldReturnTrueWhenDataAssembleSuccess)
{
    StarsSocAssembler assembler;
    std::shared_ptr<std::vector<SocBandwidthData>> dataS;
    auto data = GenerateSocBandwidthData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<SocBandwidthData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"process_name\",\"pid\":2383960832,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":"
                            "\"Stars Soc Info\"}},{\"name\":\"process_labels\",\"pid\":2383960832,\"tid\":0,\"ph\":\"M"
                            "\",\"args\":{\"labels\":\"NPU\"}},{\"name\":\"process_sort_index\",\"pid\":2383960832,\""
                            "tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":24}},{\"name\":\"L2 Buffer Bw Level\",\"pid"
                            "\":2383960832,\"tid\":0,\"ts\":\"1724405892226599.429\",\"ph\":\"C\",\"args\":{\""
                            "L2 Buffer Bw Level\":7}},{\"name\":\"Mata Bw Level\",\"pid\":2383960832,\"tid\":0,\"ts\":"
                            "\"1724405892226599.429\",\"ph\":\"C\",\"args\":{\"Mata Bw Level\":4}},{\"name\":\""
                            "L2 Buffer Bw Level\",\"pid\":2383960832,\"tid\":0,\"ts\":\"1724405892226699.429\",\"ph"
                            "\":\"C\",\"args\":{\"L2 Buffer Bw Level\":4}},{\"name\":\"Mata Bw Level\",\"pid\":"
                            "2383960832,\"tid\":0,\"ts\":\"1724405892226699.429\",\"ph\":\"C\",\"args\":{\""
                            "Mata Bw Level\":3}},{\"name\":\"L2 Buffer Bw Level\",\"pid\":2383960832,\"tid\":0,\"ts\":"
                            "\"1724405892226799.429\",\"ph\":\"C\",\"args\":{\"L2 Buffer Bw Level\":5}},{\"name\":"
                            "\"Mata Bw Level\",\"pid\":2383960832,\"tid\":0,\"ts\":\"1724405892226799.429\",\"ph\":"
                            "\"C\",\"args\":{\"Mata Bw Level\":1}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(StarsSocAssemblerUTest, ShouldReturnFalseWhenDataAssembleFail)
{
    StarsSocAssembler assembler;
    std::shared_ptr<std::vector<SocBandwidthData>> dataS;
    auto data = GenerateSocBandwidthData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<SocBandwidthData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    MOCKER_CPP(&std::vector<std::shared_ptr<TraceEvent>>::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}
