/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : export_summary_utest.cpp
 * Description        : export_summary UT
 * Author             : msprof team
 * Creation Date      : 2024/12/3
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/application/include/export_summary.h"
#include "analysis/csrc/application/summary/summary_constant.h"
#include "analysis/csrc/utils/file.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/viewer/database/database.h"
#include "analysis/csrc/viewer/database/db_runner.h"
#include "analysis/csrc/parser/environment/context.h"

using namespace Analysis::Application;
using namespace Analysis::Utils;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Parser::Environment;

namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./export_summary_test";
const std::string DEVICE = "device_0";
const std::string HASH_DB = "ge_hash.db";
const std::string TABLE_NAME = "GeHashInfo";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, OUTPUT_PATH});
}

using GeHashFormat = std::vector<std::tuple<std::string, std::string>>;
GeHashFormat DATA{{"7383439776149831", "Cast"},
                  {"247669290252505", "Add"},
                  {"8477521346829072275", "aclnnMul_MulAiCore_Mul"}};

class ExportSummaryUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH, HOST})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH, HOST, SQLITE})));
        CreateHashData(File::PathJoin({PROF_PATH, HOST, SQLITE, HASH_DB}), DATA);
    }
    virtual void TearDown()
    {
        GlobalMockObject::verify();
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
    }

    static void CreateHashData(const std::string& dbPath, GeHashFormat& data)
    {
        std::shared_ptr<HashDB> database;
        MAKE_SHARED0_RETURN_VOID(database, HashDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(TABLE_NAME);
        dbRunner->CreateTable(TABLE_NAME, cols);
        dbRunner->InsertData(TABLE_NAME, data);
    }
};

TEST_F(ExportSummaryUTest, ShouldReturnFalseWhenPathIsInvalid)
{
    std::string path = "/home/prof_0";
    ExportSummary manager(path);
    EXPECT_FALSE(manager.Run());
}

TEST_F(ExportSummaryUTest, ShouldReturnFalseWhenContextInitFail)
{
    ExportSummary manager(PROF_PATH);
    EXPECT_FALSE(manager.Run());
}

TEST_F(ExportSummaryUTest, ShouldReturnFalseWhenProcessFail)
{
    ExportSummary manager(PROF_PATH);
    MOCKER_CPP(&Context::Load).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetClockMonotonicRaw).stubs().will(returnValue(true));
    EXPECT_FALSE(manager.Run());
}