/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : cann_db_dumper_utest.cpp
 * Description        : CANNTraceDBDumper UT
 * Author             : msprof team
 * Creation Date      : 2023/11/18
 * *****************************************************************************
 */


#include <iostream>
#include <dirent.h>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include "analysis/csrc/utils/thread_pool.h"
#include "analysis/csrc/viewer/database/drafts/cann_trace_db_dumper.h"


using namespace Analysis::Utils;
using namespace Analysis::Viewer::Database::Drafts;
using namespace Analysis::Viewer::Database;
using namespace  Analysis::Entities;
using namespace Analysis::Association::Cann;
const std::string TEST_DB_FILE_PATH = "./sqlite";
const uint32_t INPUT_DATA_TYPE_POSITION = 15;
const uint32_t GROUP_NAME_POSITION = 3;
const uint32_t THREAD_ID = 1;

class CannDBDumperUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
        File::CreateDir(TEST_DB_FILE_PATH);
        File::CreateDir(TEST_DB_FILE_PATH + "/sqlite");
    }

    virtual void TearDown()
    {
        GlobalMockObject::verify();
        File::RemoveDir(TEST_DB_FILE_PATH, 0);
    }

    static void MockGetHCCLTasks()
    {
        HostTask hcclHostTask;
        auto pMiniOpDesc = std::make_shared<Analysis::Entities::HcclSmallOpDesc>(0, 0, nullptr);
        pMiniOpDesc->ctxId = 0;
        auto hcclTrace = MsprofHcclInfo{};
        hcclTrace.opType = 1;
        auto hcclInfo = MsprofAdditionalInfo();
        std::memcpy(hcclInfo.data, &hcclTrace, sizeof(hcclTrace));
        pMiniOpDesc->hcclInfo = std::make_shared<MsprofAdditionalInfo>(hcclInfo);
        auto hcclTaskPp = std::make_shared<Operator>(pMiniOpDesc, 0, Analysis::Entities::OpType::OPTYPE_COMPUTE);
        hcclHostTask.op = hcclTaskPp;
        std::shared_ptr<HostTask> hostTaskPointer = std::make_shared<HostTask>(hcclHostTask);
        auto hcclTasks = std::make_shared<HostTasks>();
        hcclTasks->push_back(hostTaskPointer);
        MOCKER_CPP(&TreeAnalyzer::GetHCCLTasks).stubs().will(returnValue(*hcclTasks));
    }

    static void MockGetComputeTasks()
    {
        auto kernelTask = std::make_shared<HostTask>();
        MsprofNodeBasicInfo msprofNodeBasicInfo{};
        auto ctx = std::make_shared<MsprofAdditionalInfo>();
        ctx->threadId = 0;
        auto tensorDesc = std::make_shared<ConcatTensorInfo>();
        tensorDesc->tensorNum = 1;
        auto kernelDesc = std::make_shared<OpDesc>();
        kernelDesc->tensorDesc = std::make_shared<ConcatTensorInfo>();
        kernelDesc->nodeDesc = std::make_shared<MsprofCompactInfo>();
        kernelDesc->nodeDesc->data.nodeBasicInfo = msprofNodeBasicInfo;
        kernelDesc->ctxId = std::make_shared<MsprofAdditionalInfo>();
        auto kernelOp = std::make_shared<Operator>(kernelDesc, 0, Analysis::Entities::OpType::OPTYPE_COMPUTE);
        kernelTask->op = kernelOp;
        std::shared_ptr<HostTask> computeTaskPointer = kernelTask;
        auto kernelTasks = std::make_shared<HostTasks>();
        kernelTasks->push_back(computeTaskPointer);
        MOCKER_CPP(&TreeAnalyzer::GetComputeTasks).stubs().will(returnValue(*kernelTasks));
    }

    static void MockGetTasks()
    {
        auto pMiniOpDesc = std::make_shared<Entities::HcclSmallOpDesc>(0, 0, nullptr);
        std::shared_ptr<Operator> pointer = std::make_shared<Operator>(pMiniOpDesc, 0,
                                                                       Entities::OpType::OPTYPE_HCCL_SMALL);
        HostTask task;
        task.op = pointer;
        std::shared_ptr<HostTask> taskPointer = std::make_shared<HostTask>(task);
        auto tasks = std::make_shared<HostTasks>();
        tasks->push_back(taskPointer);
        MOCKER_CPP(&TreeAnalyzer::GetTasks).stubs().will(returnValue(*tasks));
    }

    void MockGetHcclBigOps()
    {
        auto trace = new MsprofCompactInfo;
        auto nodeBasicInfo = new MsprofNodeBasicInfo;
        auto runtime = new MsprofRuntimeTrack;
        trace->data.nodeBasicInfo = *nodeBasicInfo;
        trace->data.runtimeTrack = *runtime;

        auto desc = std::make_shared<HcclBigOpDesc>(1, 1, 1, 1, 1, 1, 1, std::shared_ptr<MsprofCompactInfo>(trace));
        auto op = std::make_shared<Operator>(desc, 0, Entities::OpType::OPTYPE_HCCL_BIG);
        auto hcclBigOps = std::make_shared<HCCLBigOpDescs>();
        hcclBigOps->emplace_back(op);
        MOCKER_CPP(&TreeAnalyzer::GetHcclBigOps).stubs().will(returnValue(*hcclBigOps));
    }
};

TEST_F(CannDBDumperUtest,
            TestCANNDumperShouldReturnTrueWhenAllDataIsCorrectAndDBFunctionsNormalThenQueryDBShouldReturnRecords)
{
    MockGetHCCLTasks();
    MockGetComputeTasks();
    MockGetTasks();
    MockGetHcclBigOps();

    CANNTraceDBDumper cannTraceDbDumper(".");
    auto treeNode = std::make_shared<TreeNode>(nullptr);
    TreeAnalyzer treeAnalyzer(treeNode, THREAD_ID);
    bool ret = cannTraceDbDumper.DumpData(treeAnalyzer);
    EXPECT_TRUE(ret);
    HCCLDB hcclDB;
    std::string hcclTaskDBPath = Utils::File::PathJoin({TEST_DB_FILE_PATH, hcclDB.GetDBName()});
    DBRunner hcclOpDBRunner(hcclTaskDBPath);
    std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, std::string, uint32_t,
            uint32_t, std::string, std::string, uint32_t, std::string>> hcclOpData;
    hcclOpDBRunner.QueryData("select * from HCCLOP", hcclOpData);
    EXPECT_EQ(hcclOpData.size(), 1);

    RuntimeDB runtimeDB;
    std::string hostTaskDBPath = Utils::File::PathJoin({TEST_DB_FILE_PATH, runtimeDB.GetDBName()});
    DBRunner hostTaskDBRunner(hostTaskDBPath);
    std::vector<std::tuple<uint32_t,
            uint32_t, uint32_t, uint32_t, std::string, uint32_t, std::string, uint32_t,
            std::string, std::string>> hostTaskData;
    hostTaskDBRunner.QueryData("select * from HostTask", hostTaskData);
    EXPECT_EQ(hostTaskData.size(), 1);

    GEInfoDB geInfoDB;
    std::string opDescDBPath = Utils::File::PathJoin({TEST_DB_FILE_PATH, geInfoDB.GetDBName()});
    DBRunner opDescDBRunner(opDescDBPath);
    std::vector<std::tuple<uint32_t, std::string, uint32_t, uint32_t, uint32_t, uint32_t, std::string, uint32_t,
            std::string, uint32_t, uint32_t, double, uint32_t, uint32_t, std::string,
            std::string, std::string, std::string, std::string, std::string, uint32_t,
            uint32_t, uint32_t>> TaskInfoData;
    opDescDBRunner.QueryData("select * from TaskInfo", TaskInfoData);
    EXPECT_EQ(TaskInfoData.size(), 1);
    EXPECT_EQ(std::get<INPUT_DATA_TYPE_POSITION>(TaskInfoData[0]), "");

    std::vector<std::tuple<uint32_t, uint32_t, std::string, std::string, uint32_t, std::string,
            double, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
            std::string, double, std::string, std::string, uint64_t, std::string>> HCCLTaskData;
    hcclOpDBRunner.QueryData("select * from HCCLTask", HCCLTaskData);
    EXPECT_EQ(HCCLTaskData.size(), 1);
    EXPECT_EQ(std::get<GROUP_NAME_POSITION>(HCCLTaskData[0]), "0");
}


TEST_F(CannDBDumperUtest,
            TestCANNDumperShouldReturnTrueWhenAllDataIsL0AndDBFunctionsNormalThenQueryDBShouldReturnRecords)
{
    MockGetHCCLTasks();
    MockGetComputeTasks();
    MockGetTasks();
    MockGetHcclBigOps();

    CANNTraceDBDumper cannTraceDbDumper(".");
    auto treeNode = std::make_shared<TreeNode>(nullptr);
    TreeAnalyzer treeAnalyzer(treeNode, THREAD_ID);
    auto computeTasks = treeAnalyzer.GetComputeTasks();
    computeTasks[0]->op->opDesc->tensorDesc = nullptr;
    auto hcclTasks = treeAnalyzer.GetHCCLTasks();
    hcclTasks[0]->op->hcclSmallOpDesc->hcclInfo = nullptr;
    bool ret = cannTraceDbDumper.DumpData(treeAnalyzer);
    EXPECT_TRUE(ret);

    HCCLDB hcclDB;
    std::string hcclTaskDBPath = Utils::File::PathJoin({TEST_DB_FILE_PATH, hcclDB.GetDBName()});
    DBRunner hcclOpDBRunner(hcclTaskDBPath);
    std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, std::string, uint32_t,
            uint32_t, std::string, std::string, uint32_t, std::string>> hcclOpData;
    hcclOpDBRunner.QueryData("select * from HCCLOP", hcclOpData);
    EXPECT_EQ(hcclOpData.size(), 1);

    RuntimeDB runtimeDB;
    std::string hostTaskDBPath = Utils::File::PathJoin({TEST_DB_FILE_PATH, runtimeDB.GetDBName()});
    DBRunner hostTaskDBRunner(hostTaskDBPath);
    std::vector<std::tuple<uint32_t,
            uint32_t, uint32_t, uint32_t, std::string, uint32_t, std::string, uint32_t,
            std::string, std::string>> hostTaskData;
    hostTaskDBRunner.QueryData("select * from HostTask", hostTaskData);
    EXPECT_EQ(hostTaskData.size(), 1);

    GEInfoDB geInfoDB;
    std::string opDescDBPath = Utils::File::PathJoin({TEST_DB_FILE_PATH, geInfoDB.GetDBName()});
    DBRunner opDescDBRunner(opDescDBPath);
    std::vector<std::tuple<uint32_t, std::string, uint32_t, uint32_t, uint32_t, uint32_t, std::string, uint32_t,
            std::string, uint32_t, uint32_t, double, uint32_t, uint32_t, std::string,
            std::string, std::string, std::string, std::string, std::string, uint32_t,
            uint32_t, uint32_t>> TaskInfoData;
    opDescDBRunner.QueryData("select * from TaskInfo", TaskInfoData);
    EXPECT_EQ(TaskInfoData.size(), 1);
    EXPECT_EQ(std::get<INPUT_DATA_TYPE_POSITION>(TaskInfoData[0]), "N/A");

    std::vector<std::tuple<uint32_t, uint32_t, std::string, std::string, uint32_t, std::string,
            double, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
            std::string, double, std::string, std::string, uint64_t, std::string>> HCCLTaskData;
    hcclOpDBRunner.QueryData("select * from HCCLTask", HCCLTaskData);
    EXPECT_EQ(HCCLTaskData.size(), 1);
    EXPECT_EQ(std::get<GROUP_NAME_POSITION>(HCCLTaskData[0]), "N/A");
}

TEST_F(CannDBDumperUtest, TestCANNDumperShouldReturnFalseWhenInsertDataToDBFailed)
{
    MockGetHCCLTasks();
    MockGetComputeTasks();
    MockGetTasks();
    MockGetHcclBigOps();
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(true));
    CANNTraceDBDumper cannTraceDbDumper(TEST_DB_FILE_PATH);
    auto treeNode = std::make_shared<TreeNode>(nullptr);
    TreeAnalyzer treeAnalyzer(treeNode, THREAD_ID);
    bool ret = cannTraceDbDumper.DumpData(treeAnalyzer);
    EXPECT_FALSE(ret);
}


TEST_F(CannDBDumperUtest, TestCANNDumperShouldRetrunTrueWhenDataIsEmpty)
{
    MOCKER_CPP(&TreeAnalyzer::GetHCCLTasks).stubs().will(returnValue(*std::make_shared<HostTasks>()));
    MOCKER_CPP(&TreeAnalyzer::GetComputeTasks).stubs().will(returnValue(*std::make_shared<HostTasks>()));
    MOCKER_CPP(&TreeAnalyzer::GetTasks).stubs().will(returnValue(*std::make_shared<HostTasks>()));
    MOCKER_CPP(&TreeAnalyzer::GetHcclBigOps).stubs().will(returnValue(*std::make_shared<HCCLBigOpDescs>()));
    CANNTraceDBDumper cannTraceDbDumper(TEST_DB_FILE_PATH);
    auto treeNode = std::make_shared<TreeNode>(nullptr);
    TreeAnalyzer treeAnalyzer(treeNode, THREAD_ID);
    bool ret = cannTraceDbDumper.DumpData(treeAnalyzer);
    EXPECT_TRUE(ret);
}

TEST_F(CannDBDumperUtest, TestCANNDumperShouldReturnFalseWhenCreateTableFailed)
{
    MockGetHCCLTasks();
    MockGetComputeTasks();
    MockGetTasks();
    MockGetHcclBigOps();
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(false));
    CANNTraceDBDumper cannTraceDbDumper(TEST_DB_FILE_PATH);
    auto treeNode = std::make_shared<TreeNode>(nullptr);
    TreeAnalyzer treeAnalyzer(treeNode, THREAD_ID);
    bool ret = cannTraceDbDumper.DumpData(treeAnalyzer);
    EXPECT_FALSE(ret);
}