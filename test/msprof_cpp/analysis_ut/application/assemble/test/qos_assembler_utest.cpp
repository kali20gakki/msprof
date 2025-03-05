/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : qos_assembler_utest.cpp
 * Description        : qos_assembler UT
 * Author             : msprof team
 * Creation Date      : 2024/8/31
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/application/timeline/qos_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/qos_data.h"
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
const std::string BASE_PATH = "./qos_assembler_utest";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string DEVICE_PATH = File::PathJoin({PROF_PATH, "device_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, OUTPUT_PATH});
}

class QosAssemblerUTest : public testing::Test {
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
DataInventory QosAssemblerUTest::dataInventory_;

static std::vector<QosData> GenerateQosData()
{
    std::vector<QosData> res;
    QosData data;
    data.deviceId = 0; // 0
    data.timestamp = 1724405892226599429; // 1724405892226599429
    data.bw1 = 10; // 10
    data.bw2 = 10; // 10
    data.bw3 = 10; // 10
    data.bw4 = 10; // 10
    data.bw5 = 10; // 10
    data.bw6 = 10; // 10
    data.bw7 = 10; // 10
    data.bw8 = 10; // 10
    data.bw9 = 10; // 10
    data.bw10 = 10; // 10
    res.push_back(data);
    data.timestamp = 1724405892227599429; // 1724405892227599429
    data.bw1 = 20; // 20
    data.bw2 = 20; // 20
    data.bw3 = 20; // 20
    data.bw4 = 20; // 20
    data.bw5 = 20; // 20
    data.bw6 = 20; // 20
    data.bw7 = 20; // 20
    data.bw8 = 20; // 20
    data.bw9 = 20; // 20
    data.bw10 = 20; // 20
    res.push_back(data);
    data.timestamp = 1724405892228599429; // 1724405892228599429
    data.bw1 = 30; // 30
    data.bw2 = 30; // 30
    data.bw3 = 30; // 30
    data.bw4 = 30; // 30
    data.bw5 = 30; // 30
    data.bw6 = 30; // 30
    data.bw7 = 30; // 30
    data.bw8 = 30; // 30
    data.bw9 = 30; // 30
    data.bw10 = 30; // 30
    res.push_back(data);
    return res;
}

TEST_F(QosAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    QosAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}

TEST_F(QosAssemblerUTest, ShouldReturnTrueWhenDataAssembleSuccess)
{
    QosAssembler assembler;
    std::shared_ptr<std::vector<QosData>> dataS;
    auto data = GenerateQosData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<QosData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    std::vector<std::string> qosEvents {"OTHERS", "DVPP"};
    MOCKER_CPP(&Context::GetQosEvents).stubs().will(returnValue(qosEvents));
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    std::string expectStr = "{\"name\":\"process_name\",\"pid\":2383960960,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":"
                            "\"QoS\"}},{\"name\":\"process_labels\",\"pid\":2383960960,\"tid\":0,\"ph\":\"M\",\"args\""
                            ":{\"labels\":\"NPU\"}},{\"name\":\"process_sort_index\",\"pid\":2383960960,\"tid\":0,\"ph"
                            "\":\"M\",\"args\":{\"sort_index\":28}},{\"name\":\"QoS OTHERS\",\"pid\":2383960960,\"tid"
                            "\":0,\"ts\":\"1724405892226599.429\",\"ph\":\"C\",\"args\":{\"value\":10}},{\"name\":"
                            "\"QoS DVPP\",\"pid\":2383960960,\"tid\":0,\"ts\":\"1724405892226599.429\",\"ph\":\"C\""
                            ",\"args\":{\"value\":10}},{\"name\":\"QoS OTHERS\",\"pid\":2383960960,\"tid\":0,\"ts\":\""
                            "1724405892227599.429\",\"ph\":\"C\",\"args\":{\"value\":20}},{\"name\":\"QoS DVPP\",\""
                            "pid\":2383960960,\"tid\":0,\"ts\":\"1724405892227599.429\",\"ph\":\"C\",\"args\":{\""
                            "value\":20}},{\"name\":\"QoS OTHERS\",\"pid\":2383960960,\"tid\":0,\"ts\":\""
                            "1724405892228599.429\",\"ph\":\"C\",\"args\":{\"value\":30}},{\"name\":\"QoS DVPP\",\""
                            "pid\":2383960960,\"tid\":0,\"ts\":\"1724405892228599.429\",\"ph\":\"C\",\"args\":"
                            "{\"value\":30}},";
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(QosAssemblerUTest, ShouldReturnFalseWhenDataAssembleFail)
{
    QosAssembler assembler;
    std::shared_ptr<std::vector<QosData>> dataS;
    auto data = GenerateQosData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<QosData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    MOCKER_CPP(&std::vector<std::shared_ptr<TraceEvent>>::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}
