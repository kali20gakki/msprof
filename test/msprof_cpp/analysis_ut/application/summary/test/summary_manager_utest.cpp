/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : summary_manager_utest.cpp
 * Description        : summary_manager UT
 * Author             : msprof team
 * Creation Date      : 2025/5/24
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/application/summary/summary_manager.h"
#include "analysis/csrc/application/summary/summary_factory.h"

using namespace Analysis::Application;
using namespace Analysis::Utils;
namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./summary_test";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, OUTPUT_PATH});
}

class SummaryManagerUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        GlobalMockObject::verify();
    }
    static void SetUpTestCase()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(RESULT_PATH));
    }
    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
        GlobalMockObject::verify();
    }
protected:
    DataInventory dataInventory_;
};

TEST_F(SummaryManagerUTest, ShouldReturnTrueWhenNoDataInDataInventory)
{
    SummaryManager manager(PROF_PATH, RESULT_PATH);
    EXPECT_TRUE(manager.Run(dataInventory_));
}

TEST_F(SummaryManagerUTest, ShouldReturnFalseWhenGetAssemblerFailed)
{
    SummaryManager manager(PROF_PATH, RESULT_PATH);
    std::shared_ptr<SummaryAssembler> assembler{nullptr};
    MOCKER_CPP(&SummaryFactory::GetAssemblerByName).stubs().will(returnValue(assembler));
    EXPECT_FALSE(manager.Run(dataInventory_));
    MOCKER_CPP(&SummaryFactory::GetAssemblerByName).reset();
}