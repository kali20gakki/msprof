/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : connection_utest.cpp
 * Description        : Connection UT
 * Author             : msprof team
 * Creation Date      : 2023/11/8
 * *****************************************************************************
 */
#include <memory>
#include <string>
#include "gtest/gtest.h"
#include "db_runner.h"
#include "thread_pool.h"

using DATA_FORMAT = std::vector<std::tuple<int32_t, uint32_t, int64_t, double, double, std::string>>;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Utils;

class DBRunnerUtest : public testing::Test {
protected:
    static void TearDownTestCase()
    {
        if (File::Exist("/tmp/a.db")) {
            File::DeleteFile("/tmp/a.db");
        }
    }
};

std::vector<TableColumn> cols = {
    TableColumn("col1", "INTEGER"),
    TableColumn("col2", "INTEGER"),
    TableColumn("col3", "INTEGER"),
    TableColumn("col4", "NUMERIC"),
    TableColumn("col5", "REAL"),
    TableColumn("col6", "TEXT")
};

TEST_F(DBRunnerUtest, DBRunner)
{
    std::string path = "/tmp/a.db";
    auto dbRunner = std::make_shared<DBRunner>(path);
    EXPECT_EQ(path, dbRunner->path_);
}

TEST_F(DBRunnerUtest, CreateTable)
{
    std::string path = "/tmp/a.db";
    auto dbRunner = std::make_shared<DBRunner>(path);
    std::vector<TableColumn> cols = {
        TableColumn("id", "INT"),
        TableColumn("name", "VARCHAR(255)"),
        TableColumn("age", "INT")
    };
    auto rc = dbRunner->CreateTable("tb3", cols);
    EXPECT_EQ(rc, true);
}

TEST_F(DBRunnerUtest, DropTable)
{
    std::string path = "/tmp/a.db";
    auto dbRunner = std::make_shared<DBRunner>(path);
    auto rc = dbRunner->DropTable("tb3");
    EXPECT_EQ(rc, true);
}

TEST_F(DBRunnerUtest, InsertData)
{
    std::string path = "/tmp/a.db";
    auto dbRunner = std::make_shared<DBRunner>(path);
    dbRunner->CreateTable("tb4", cols);
    DATA_FORMAT data = {
        std::make_tuple(2147483647, 4294967295, 9223372036854775807, 118972596256332.95,
                        1340.046875,  "hcom_allGather__516_2")
    };
    auto rc = dbRunner->InsertData("tb4", data);
    EXPECT_EQ(rc, true);
}

TEST_F(DBRunnerUtest, DeleteData)
{
    std::string path = "/tmp/a.db";
    auto dbRunner = std::make_shared<DBRunner>(path);
    std::vector<TableColumn> cols = {
        TableColumn("id", "INT"),
        TableColumn("name", "VARCHAR(255)"),
        TableColumn("age", "INT")
    };
    dbRunner->CreateTable("tb3", cols);
    std::vector<std::tuple<int, std::string, int>> data = {
        std::make_tuple(1, "Bob", 26),
        std::make_tuple(2, "Tom", 18)
    };
    dbRunner->InsertData("tb3", data);
    auto rc = dbRunner->DeleteData("DELETE FROM tb3 WHERE age>20;");
    EXPECT_EQ(rc, true);
}

TEST_F(DBRunnerUtest, QueryData)
{
    std::string path = "/tmp/a.db";
    auto dbRunner = std::make_shared<DBRunner>(path);
    dbRunner->CreateTable("tb4", cols);
    dbRunner->DeleteData("DELETE FROM tb4;");
    DATA_FORMAT data = {
        std::make_tuple(7, 8, 9, 1320.4, 56.5, "hcom_allGather__516_1")
    };
    dbRunner->InsertData("tb4", data);
    std::string sql = "SELECT * FROM tb4;";
    DATA_FORMAT result;
    auto rc = dbRunner->QueryData(sql, result);
    EXPECT_EQ(rc, true);
    int32_t num1 = 7;
    uint32_t num2 = 8;
    int64_t num3 = 9;
    for (const auto& row : result) {
        int32_t c1 = std::get<0>(row);
        uint32_t c2 = std::get<1>(row);
        int64_t c3 = std::get<2>(row);
        std::string c6 = std::get<5>(row);
        EXPECT_EQ(num1, c1);
        EXPECT_EQ(num2, c2);
        EXPECT_EQ(num3, c3);
        EXPECT_EQ("hcom_allGather__516_1", c6);
    }
}

TEST_F(DBRunnerUtest, UpdateData)
{
    std::string path = "/tmp/a.db";
    auto dbRunner = std::make_shared<DBRunner>(path);
    std::vector<TableColumn> cols = {
        TableColumn("id", "INT"),
        TableColumn("name", "VARCHAR(255)"),
        TableColumn("age", "INT")
    };
    dbRunner->CreateTable("tb3", cols);
    std::vector<std::tuple<int, std::string, int>> data = {
        std::make_tuple(1, "Bob", 26),
        std::make_tuple(2, "Tom", 18)
    };
    dbRunner->InsertData("tb3", data);
    auto rc = dbRunner->UpdateData("UPDATE tb3 SET age = 30 WHERE name = 'Tom';");
    EXPECT_EQ(rc, true);
}

TEST_F(DBRunnerUtest, MultithreadingInsert)
{
    std::string path = "/tmp/a.db";
    auto dbRunner = std::make_shared<DBRunner>(path);
    dbRunner->DropTable("tb4");
    std::vector<TableColumn> cols = {
        TableColumn("col1", "INTEGER"),
        TableColumn("col2", "INTEGER"),
        TableColumn("col3", "INTEGER"),
        TableColumn("col4", "NUMERIC"),
        TableColumn("col5", "REAL"),
        TableColumn("col6", "TEXT")
    };
    dbRunner->CreateTable("tb4", cols);
    const int threadsNum = 10;
    const int rowNum = 10000;
    DATA_FORMAT data;
    auto row = std::make_tuple(2147483647, 4294967295, 9223372036854775807, 118972596256332.95,
                               1340.046875,  "hcom_allGather__516_2");
    for (int i = 0; i < rowNum; i++) {
        data.emplace_back(row);
    }
    ThreadPool pool(threadsNum);
    pool.Start();
    auto task1 = [dbRunner, data]() {
        dbRunner->InsertData("tb4", data);
    };
    auto task2 = [dbRunner, data]() {
        dbRunner->InsertData("tb5", data);
    };
    auto task3 = [dbRunner, data]() {
        dbRunner->InsertData("tb6", data);
    };
    pool.AddTask(task1);
    pool.AddTask(task2);
    pool.AddTask(task3);
    pool.WaitAllTasks();
    pool.Stop();
    std::string sql1 = "SELECT * FROM tb4;";
    std::string sql2 = "SELECT * FROM tb5;";
    std::string sql3 = "SELECT * FROM tb6;";
    DATA_FORMAT result;
    dbRunner->QueryData(sql1, result);
    EXPECT_EQ(rowNum, result.size());
    dbRunner->QueryData(sql2, result);
    EXPECT_EQ(rowNum, result.size());
    dbRunner->QueryData(sql3, result);
    EXPECT_EQ(rowNum, result.size());
}