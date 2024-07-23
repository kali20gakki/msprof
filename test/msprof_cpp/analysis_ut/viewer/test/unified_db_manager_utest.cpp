/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : unified_db_manager_utest.cpp
 * Description        : unifiedDbManager.cpp UT
 * Author             : msprof team
 * Creation Date      : 2024/01/27
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/unified_db_manager.h"
#include "analysis/csrc/parser/environment/context.h"

#include <cstdlib>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/utils/file.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Utils;
using namespace Analysis::Parser::Environment;
using namespace Analysis::Viewer::Database;

const std::string UNIFIED_DB_DIR = "./unifiedDBManagerUTest";
const std::string SINGLE_DIR =  File::PathJoin({UNIFIED_DB_DIR, "single"});
const std::string MULTI_DIR = File::PathJoin({UNIFIED_DB_DIR, "multi"});
const std::string PROF_1 = "PROF_1";
const std::string PROF_2 = "PROF_2";

class UnifiedDBManagerUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(UNIFIED_DB_DIR));
        EXPECT_TRUE(File::CreateDir(SINGLE_DIR));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({SINGLE_DIR, PROF_1})));
        EXPECT_TRUE(File::CreateDir(MULTI_DIR));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({MULTI_DIR, PROF_1})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({MULTI_DIR, PROF_2})));
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(UNIFIED_DB_DIR, 0));
    }

    virtual void SetUp() {}

    virtual void TearDown()
    {
        GlobalMockObject::verify();
    }
};

TEST_F(UnifiedDBManagerUTest, TestInitReturnFalseWhenOutputDirIsEmpty)
{
    UnifiedDBManager mgr = UnifiedDBManager("");
    EXPECT_FALSE(mgr.Init());
}

TEST_F(UnifiedDBManagerUTest, TestInitReturnFalseWhenNoPROFInPathCheck)
{
    UnifiedDBManager mgr = UnifiedDBManager(File::PathJoin({UNIFIED_DB_DIR, "abc"}));
    EXPECT_FALSE(mgr.Init());
}

TEST_F(UnifiedDBManagerUTest, TestInitReturnFalseWhenLoadContextFailedInInit)
{
    UnifiedDBManager mgr = UnifiedDBManager(SINGLE_DIR);
    MOCKER_CPP(&Context::Load).stubs().will(returnValue(false));
    EXPECT_FALSE(mgr.Init());
}

TEST_F(UnifiedDBManagerUTest, TestInitReturnTrueWhenOk)
{
    UnifiedDBManager mgr = UnifiedDBManager(SINGLE_DIR);
    MOCKER_CPP(&Context::Load).stubs().will(returnValue(true));
    EXPECT_TRUE(mgr.Init());
}

TEST_F(UnifiedDBManagerUTest, TestRunReturnFalseWhenNoData)
{
    UnifiedDBManager mgr = UnifiedDBManager(SINGLE_DIR);
    MOCKER_CPP(&Context::Load).stubs().will(returnValue(true));
    EXPECT_TRUE(mgr.Init());
    // 直接跑的run，部分表由于缺少原始数据会报错，但是不影响执行整体流程
    EXPECT_FALSE(mgr.Run());
}
