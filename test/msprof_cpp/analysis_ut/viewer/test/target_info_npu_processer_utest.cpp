/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : target_info_npu_processer.cpp
 * Description        : TargetInfoNpuProcesser UT
 * Author             : msprof team
 * Creation Date      : 2024/01/11
 * *****************************************************************************
 */

#include "analysis/csrc/viewer/database/finals/target_info_npu_processer.h"

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

const std::string TARGET_INFO_NPU_DIR = "./target_info_npu";
const std::string REPORT = "report.db";


using namespace Analysis::Viewer::Database;
using namespace Parser::Environment;
using namespace Analysis::Utils;
using NpuInfoDataFormat = std::vector<std::tuple<uint32_t, std::string>>;

class TargetInfoNpuProcesserUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(TARGET_INFO_NPU_DIR));
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(TARGET_INFO_NPU_DIR, 0));
    }

    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
        if (File::Exist(File::PathJoin({TARGET_INFO_NPU_DIR, REPORT}))) {
            EXPECT_TRUE(File::DeleteFile(File::PathJoin({TARGET_INFO_NPU_DIR, REPORT})));
        }
    }
};

TEST_F(TargetInfoNpuProcesserUTest, TestRunShouldReturnTrueWhenProcesserRunSuccess)
{
    std::vector<std::string> deviceDirs = {"device_0", "device_1", "device_2", "device_3", "device_4", "device_5"};
    uint16_t chip0 = 0;
    uint16_t chip1 = 1;
    uint16_t chip4 = 4;
    uint16_t chip5 = 5;
    uint16_t chip7 = 7;
    uint16_t chipX = 20;
    std::string dbPath = File::PathJoin({TARGET_INFO_NPU_DIR, REPORT});
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
    auto processer = TargetInfoNpuProcesser(File::PathJoin({TARGET_INFO_NPU_DIR, REPORT}), {""});
    EXPECT_TRUE(processer.Run());
    MOCKER_CPP(&File::GetFilesWithPrefix).reset();
    MOCKER_CPP(&Context::GetPlatformVersion).reset();
    auto dbRunner = MakeShared<DBRunner>(dbPath);
    NpuInfoDataFormat checkData;
    NpuInfoDataFormat expectData = {
        {0, "Ascend310"},
        {1, "Ascend910A"},
        {2, "Ascend310P"},
        {3, "Ascend910B"},
        {4, "Ascend310B"},
        {5, "UNKNOWN"},
    };
    std::string sqlStr = "Select id, name From " + TABLE_NAME_TARGET_INFO_NPU;
    if (dbRunner != nullptr) {
        EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
        EXPECT_EQ(expectData, checkData);
    }
}

