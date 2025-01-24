/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : chip_trans_assembler_utest.cpp
 * Description        : chip_trans_assembler UT
 * Author             : msprof team
 * Creation Date      : 2024/8/31
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/application/timeline/chip_trans_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/chip_trans_data.h"
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
const std::string BASE_PATH = "./chip_trans_assembler_utest";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string DEVICE_PATH = File::PathJoin({PROF_PATH, "device_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, OUTPUT_PATH});
}

class ChipTransAssemblerUTest : public testing::Test {
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
DataInventory ChipTransAssemblerUTest::dataInventory_;

static std::vector<PaLinkInfoData> GeneratePaLinkInfoData()
{
    std::vector<PaLinkInfoData> res;
    PaLinkInfoData data;
    data.deviceId = 0; // device 0
    data.pa_link_id = 0; // id 0
    data.pa_link_traffic_monit_rx = "5";
    data.pa_link_traffic_monit_tx = "2";
    data.local_time = 1724405892226599429; // 本地时间 1724405892226599429
    res.push_back(data);
    data.pa_link_id = 7; // id 7
    data.pa_link_traffic_monit_rx = "5";
    data.pa_link_traffic_monit_tx = "4";
    data.local_time = 1724405892226599429; // 本地时间 1724405892226599429
    res.push_back(data);
    data.pa_link_id = 0; // id 0
    data.pa_link_traffic_monit_rx = "4";
    data.pa_link_traffic_monit_tx = "4";
    data.local_time = 1724405892226699429; // 本地时间 1724405892226699429
    res.push_back(data);
    data.pa_link_id = 7; // id 7
    data.pa_link_traffic_monit_rx = "3";
    data.pa_link_traffic_monit_tx = "3";
    data.local_time = 1724405892226699429; // 本地时间 1724405892226699429
    res.push_back(data);
    return res;
}

static std::vector<PcieInfoData> GeneratePcieInfoData()
{
    std::vector<PcieInfoData> res;
    PcieInfoData data;
    data.deviceId = 0; // device 0
    data.pcie_id = 0; // id 0
    data.pcie_read_bandwidth = 3; // level 3
    data.pcie_write_bandwidth = 4; // level 4
    data.local_time = 1724405892226599429; // 本地时间 1724405892226599429
    res.push_back(data);
    data.pcie_id = 7; // id 7
    data.pcie_read_bandwidth = 2; // level 2
    data.pcie_write_bandwidth = 6; // level 6
    data.local_time = 1724405892226599429; // 本地时间 1724405892226599429
    res.push_back(data);
    data.pcie_id = 0; // id 0
    data.pcie_read_bandwidth = 1; // level 1
    data.pcie_write_bandwidth = 6; // level 6
    data.local_time = 1724405892226699429; // 本地时间 1724405892226699429
    res.push_back(data);
    data.pcie_id = 7; // id 7
    data.pcie_read_bandwidth = 5; // level 5
    data.pcie_write_bandwidth = 5; // level 5
    data.local_time = 1724405892226699429; // 本地时间 1724405892226699429
    res.push_back(data);
    return res;
}

TEST_F(ChipTransAssemblerUTest, ShouldReturnTrueWhenDataNotExists)
{
    ChipTransAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
}

std::string expectStr = "{\"name\":\"process_name\",\"pid\":2383960864,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\""
                        "Stars Chip Trans\"}},{\"name\":\"process_labels\",\"pid\":2383960864,\"tid\":0,\"ph\":\"M\","
                        "\"args\":{\"labels\":\"NPU\"}},{\"name\":\"process_sort_index\",\"pid\":2383960864,\"tid\":0"
                        ",\"ph\":\"M\",\"args\":{\"sort_index\":25}},{\"name\":\"PCIE Read Bandwidth\",\"pid\":"
                        "2383960864,\"tid\":100010,\"ts\":\"1724405892226599.429\",\"ph\":\"C\",\"args\":{\""
                        "PCIE Read Bandwidth\":3,\"PCIE ID\":0}},{\"name\":\"PCIE Write Bandwidth\",\"pid\":2383960864"
                        ",\"tid\":100010,\"ts\":\"1724405892226599.429\",\"ph\":\"C\",\"args\":{\""
                        "PCIE Write Bandwidth\":4,\"PCIE ID\":0}},{\"name\":\"PCIE Read Bandwidth\",\"pid\":2383960864"
                        ",\"tid\":100010,\"ts\":\"1724405892226599.429\",\"ph\":\"C\",\"args\":{\""
                        "PCIE Read Bandwidth\":2,\"PCIE ID\":7}},{\"name\":\"PCIE Write Bandwidth\",\"pid\":2383960864"
                        ",\"tid\":100010,\"ts\":\"1724405892226599.429\",\"ph\":\"C\",\"args\":{\""
                        "PCIE Write Bandwidth\":6,\"PCIE ID\":7}},{\"name\":\"PCIE Read Bandwidth\",\"pid\":2383960864"
                        ",\"tid\":100010,\"ts\":\"1724405892226699.429\",\"ph\":\"C\",\"args\":{\""
                        "PCIE Read Bandwidth\":1,\"PCIE ID\":0}},{\"name\":\"PCIE Write Bandwidth\",\"pid\":2383960864"
                        ",\"tid\":100010,\"ts\":\"1724405892226699.429\",\"ph\":\"C\",\"args\":{\""
                        "PCIE Write Bandwidth\":6,\"PCIE ID\":0}},{\"name\":\"PCIE Read Bandwidth\",\"pid\":2383960864"
                        ",\"tid\":100010,\"ts\":\"1724405892226699.429\",\"ph\":\"C\",\"args\":{\""
                        "PCIE Read Bandwidth\":5,\"PCIE ID\":7}},{\"name\":\"PCIE Write Bandwidth\",\"pid\":2383960864"
                        ",\"tid\":100010,\"ts\":\"1724405892226699.429\",\"ph\":\"C\",\"args\":{\""
                        "PCIE Write Bandwidth\":5,\"PCIE ID\":7}},{\"name\":\"PA Link Rx\",\"pid\":2383960864,\"tid\":"
                        "111111,\"ts\":\"1724405892226599.429\",\"ph\":\"C\",\"args\":{\"PA Link Rx\":5,\""
                        "PA Link ID\":0}},{\"name\":\"PA Link Tx\",\"pid\":2383960864,\"tid\":111111,\"ts\":\""
                        "1724405892226599.429\",\"ph\":\"C\",\"args\":{\"PA Link Tx\":2,\"PA Link ID\":0}},{\"name"
                        "\":\"PA Link Rx\",\"pid\":2383960864,\"tid\":111111,\"ts\":\"1724405892226599.429\",\"ph\""
                        ":\"C\",\"args\":{\"PA Link Rx\":5,\"PA Link ID\":7}},{\"name\":\"PA Link Tx\",\"pid\":"
                        "2383960864,\"tid\":111111,\"ts\":\"1724405892226599.429\",\"ph\":\"C\",\"args\":{\""
                        "PA Link Tx\":4,\"PA Link ID\":7}},{\"name\":\"PA Link Rx\",\"pid\":2383960864,\"tid\":111111,"
                        "\"ts\":\"1724405892226699.429\",\"ph\":\"C\",\"args\":{\"PA Link Rx\":4,\"PA Link ID\":0}}"
                        ",{\"name\":\"PA Link Tx\",\"pid\":2383960864,\"tid\":111111,\"ts\":\"1724405892226699.429"
                        "\",\"ph\":\"C\",\"args\":{\"PA Link Tx\":4,\"PA Link ID\":0}},{\"name\":\"PA Link Rx\",\"pid"
                        "\":2383960864,\"tid\":111111,\"ts\":\"1724405892226699.429\",\"ph\":\"C\",\"args\":{\""
                        "PA Link Rx\":3,\"PA Link ID\":7}},{\"name\":\"PA Link Tx\",\"pid\":2383960864,\"tid\":111111,"
                        "\"ts\":\"1724405892226699.429\",\"ph\":\"C\",\"args\":{\"PA Link Tx\":3,\""
                        "PA Link ID\":7}},";

TEST_F(ChipTransAssemblerUTest, ShouldReturnTrueWhenDataAssembleSuccess)
{
    ChipTransAssembler assembler;
    std::shared_ptr<std::vector<PaLinkInfoData>> dataS;
    auto data = GeneratePaLinkInfoData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<PaLinkInfoData>, data);
    dataInventory_.Inject(dataS);
    std::shared_ptr<std::vector<PcieInfoData>> dataS1;
    auto data1 = GeneratePcieInfoData();
    MAKE_SHARED_NO_OPERATION(dataS1, std::vector<PcieInfoData>, data1);
    dataInventory_.Inject(dataS1);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    EXPECT_TRUE(assembler.Run(dataInventory_, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_EQ(expectStr, res.back());
}

TEST_F(ChipTransAssemblerUTest, ShouldReturnFalseWhenDataAssembleFail)
{
    ChipTransAssembler assembler;
    std::shared_ptr<std::vector<PaLinkInfoData>> dataS;
    auto data = GeneratePaLinkInfoData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<PaLinkInfoData>, data);
    dataInventory_.Inject(dataS);
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(2328086)); // pid 2328086
    MOCKER_CPP(&std::vector<std::shared_ptr<TraceEvent>>::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory_, PROF_PATH));
}
