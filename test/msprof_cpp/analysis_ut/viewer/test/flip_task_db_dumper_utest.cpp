/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : flip_task_db_dumper_utest.cpp
 * Description        : FlipTaskDBDumper UT
 * Author             : msprof team
 * Creation Date      : 2024/03/06
 * *****************************************************************************
 */
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/services/persistence/host/flip_task_db_dumper.h"

using namespace Analysis::Utils;
using namespace Analysis::Infra;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Domain;
using FlipTask = Analysis::Domain::Adapter::FlipTask;
using FlipTasks = std::vector<std::shared_ptr<FlipTask>>;
const std::string TEST_DB_FILE_PATH = "./sqlite";
const uint32_t ACL_LEVEL_NUMBER = 20000;
const uint32_t PYTORCH_LEVEL_NUMBER = 30000;
class FlipTaskDBDumperUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
        File::CreateDir(TEST_DB_FILE_PATH);
    }

    virtual void TearDown()
    {
        GlobalMockObject::verify();
        File::RemoveDir(TEST_DB_FILE_PATH, 0);
    }
};

TEST_F(FlipTaskDBDumperUtest, TestFlipTaskDBDumperWithCompletedApiEventShouldInsertDataCorrectly)
{
    FlipTaskDBDumper FlipTaskDBDumper(".");
    auto flipTask1 = std::make_shared<FlipTask>();
    flipTask1->flipNum = 1;
    auto flipTask2 = std::make_shared<FlipTask>();
    flipTask2->streamId = 1;

    FlipTasks flipTasks {flipTask1, flipTask2};

    auto res = FlipTaskDBDumper.DumpData(flipTasks);
    EXPECT_TRUE(res);

    RtsTrackDB rtsTrackDb;
    std::string runtimeDBPath = Utils::File::PathJoin({TEST_DB_FILE_PATH, rtsTrackDb.GetDBName()});
    DBRunner runtimeDBRunner(runtimeDBPath);
    std::vector<std::tuple<std::string, std::string>> data;
    runtimeDBRunner.QueryData("select * from HostTaskFlip", data);
    // 如果level是acl，则会直接把id置为0
    EXPECT_EQ(data.size(), flipTasks.size());
}


TEST_F(FlipTaskDBDumperUtest, TestFlipTaskDBDumperShouldReturnFalseWhenDBNotCreated)
{
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(false));
    FlipTaskDBDumper FlipTaskDBDumper(".");
    auto flipTask1 = std::make_shared<FlipTask>();
    flipTask1->flipNum = 1;
    auto flipTask2 = std::make_shared<FlipTask>();
    flipTask2->streamId = 1;

    FlipTasks flipTasks {flipTask1, flipTask2};
    auto res = FlipTaskDBDumper.DumpData(flipTasks);
    ASSERT_FALSE(res);
}

TEST_F(FlipTaskDBDumperUtest, TestFlipTaskDBDumperShouldReturnFalseWhenCannotInsertTable)
{
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(true));
    FlipTaskDBDumper FlipTaskDBDumper(".");
    auto flipTask1 = std::make_shared<FlipTask>();
    flipTask1->flipNum = 1;
    auto flipTask2 = std::make_shared<FlipTask>();
    flipTask2->streamId = 1;

    FlipTasks flipTasks {flipTask1, flipTask2};
    auto res = FlipTaskDBDumper.DumpData(flipTasks);
    ASSERT_FALSE(res);
}