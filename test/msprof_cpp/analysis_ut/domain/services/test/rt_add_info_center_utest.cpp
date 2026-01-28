/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
*/

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include "analysis/csrc/domain/services/parser/host/cann/rt_add_info_center.h"
#include "analysis/csrc/domain/services/parser/host/cann/hash_data.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"

using namespace Analysis::Utils;
using namespace Analysis::Domain::Host::Cann;
using namespace Analysis::Infra;
using namespace Analysis::Domain;

namespace {
using dataFormat = std::vector<std::tuple<std::string, std::string, uint32_t, uint32_t, uint16_t, uint64_t, uint32_t,
                                          uint16_t, std::string, std::string, std::string, uint16_t, uint16_t,
                                          uint16_t, std::string, uint16_t, std::string, std::string, std::string,
                                          std::string, std::string, std::string>>;
const dataFormat DATA{
    {"node", "123", 123, 111, 5, 800, 30, 10, "12345", "AI_CORE", "123456", 20, 0, 0, "dynamic", 3,
     "NCL;NCL", "FLOAT;FLOAT", "\"63,63,511;63,63,511\"", "ND", "FLOAT", "\"261121\""},
    {"node", "123", 123, 111, 5, 800, 30, 11, "54321", "AI_VECTOR_CORE", "654321", 20, 0, 0, "static", 3,
     "NCL;ND", "BOOL;INT64", "\"63,63,511;1\"", "ND", "BOOL", "\"63,63\""},
    {"node", "123", 123, 111, 5, 800, 30, 12, "abcde", "AI_VECTOR_CORE", "abcde", 20, 0, 0, "static", 3,
        "NCL;ND", "BOOL;INT64", "\"63,63,511;1\"", "ND", "BOOL", "\"63,63\""}};
const std::string PROF_PATH = "./PROF_XXX";
const std::string HOST_PATH = File::PathJoin({PROF_PATH, "host"});
const std::string SQLITE_PATH = File::PathJoin({HOST_PATH, "sqlite"});
const std::string RTS_TRACK_DB_PATH = File::PathJoin({PROF_PATH, "host", "sqlite", "rts_track.db"});
const std::string DATA_PATH = File::PathJoin({HOST_PATH, "data"});
const std::string HASH_FILE = File::PathJoin({DATA_PATH, "unaging.additional.hash_dic.slice_0"});

}

class RTAddInfoCenterUTest : public testing::Test {
protected:
    // 所有测试用例之前执行
    static void SetUpTestCase()
    {
        // 创建db路径
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(HOST_PATH));
        EXPECT_TRUE(File::CreateDir(SQLITE_PATH));
        EXPECT_TRUE(File::CreateDir(DATA_PATH));

        // 创建hash_file文件
        FileWriter fw(HASH_FILE);
        fw.WriteText("12345:matmulV2\n");
        fw.WriteText("123456:matmul\n");
        fw.WriteText("54321:mulV2\n");
        fw.WriteText("654321:mul\n");
        fw.Close();
    }
    // 所有测试用例之后执行
    static void TearDownTestCase()
    {
        File::RemoveDir(PROF_PATH, 0);
    }

    void SetUp() override
    {
        RtsTrackDB rtsTrackDB;
        std::string opInfo = "RuntimeOpInfo";
        CreateDB(rtsTrackDB, opInfo, RTS_TRACK_DB_PATH, DATA);
    }

    void TearDown() override
    {
        EXPECT_TRUE(File::DeleteFile(RTS_TRACK_DB_PATH));
    }

    template<typename T>
    void CreateDB(Database db, const std::string &tableName, const std::string& dpPath, T data)
    {
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dpPath);
        auto cols = db.GetTableCols(tableName);
        EXPECT_TRUE(dbRunner->CreateTable(tableName, cols));
        EXPECT_TRUE(dbRunner->InsertData(tableName, data));
    }
};

TEST_F(RTAddInfoCenterUTest, LoadWhenFileNotExist)
{
    RTAddInfoCenter::GetInstance().Load(PROF_PATH);
}

void CheckRuntimeOpInfo(RuntimeOpInfo info, RuntimeOpInfo expect)
{
    EXPECT_EQ(info.deviceId, expect.deviceId);
    EXPECT_EQ(info.taskId, expect.taskId);
    EXPECT_EQ(info.blockNum, expect.blockNum);
    EXPECT_EQ(info.mixBlockNum, expect.mixBlockNum);
    EXPECT_EQ(info.opFlag, expect.opFlag);
    EXPECT_EQ(info.tensorNum, expect.tensorNum);
    EXPECT_EQ(info.modelId, expect.modelId);
    EXPECT_EQ(info.taskType, expect.taskType);
    EXPECT_EQ(info.opType, expect.opType);
    EXPECT_EQ(info.opName, expect.opName);
    EXPECT_EQ(info.inputDataTypes, expect.inputDataTypes);
    EXPECT_EQ(info.inputShapes, expect.inputShapes);
    EXPECT_EQ(info.inputFormats, expect.inputFormats);
    EXPECT_EQ(info.outputDataTypes, expect.outputDataTypes);
    EXPECT_EQ(info.outputShapes, expect.outputShapes);
    EXPECT_EQ(info.outputFormats, expect.outputFormats);
}

TEST_F(RTAddInfoCenterUTest, LoadThenTestGetWhenLoadSuccess)
{
    HashData::GetInstance().Load(DATA_PATH);
    RTAddInfoCenter::GetInstance().Load(SQLITE_PATH);
    // info 1 2 是有效值，3是空
    auto info1 = RTAddInfoCenter::GetInstance().Get(5, 30, 10);
    auto info2 = RTAddInfoCenter::GetInstance().Get(5, 30, 11);
    auto info3 = RTAddInfoCenter::GetInstance().Get(5, 20, 10);

    RuntimeOpInfo expect1{5, 10, 20, 0, 0, 3, 30, 800, "AI_CORE", "matmul", "matmulV2", "dynamic",
                          "NCL;NCL", "FLOAT;FLOAT", "\"63,63,511;63,63,511\"", "ND", "FLOAT", "\"261121\""};
    RuntimeOpInfo expect2{5, 11, 20, 0, 0, 3, 30, 800, "AI_VECTOR_CORE", "mul", "mulV2", "static",
                          "NCL;ND", "BOOL;INT64", "\"63,63,511;1\"", "ND", "BOOL", "\"63,63\""};
    CheckRuntimeOpInfo(info1, expect1);
    CheckRuntimeOpInfo(info2, expect2);
    CheckRuntimeOpInfo(info3, RuntimeOpInfo());
}
