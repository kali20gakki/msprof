/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : memcpy_info_dumper_utest.cpp
 * Description        : MemcpyInfoDumper UT
 * Author             : msprof team
 * Creation Date      : 2024/12/12
 * *****************************************************************************
 */
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/entities/event.h"
#include "analysis/csrc/viewer/database/drafts/memcpy_info_dumper.h"

using namespace Analysis::Utils;
using namespace Analysis::Viewer::Database::Drafts;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Entities;
using ApiData = std::vector<std::shared_ptr<Event>>;
const std::string TEST_DB_FILE_PATH = "./sqlite";
const uint32_t RUNTIME_LEVEL_NUMBER = 5000;
class MemcpyInfoDumperUtest : public testing::Test {
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

TEST_F(MemcpyInfoDumperUtest, TestMemcpyInfoDumperShouldInsertDataCorrectly)
{
    MemcpyInfoDumper memcpyInfoDumper(".");
    auto info = std::make_shared<MsprofCompactInfo>();
    info->level = RUNTIME_LEVEL_NUMBER;
    info->threadId = 1; // threadId 1
    info->timeStamp = 1;
    info->dataLen = 18;  // dataLen 18
    info->data.memcpyInfo.dataSize = 2048;  // dataSize 2048
    info->data.memcpyInfo.memcpyDirection = 0;
    info->data.memcpyInfo.maxSize = 4096;  // maxSize 4096
    std::vector<std::shared_ptr<MsprofCompactInfo>> memcpyInfos;
    memcpyInfos.push_back(info);
    auto res = memcpyInfoDumper.DumpData(memcpyInfos);
    EXPECT_TRUE(res);
    RuntimeDB runtimeDB;
    std::string runtimeDBPath = Utils::File::PathJoin({TEST_DB_FILE_PATH, runtimeDB.GetDBName()});
    DBRunner runtimeDBRunner(runtimeDBPath);
    MemcpyInfoData data;
    runtimeDBRunner.QueryData("select * from MemcpyInfo", data);
    EXPECT_EQ(std::get<1>(data[0]), "runtime");
    EXPECT_EQ(std::get<2>(data[0]), 1);  // 第2列 2
    EXPECT_EQ(std::get<3>(data[0]), 18);  // 第3列 3, 18
    EXPECT_EQ(std::get<4>(data[0]), 1);  // 第4列 4
    EXPECT_EQ(std::get<5>(data[0]), 2048);  // 第5列 5, 2048
    EXPECT_EQ(std::get<6>(data[0]), 0);  // 第5列 6
    EXPECT_EQ(std::get<7>(data[0]), 4096);  // 第5列 7, 4096
}

TEST_F(MemcpyInfoDumperUtest, TestMemcpyInfoDumperShouldReturnFalseWhenDBNotCreated)
{
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(false));
    MemcpyInfoDumper memcpyInfoDumper(".");
    auto info = std::make_shared<MsprofCompactInfo>();
    std::vector<std::shared_ptr<MsprofCompactInfo>> memcpyInfos;
    memcpyInfos.push_back(info);
    auto res = memcpyInfoDumper.DumpData(memcpyInfos);
    ASSERT_FALSE(res);
}

TEST_F(MemcpyInfoDumperUtest, TestMemcpyInfoDumperShouldReturnFalseWhenCannotInsertTable)
{
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(true));
    MemcpyInfoDumper memcpyInfoDumper(".");
    std::vector<std::shared_ptr<MsprofCompactInfo>> memcpyInfos;
    auto info = std::make_shared<MsprofCompactInfo>();
    memcpyInfos.push_back(info);
    auto res = memcpyInfoDumper.DumpData(memcpyInfos);
    ASSERT_FALSE(res);
}