/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : npu_info_processor.cpp
 * Description        : NpuInfoProcessor UT
 * Author             : msprof team
 * Creation Date      : 2024/01/11
 * *****************************************************************************
 */

#include "analysis/csrc/viewer/database/finals/npu_info_processor.h"

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"


using namespace Analysis::Viewer::Database;
using namespace Domain::Environment;
using namespace Analysis::Utils;
using NpuInfoDataFormat = std::vector<std::tuple<uint32_t, std::string>>;

const std::string NPU_INFO_DIR = "./npu_info";
const std::string MSPROF = "msprof.db";
const std::string DB_PATH = File::PathJoin({NPU_INFO_DIR, MSPROF});

class NpuInfoProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(NPU_INFO_DIR));
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(NPU_INFO_DIR, 0));
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

TEST_F(NpuInfoProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    std::vector<std::string> deviceDirs = {"device_0", "device_1", "device_2", "device_3", "device_4", "device_5"};
    uint16_t chip0 = 0;
    uint16_t chip1 = 1;
    uint16_t chip4 = 4;
    uint16_t chip5 = 5;
    uint16_t chip7 = 7;
    uint16_t chipX = 20;
    MOCKER_CPP(&File::GetFilesWithPrefix)
        .stubs()
        .will(returnValue(deviceDirs));
    MOCKER_CPP(&Context::GetPlatformVersion)
        .stubs()
        .will(returnValue(chip0))
        .then(returnValue(chip1))
        .then(returnValue(chip4))
        .then(returnValue(chip5))
        .then(returnValue(chip7))
        .then(returnValue(chipX));
    auto processor = NpuInfoProcessor(DB_PATH, {""});
    EXPECT_TRUE(processor.Run());
    MOCKER_CPP(&File::GetFilesWithPrefix).reset();
    MOCKER_CPP(&Context::GetPlatformVersion).reset();
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, DB_PATH);
    NpuInfoDataFormat checkData;
    NpuInfoDataFormat expectData = {
        {0, "Ascend310"},
        {1, "Ascend910A"},
        {2, "Ascend310P"},
        {3, "Ascend910B"},
        {4, "Ascend310B"},
        {5, "UNKNOWN"},
    };
    std::string sqlStr = "SELECT id, name FROM " + TABLE_NAME_NPU_INFO;
    ASSERT_NE(dbRunner, nullptr);
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
    EXPECT_EQ(expectData, checkData);
}

TEST_F(NpuInfoProcessorUTest, TestRunShouldReturnTrueWhenNoDevice)
{
    auto processor = NpuInfoProcessor(DB_PATH, {""});
    EXPECT_TRUE(processor.Run());
}

TEST_F(NpuInfoProcessorUTest, TestRunShouldReturnFalseWhenOneProcessFailInMultithreading)
{
    using format = std::vector<std::tuple<uint16_t, std::string>>;
    std::set<std::string> profPath = {"PROF_0", "PROF_1", "PROF_2", "PROF_3"};
    std::vector<std::string> deviceDirs0 = {"device_0"};
    std::vector<std::string> deviceDirs1 = {"device_1"};
    std::vector<std::string> deviceDirs2 = {"device_2"};
    uint16_t chip0 = 0;
    MOCKER_CPP(&File::GetFilesWithPrefix)
    .stubs()
    .will(returnValue(deviceDirs0))
    .then(returnValue(deviceDirs1))
    .then(returnValue(deviceDirs1))
    .then(returnValue(deviceDirs2));
    MOCKER_CPP(&Context::GetPlatformVersion)
    .stubs()
    .will(returnValue(chip0));
    format checkData;
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(false));
    auto processor = NpuInfoProcessor(DB_PATH, profPath);
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&DBRunner::CreateTable).reset();
    MOCKER_CPP(&File::GetFilesWithPrefix).reset();
    MOCKER_CPP(&Context::GetPlatformVersion).reset();
}

