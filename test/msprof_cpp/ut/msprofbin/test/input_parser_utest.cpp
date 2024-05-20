#include "mockcpp/mockcpp.hpp"
#include "gtest/gtest.h"
#include "errno/error_code.h"
#include "input_parser.h"
#include "config/config_manager.h"
#include "mmpa_api.h"

using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::Msprof;
using namespace Collector::Dvvp::Plugin;
using namespace Collector::Dvvp::Mmpa;

class INPUT_PARSER_UTEST : public testing::Test {
protected:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

TEST_F(INPUT_PARSER_UTEST, MsprofGetOpts_1)
{
    InputParser parser = InputParser();
    const char* myArray[] = {"msprof", "--application='bash test.sh'", "bash", "test.sh", "args"};
    const char** argv = myArray;
    int argc = 5;
	  MOCKER(MmGetOptLong)
		.stubs()
		.will(returnValue(100)); // 100 表示超过可以识别的参数范围
    EXPECT_EQ(nullptr, parser.MsprofGetOpts(argc, argv));
}

TEST_F(INPUT_PARSER_UTEST, MsprofGetOpts_2)
{
    InputParser parser = InputParser();
    const char* myArray[] = {"msprof", "--application='bash test.sh'", "bash", "test.sh", "args"};
    const char** argv = myArray;
    int argc = 5;
    MOCKER(MmGetOptLong)
    .stubs()
    .will(returnValue(0));
    EXPECT_EQ(nullptr, parser.MsprofGetOpts(argc, argv));
}

TEST_F(INPUT_PARSER_UTEST, MsprofGetOpts_3)
{
    InputParser parser = InputParser();
    const char* myArray[] = {"msprof", "--application='bash test.sh'", "--output='./"};
    const char** argv = myArray;
    int argc = 3;
    EXPECT_EQ(nullptr, parser.MsprofGetOpts(argc, argv));
}


TEST_F(INPUT_PARSER_UTEST, MsprofGetOpts_4)
{
    InputParser parser = InputParser();
    const char* myArray[] = {"msprof", "bash", "test.sh"};
    const char** argv = myArray;
    int argc = 3;
    EXPECT_EQ(nullptr, parser.MsprofGetOpts(argc, argv));
}

TEST_F(INPUT_PARSER_UTEST, MsprofGetOpts_5)
{
    GlobalMockObject::verify();
    InputParser parser = InputParser();
    const char* myArray[] = {"msprof", "bash", "test.sh"};
    const char** argv = myArray;
    int argc = 3;
    int opt = ARGS_APPLICATION;
	  MOCKER(MmGetOptLong)
		.stubs()
		.will(returnValue(opt));
    EXPECT_EQ(nullptr, parser.MsprofGetOpts(argc, argv));
}

const int MSPROF_DAEMON_ERROR = -1;
TEST_F(INPUT_PARSER_UTEST, MsprofGetOpts_6)
{
    GlobalMockObject::verify();
    InputParser parser = InputParser();
    const char* myArray[] = {"msprof", "bash", "test.sh"};
    const char** argv = myArray;
    int argc = 3;
    int opt = ARGS_SUMMARY_FORMAT;
	  MOCKER(MmGetOptLong)
		.stubs()
		.will(returnValue(MSPROF_DAEMON_ERROR));
    EXPECT_NE(nullptr, parser.MsprofGetOpts(argc, argv));
}
