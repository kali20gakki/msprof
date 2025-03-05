/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : host_usage_processor_utest.cpp
 * Description        : HostUsageProcessor UT
 * Author             : msprof team
 * Creation Date      : 2024/08/21
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/data_process/system/host_usage_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Domain;
using namespace Domain::Environment;
using namespace Analysis::Utils;
using namespace Analysis::Viewer::Database;
namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./usage";
const std::string PROF_PATH_A = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string CPU_DB_NAME = "host_cpu_usage.db";
const std::string CPU_TABLE_NAME = "CpuUsage";
const std::string MEM_DB_NAME = "host_mem_usage.db";
const std::string MEM_TABLE_NAME = "MemUsage";
const std::string DISK_DB_NAME = "host_disk_usage.db";
const std::string DISK_TABLE_NAME = "DiskUsage";
const std::string NETWORK_DB_NAME = "host_network_usage.db";
const std::string NETWORK_TABLE_NAME = "NetworkUsage";
}
using CpuUsageDataType = std::vector<std::tuple<uint64_t, uint64_t, std::string, double>>;
CpuUsageDataType cpuUsage{{3758215093862910, 3758215114581640, "50", 0},
                          {3758215093862910, 3758215114581640, "Avg", 0},
                          {3758215114581640, 3758215135288460, "54", 50}};
using MemUsageDataType = std::vector<std::tuple<uint64_t, uint64_t, double>>;
MemUsageDataType memUsage{{3758215115714500, 3758215406847370, 0.144066},
                          {3758215406847370, 3758215712894170, 0.144523}};
using DiskUsageDataType = std::vector<std::tuple<uint64_t, uint64_t, double, double, std::string, double>>;
DiskUsageDataType diskUsage{{35675969243521, 35675989243521, 0, 0, "0.00", 0},
                            {35676649243521, 35676669243521, 1000.23, 10.56, "0.00", 0.98}};
using NetWorkUsageDataType = std::vector<std::tuple<uint64_t, uint64_t, double, double>>;
NetWorkUsageDataType networkUsage{{35675490307527, 35675511080960, 0.002465, 3.00865051962984},
                                  {35675531284823, 35675552244520, 0, 0},
                                  {35675697968700, 35675718727140, 0.002466, 3.01082354863434}};

class HostUsageProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, HOST})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, HOST, SQLITE})));
        EXPECT_TRUE(CreateCpuUsageData(File::PathJoin({PROF_PATH_A, HOST, SQLITE, CPU_DB_NAME}), cpuUsage));
        EXPECT_TRUE(CreateMemUsageData(File::PathJoin({PROF_PATH_A, HOST, SQLITE, MEM_DB_NAME}), memUsage));
        EXPECT_TRUE(CreateDiskUsageData(File::PathJoin({PROF_PATH_A, HOST, SQLITE, DISK_DB_NAME}), diskUsage));
        EXPECT_TRUE(CreateNetWorkUsageData(File::PathJoin({PROF_PATH_A, HOST, SQLITE, NETWORK_DB_NAME}),
                                           networkUsage));
    }
    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
    }
    virtual void SetUp()
    {
        nlohmann::json record = {
            {"startCollectionTimeBegin", "1701069324370978"},
            {"endCollectionTimeEnd", "1701069338159976"},
            {"startClockMonotonicRaw", "30001129942580"},
            {"pid", "10"},
            {"hostCntvct", "65177261204177"},
            {"CPU", {{{"Frequency", "100.000000"}}}},
            {"hostMonotonic", "651599377155020"},
        };
        MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    }
    virtual void TearDown()
    {
        GlobalMockObject::verify();
    }
    static bool CreateCpuUsageData(const std::string &dbPath, CpuUsageDataType &data)
    {
        std::shared_ptr<HostCpuUsage> database;
        MAKE_SHARED_RETURN_VALUE(database, HostCpuUsage, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, dbPath);
        auto cols = database->GetTableCols(CPU_TABLE_NAME);
        dbRunner->CreateTable(CPU_TABLE_NAME, cols);
        dbRunner->InsertData(CPU_TABLE_NAME, data);
        return true;
    }
    static bool CreateMemUsageData(const std::string &dbPath, MemUsageDataType &data)
    {
        std::shared_ptr<HostMemUsage> database;
        MAKE_SHARED_RETURN_VALUE(database, HostMemUsage, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, dbPath);
        auto cols = database->GetTableCols(MEM_TABLE_NAME);
        dbRunner->CreateTable(MEM_TABLE_NAME, cols);
        dbRunner->InsertData(MEM_TABLE_NAME, data);
        return true;
    }
    static bool CreateDiskUsageData(const std::string &dbPath, DiskUsageDataType &data)
    {
        std::shared_ptr<HostDiskUsage> database;
        MAKE_SHARED_RETURN_VALUE(database, HostDiskUsage, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, dbPath);
        auto cols = database->GetTableCols(DISK_TABLE_NAME);
        dbRunner->CreateTable(DISK_TABLE_NAME, cols);
        dbRunner->InsertData(DISK_TABLE_NAME, data);
        return true;
    }
    static bool CreateNetWorkUsageData(const std::string &dbPath, NetWorkUsageDataType &data)
    {
        std::shared_ptr<HostNetworkUsage> database;
        MAKE_SHARED_RETURN_VALUE(database, HostNetworkUsage, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, dbPath);
        auto cols = database->GetTableCols(NETWORK_TABLE_NAME);
        dbRunner->CreateTable(NETWORK_TABLE_NAME, cols);
        dbRunner->InsertData(NETWORK_TABLE_NAME, data);
        return true;
    }
};

TEST_F(HostUsageProcessorUTest, ShouldReturnOKWhenCpuUsageProcessRunSuccess)
{
    DataInventory dataInventory;
    auto processor = HostCpuUsageProcessor(PROF_PATH_A);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_CPU_USAGE));
    auto res = dataInventory.GetPtr<std::vector<CpuUsageData>>();
    EXPECT_EQ(3ul, res->size());
}

TEST_F(HostUsageProcessorUTest, ShouldReturnFalseWhenCpuUsageProcessReserveException)
{
    DataInventory dataInventory;
    auto processor = HostCpuUsageProcessor(PROF_PATH_A);
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&std::vector<CpuUsageData>::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_CPU_USAGE));
}

TEST_F(HostUsageProcessorUTest, ShouldReturnFalseWhenCpuUsageProcessCheckFailed)
{
    DataInventory dataInventory;
    auto processor = HostCpuUsageProcessor(PROF_PATH_A);
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_CPU_USAGE));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();

    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&DBRunner::CheckTableExists).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_CPU_USAGE));
}

TEST_F(HostUsageProcessorUTest, ShouldReturnTrueWhenCpuUsageProcessPathNotExists)
{
    DataInventory dataInventory;
    auto processor = HostCpuUsageProcessor(PROF_PATH_A);
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Utils::File::Exist).stubs().will(returnValue(false));
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_CPU_USAGE));
    EXPECT_FALSE(dataInventory.GetPtr<std::vector<CpuUsageData>>());
}

TEST_F(HostUsageProcessorUTest, ShouldReturnOKWhenMemUsageProcessRunSuccess)
{
    DataInventory dataInventory;
    auto processor = HostMemUsageProcessor(PROF_PATH_A);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_MEM_USAGE));
    auto res = dataInventory.GetPtr<std::vector<MemUsageData>>();
    EXPECT_EQ(2ul, res->size());
}

TEST_F(HostUsageProcessorUTest, ShouldReturnFalseWhenMemUsageProcessReserveException)
{
    DataInventory dataInventory;
    auto processor = HostMemUsageProcessor(PROF_PATH_A);
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&std::vector<MemUsageData>::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_MEM_USAGE));
}

TEST_F(HostUsageProcessorUTest, ShouldReturnOKWhenDiskUsageProcessRunSuccess)
{
    DataInventory dataInventory;
    auto processor = HostDiskUsageProcessor(PROF_PATH_A);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_DISK_USAGE));
    auto res = dataInventory.GetPtr<std::vector<DiskUsageData>>();
    std::cout << "res size is " << res->size() << std::endl;
    EXPECT_EQ(2ul, res->size());
}

TEST_F(HostUsageProcessorUTest, ShouldReturnFalseWhenDiskUsageProcessReserveException)
{
    DataInventory dataInventory;
    auto processor = HostDiskUsageProcessor(PROF_PATH_A);
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&std::vector<DiskUsageData>::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_DISK_USAGE));
}

TEST_F(HostUsageProcessorUTest, ShouldReturnOKWhenNetworkUsageProcessRunSuccess)
{
    DataInventory dataInventory;
    auto processor = HostNetworkUsageProcessor(PROF_PATH_A);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_NETWORK_USAGE));
    auto res = dataInventory.GetPtr<std::vector<NetWorkUsageData>>();
    EXPECT_EQ(3ul, res->size());
}

TEST_F(HostUsageProcessorUTest, ShouldReturnFalseWhenNetworkUsageProcessReserveException)
{
    DataInventory dataInventory;
    auto processor = HostNetworkUsageProcessor(PROF_PATH_A);
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&std::vector<NetWorkUsageData>::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NETWORK_USAGE));
}