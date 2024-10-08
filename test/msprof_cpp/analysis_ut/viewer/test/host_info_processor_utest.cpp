/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : host_info_processor.cpp
 * Description        : HostInfoProcessor UT
 * Author             : msprof team
 * Creation Date      : 2024/04/11
 * *****************************************************************************
 */

#include "analysis/csrc/viewer/database/finals/host_info_processor.h"

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"


using namespace Analysis::Viewer::Database;
using namespace Parser::Environment;
using namespace Analysis::Utils;
using HostInfoDataFormat = std::vector<std::tuple<std::string, std::string>>;

const std::string HOST_INFO_DIR = "./host_info";
const std::string MSPROF = "msprof.db";
const std::string DB_PATH = File::PathJoin({HOST_INFO_DIR, MSPROF});

class HostInfoProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(HOST_INFO_DIR));
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(HOST_INFO_DIR, 0));
    }

    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
        if (File::Exist(DB_PATH)) {
            EXPECT_TRUE(File::DeleteFile(DB_PATH));
        }
    }
};

TEST_F(HostInfoProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    std::vector<std::string> hostDirs = {"host"};
    std::string hostUid = "16446744073709551615";
    std::string hostName = "localhost";
    MOCKER_CPP(&File::GetFilesWithPrefix).stubs().will(returnValue(hostDirs));
    MOCKER_CPP(&Context::GetHostUid).stubs().will(returnValue(hostUid));
    MOCKER_CPP(&Context::GetHostName).stubs().will(returnValue(hostName));
    auto processor = HostInfoProcessor(DB_PATH, {""});
    EXPECT_TRUE(processor.Run());
    MOCKER_CPP(&File::GetFilesWithPrefix).reset();
    MOCKER_CPP(&Context::GetHostUid).reset();
    MOCKER_CPP(&Context::GetHostName).reset();
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, DB_PATH);
    HostInfoDataFormat checkData;
    HostInfoDataFormat expectData = {
        {hostUid, hostName}
    };
    std::string sqlStr = "SELECT hostUid, hostName FROM " + TABLE_NAME_HOST_INFO;
    ASSERT_NE(dbRunner, nullptr);
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
    EXPECT_EQ(expectData, checkData);
}

TEST_F(HostInfoProcessorUTest, TestRunShouldReturnTrueWhenNoHost)
{
    auto processor = HostInfoProcessor(DB_PATH, {""});
    EXPECT_TRUE(processor.Run());
}

TEST_F(HostInfoProcessorUTest, TestRunShouldReturnFalseWhenOneProcessFailInMultithreading)
{
    std::vector<std::string> hostDirs = {"host"};
    std::string hostUid = "16137761922582644485";
    std::string hostName = "localhost.localhost";
    MOCKER_CPP(&File::GetFilesWithPrefix).stubs().will(returnValue(hostDirs));
    MOCKER_CPP(&Context::GetHostUid).stubs().will(returnValue(hostUid));
    MOCKER_CPP(&Context::GetHostName).stubs().will(returnValue(hostName));
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(false));
    auto processor = HostInfoProcessor(DB_PATH, {""});
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&DBRunner::CreateTable).reset();
    MOCKER_CPP(&File::GetFilesWithPrefix).reset();
    MOCKER_CPP(&Context::GetHostUid).reset();
    MOCKER_CPP(&Context::GetHostName).reset();
}
