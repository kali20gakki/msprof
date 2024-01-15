/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : log_utest.cpp
 * Description        : Log UT
 * Author             : msprof team
 * Creation Date      : 2023/11/13
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/dfx/log.h"

using namespace Analysis;
using namespace Analysis::Utils;

class LogUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

TEST_F(LogUTest, TestFormatShouldReturnFormatStringWhenNoArgs)
{
    ASSERT_EQ("test format", Format("test format"));
}

TEST_F(LogUTest, TestFormatShouldReplacePercentSignWhenArgs)
{
    ASSERT_EQ("test format key: 1", Format("test format %: %", "key", 1));
}

TEST_F(LogUTest, TestFormatShouldReturnPercentSignWhenDoubelPercentSign)
{
    ASSERT_EQ("test format key: 1 %", Format("test format %: % %%", "key", 1));
}

TEST_F(LogUTest, TestInitShouldReturnErrorWhenEmptyLogPath)
{
    ASSERT_EQ(ANALYSIS_ERROR, Log::GetInstance().Init(""));
}

TEST_F(LogUTest, TestInitShouldReturnErrorWhenCreateLogDirFail)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&File::CreateDir)
        .stubs().will(returnValue(false));
    ASSERT_EQ(ANALYSIS_ERROR, Log::GetInstance().Init("test"));
}

TEST_F(LogUTest, TestInitShouldReturnErrorWhenOpenLogFileFail)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&File::CreateDir)
        .stubs().will(returnValue(true));
    MOCKER_CPP(&File::Access)
        .stubs().will(returnValue(true));
    MOCKER_CPP(&FileWriter::Open).stubs();
    ASSERT_EQ(ANALYSIS_ERROR, Log::GetInstance().Init("test"));
}

TEST_F(LogUTest, TestInitShouldReturnOKWhenOpenLogSuccess)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&File::CreateDir)
        .stubs().will(returnValue(true));
    MOCKER_CPP(&File::Access)
        .stubs().will(returnValue(true));
    MOCKER_CPP(&FileWriter::IsOpen)
        .stubs().will(returnValue(true));
    ASSERT_EQ(ANALYSIS_OK, Log::GetInstance().Init("test"));
}

TEST_F(LogUTest, TestGetTimeShouldReturnCurrentTime)
{
    std::string currentTime = Log::GetTime();
    const std::string timeFormat = "yyyy-mm-dd HH:MM:SS";
    ASSERT_EQ(timeFormat.size(), currentTime.size());
}

TEST_F(LogUTest, TestLogMsg)
{
    GlobalMockObject::verify();
    INFO("Test: %", "LogMsg");
}

TEST_F(LogUTest, TestPrintMsg)
{
    GlobalMockObject::verify();
    const std::string currTime = "2023-11-15 00:00:00";
    MOCKER_CPP(&Log::GetTime)
        .stubs().will(returnValue(currTime));
    testing::internal::CaptureStdout();
    PRINT_INFO("Test: %", "PrintMsg");
    const std::string stdOut = testing::internal::GetCapturedStdout();
    const std::string out = Format("2023-11-15 00:00:00 [INFO] [%] log_utest.cpp: Test: PrintMsg\n", getpid());
    ASSERT_EQ(out, stdOut);
}

TEST_F(LogUTest, TestGetFileNameShouldReturnFileNameWhenInputPath)
{
    ASSERT_EQ("test", Log::GetFileName("/ut/log/test"));
}
