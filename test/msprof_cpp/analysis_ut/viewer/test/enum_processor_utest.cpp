/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : enum_processor.cpp
 * Description        : EnumApiLevelProcessor UT
 * Author             : msprof team
 * Creation Date      : 2024/01/10
 * *****************************************************************************
 */

#include "analysis/csrc/viewer/database/finals/enum_processor.h"

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Viewer::Database;
using namespace Analysis::Utils;
using EnumDataFormat = std::vector<std::tuple<uint32_t, std::string>>;

const std::string ENUM_DIR = "./enum";
const std::string REPORT = "report.db";
const std::string DB_PATH = File::PathJoin({ENUM_DIR, REPORT});

class EnumProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(ENUM_DIR));
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(ENUM_DIR, 0));
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

void CheckEnumApiLevel(const std::shared_ptr<DBRunner> &dbRunner)
{
    EnumDataFormat checkData;
    std::string sqlStr = "SELECT id, name FROM " + TABLE_NAME_ENUM_API_TYPE;
    const uint32_t ID_INDEX = 0;
    const uint32_t NAME_INDEX = 1;
    const uint16_t expectNum = API_LEVEL_TABLE.size();
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
    EXPECT_EQ(expectNum, checkData.size());
    for (auto record : checkData) {
        EXPECT_EQ(std::get<ID_INDEX>(record), API_LEVEL_TABLE.find(std::get<NAME_INDEX>(record))->second);
    }
}

void CheckEnumMemory(const std::shared_ptr<DBRunner> &dbRunner)
{
    EnumDataFormat checkData;
    std::string sqlStr = "SELECT id, name FROM " + TABLE_NAME_ENUM_MEMORY;
    const uint32_t ID_INDEX = 0;
    const uint32_t NAME_INDEX = 1;
    const uint16_t expectNum = MEMORY_TABLE.size();
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
    EXPECT_EQ(expectNum, checkData.size());
    for (auto record : checkData) {
        EXPECT_EQ(std::get<ID_INDEX>(record), MEMORY_TABLE.find(std::get<NAME_INDEX>(record))->second);
    }
}

void CheckEnumNpuModule(const std::shared_ptr<DBRunner> &dbRunner)
{
    EnumDataFormat checkData;
    std::string sqlStr = "SELECT id, name FROM " + TABLE_NAME_ENUM_NPU_MODULE;
    const uint32_t ID_INDEX = 0;
    const uint32_t NAME_INDEX = 1;
    const uint16_t expectNum = NPU_MODULE_NAME_TABLE.size();
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
    EXPECT_EQ(expectNum, checkData.size());
    for (auto record : checkData) {
        EXPECT_EQ(std::get<ID_INDEX>(record), NPU_MODULE_NAME_TABLE.find(std::get<NAME_INDEX>(record))->second);
    }
}

TEST_F(EnumProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    // 执行enum processor
    auto processor = EnumProcessor(DB_PATH, {""});
    EXPECT_TRUE(processor.Run());

    // 校验processor生成的若干表及内容
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, DB_PATH);
    ASSERT_NE(dbRunner, nullptr);
    CheckEnumApiLevel(dbRunner);
    CheckEnumMemory(dbRunner);
    CheckEnumNpuModule(dbRunner);
}

TEST_F(EnumProcessorUTest, TestRunShouldReturnFalseWhenReserveFailedThenDataIsEmpty)
{
    using TempT = std::tuple<uint16_t, std::string>;
    MOCKER_CPP(&std::vector<TempT>::reserve).stubs().will(throws(std::bad_alloc()));
    auto processor = EnumProcessor(DB_PATH, {""});
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&std::vector<TempT>::reserve).reset();
}

TEST_F(EnumProcessorUTest, TestRunShouldReturnFalseWhenMakeSharedFailedThenPtrIsNullptr)
{
    MOCKER_CPP(&std::make_shared<ReportDB>).stubs().will(throws(std::bad_alloc()));
    auto processor = EnumProcessor(DB_PATH, {""});
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&std::make_shared<ReportDB>).reset();
}

TEST_F(EnumProcessorUTest, TestRunShouldReturnFalseWhenCreateTableFailed)
{
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(false));
    auto processor = EnumProcessor(DB_PATH, {""});
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&DBRunner::CreateTable).reset();
}
