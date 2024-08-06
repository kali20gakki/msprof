/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hash_init_processor_utest.cpp
 * Description        : hash_init_processor UT
 * Author             : msprof team
 * Creation Date      : 2024/8/1
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/data_process/ai_task/hash_init_processor.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/utils/file.h"

using namespace Analysis::Viewer::Database;
using namespace Analysis::Domain;
using namespace Analysis::Utils;
const int DEPTH = 0;
const std::string HASH_PATH = "./hash";
const std::string PROF_PATH = File::PathJoin({HASH_PATH, "PROF_0"});
const std::string DB_SUFFIX = "ge_hash.db";
const std::string TABLE_NAME = "GeHashInfo";
const std::string PROCESS_HASH = "Hash";

using GeHashFormat = std::vector<std::tuple<std::string, std::string>>;

GeHashFormat DATA{{"7383439776149831", "Cast"},
                  {"247669290252505", "Add"},
                  {"8477521346829072275", "aclnnMul_MulAiCore_Mul"}};

class HashInitProcessorUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        if (File::Check(HASH_PATH)) {
            File::RemoveDir(HASH_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(HASH_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH, HOST})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH, HOST, SQLITE})));
        CreateHashData(File::PathJoin({PROF_PATH, HOST, SQLITE, DB_SUFFIX}), DATA);
    }
    virtual void TearDown()
    {
        GlobalMockObject::verify();
        EXPECT_TRUE(File::RemoveDir(HASH_PATH, DEPTH));
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

TEST_F(HashInitProcessorUTest, ShouldReturnTrueWhenProcessRunSuccess)
{
    DataInventory dataInventory;
    auto processor = HashInitProcessor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESS_HASH));
    auto res = dataInventory.GetPtr<GeHashMap>();
    EXPECT_EQ(3ul, res->size());
}

TEST_F(HashInitProcessorUTest, ShouldReturnFalseWhenSourceTableNotExists)
{
    auto dbPath = File::PathJoin({PROF_PATH, HOST, SQLITE, DB_SUFFIX});
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    DataInventory dataInventory;
    auto processor = HashInitProcessor(PROF_PATH);
    EXPECT_FALSE(processor.Run(dataInventory, PROCESS_HASH));
    auto res = dataInventory.GetPtr<GeHashMap>();
    EXPECT_EQ(0ul, res->size());
}

TEST_F(HashInitProcessorUTest, ShouldReturnFalseWhenFileOverMaxSize)
{
    DataInventory dataInventory;
    auto processor = HashInitProcessor(PROF_PATH);
    MOCKER_CPP(&Analysis::Utils::FileReader::Check)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESS_HASH));
    auto res = dataInventory.GetPtr<GeHashMap>();
    EXPECT_EQ(0ul, res->size());
}