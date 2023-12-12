/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : context_utest.cpp
 * Description        : Context UT
 * Author             : msprof team
 * Creation Date      : 2023/12/07
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "nlohmann/json.hpp"
#include "context.h"
#include "file.h"

using namespace Analysis::Utils;
using namespace Analysis::Parser::Environment;

const auto INFO_JSON = "./info.json";

class ContextUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        nlohmann::json info = {
            {"drvVersion", 467732},
            {"platform_version", "7"},
        };
        FileWriter infoWriter(INFO_JSON);
        infoWriter.WriteText(info.dump());
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::DeleteFile(INFO_JSON));
    }

    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
        Context::GetInstance().Clear();
    }
};

TEST_F(ContextUTest, TestLoadShouldReturnTrueWhenReadJsonSuccess)
{
    EXPECT_TRUE(Context::GetInstance().Load("./", HOST_ID));
}

TEST_F(ContextUTest, TestLoadShouldReturnFalseWhenReadJsonException)
{
    const std::string infoJsonError = "./info.json.error";
    FileWriter fw(infoJsonError);
    fw.WriteText(".");
    EXPECT_FALSE(Context::GetInstance().Load("./", HOST_ID));
    EXPECT_TRUE(File::DeleteFile(infoJsonError));
}

TEST_F(ContextUTest, TestIsAllExportShouldReturnFalseWhenContextEmpty)
{
    EXPECT_FALSE(Context::GetInstance().IsAllExport());
}

TEST_F(ContextUTest, TestIsAllExportShouldReturnTrueWhenAllExportConditionIsMatch)
{
    EXPECT_TRUE(Context::GetInstance().Load("./", HOST_ID));
    EXPECT_TRUE(Context::GetInstance().IsAllExport());
}

TEST_F(ContextUTest, TestIsAllExportShouldReturnFalseWhenDrvVersionLessThanAllExportVersion)
{
    const std::string infoJson = "./info.json.2";
    nlohmann::json info = {
        {"drvVersion", 0},
    };
    FileWriter infoWriter(infoJson);
    infoWriter.WriteText(info.dump());
    EXPECT_TRUE(Context::GetInstance().Load("./", HOST_ID));
    EXPECT_FALSE(Context::GetInstance().IsAllExport());
    EXPECT_TRUE(File::DeleteFile(infoJson));
}

TEST_F(ContextUTest, TestIsAllExportShouldReturnFalseWhenStrToU16Failed)
{
    const std::string infoJson = "./info.json.2";
    nlohmann::json info = {
        {"drvVersion", 467732},
        {"platform_version", "2dd"},
    };
    FileWriter infoWriter(infoJson);
    infoWriter.WriteText(info.dump());
    EXPECT_TRUE(Context::GetInstance().Load("./", HOST_ID));
    EXPECT_FALSE(Context::GetInstance().IsAllExport());
    EXPECT_TRUE(File::DeleteFile(infoJson));
}

TEST_F(ContextUTest, TestIsAllExportShouldReturnFalseWhenChipV310)
{
    const std::string infoJson = "./info.json.2";
    nlohmann::json info = {
        {"drvVersion", 467732},
        {"platform_version", "2"},
    };
    FileWriter infoWriter(infoJson);
    infoWriter.WriteText(info.dump());
    EXPECT_TRUE(Context::GetInstance().Load("./", HOST_ID));
    EXPECT_FALSE(Context::GetInstance().IsAllExport());
    EXPECT_TRUE(File::DeleteFile(infoJson));
}