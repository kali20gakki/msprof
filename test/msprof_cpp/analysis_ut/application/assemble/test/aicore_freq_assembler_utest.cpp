/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : aicore_freq_assembler_utest.cpp
 * Description        : aicore_freq_assembler UT
 * Author             : msprof team
 * Creation Date      : 2024/9/3
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/application/timeline/aicore_freq_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/aicore_freq_data.h"
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
const std::string BASE_PATH = "./aicore_freq_assembler_utest";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string DEVICE_PATH = File::PathJoin({PROF_PATH, "device_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, OUTPUT_PATH});
}

class AicoreFreqAssemblerUTest : public testing::Test {
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
DataInventory AicoreFreqAssemblerUTest::dataInventory_;

static std::vector<AicoreFreqData> GenerateFreqData()
{
    std::vector<AicoreFreqData> res;
    AicoreFreqData data;
    data.deviceId = 0; // device 0
    data.timestamp = 1719621074669030430; // timestamp 1719621074669030430
    data.freq = 800; // freq 800 MHz
    res.push_back(data);

    data.deviceId = 0; // device 0
    data.timestamp = 1719621074688865380; // timestamp 1719621074688865380
    data.freq = 1850; // freq 1850 MHz
    res.push_back(data);

    data.deviceId = 0; // device 0
    data.timestamp = 1719621074688867780; // timestamp 1719621074688867780
    data.freq = 1800; // freq 1800 MHz
    res.push_back(data);

    data.deviceId = 0; // device 0
    data.timestamp = 1719621074688868780; // timestamp 1719621074688868780
    data.freq = 1800; // freq 1800 MHz
    res.push_back(data);
    return res;
}

TEST_F(AicoreFreqAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    AicoreFreqAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(AicoreFreqAssemblerUTest, ShouldReturnTrueWhenDataAssembleSuccess)
{
    AicoreFreqAssembler assembler;
    std::shared_ptr<std::vector<AicoreFreqData>> dataS;
    auto data = GenerateFreqData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<AicoreFreqData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"AI Core Freq\",\"pid\":2383960512,\"tid\":0,\"ts\":\"1719621074669030.500000"
                            "\",\"ph\":\"C\",\"args\":{\"MHz\":800}},{\"name\":\"AI Core Freq\",\"pid\":2383960512,\""
                            "tid\":0,\"ts\":\"1719621074688865.250000\",\"ph\":\"C\",\"args\":{\"MHz\":1850}},{\"name"
                            "\":\"AI Core Freq\",\"pid\":2383960512,\"tid\":0,\"ts\":\"1719621074688867.750000\",\"ph"
                            "\":\"C\",\"args\":{\"MHz\":1800}},{\"name\":\"AI Core Freq\",\"pid\":2383960512,\"tid\":"
                            "0,\"ts\":\"1719621074688868.750000\",\"ph\":\"C\",\"args\":{\"MHz\":1800}},{\"name\":\""
                            "process_name\",\"pid\":2383960512,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\""
                            "AI Core Freq\"}},{\"name\":\"process_labels\",\"pid\":2383960512,\"tid\":0,\"ph\":\"M\","
                            "\"args\":{\"labels\":\"NPU\"}},{\"name\":\"process_sort_index\",\"pid\":2383960512,\"tid"
                            "\":0,\"ph\":\"M\",\"args\":{\"sort_index\":14}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(AicoreFreqAssemblerUTest, ShouldReturnFalseWhenDataAssembleFail)
{
    AicoreFreqAssembler assembler;
    std::shared_ptr<std::vector<AicoreFreqData>> dataS;
    auto data = GenerateFreqData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<AicoreFreqData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    MOCKER_CPP(&std::vector<std::shared_ptr<TraceEvent>>::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}
