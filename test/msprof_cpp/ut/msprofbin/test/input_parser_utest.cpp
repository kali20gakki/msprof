#include "mockcpp/mockcpp.hpp"
#include "gtest/gtest.h"
#include "errno/error_code.h"
#include "input_parser.h"
#include "config/config_manager.h"
#include "mmpa_api.h"
#include "params_adapter_msprof.h"

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

TEST_F(INPUT_PARSER_UTEST, MsprofGetOpts_help)
{
    GlobalMockObject::verify();
    InputParser parser = InputParser();
    const char* myArray[] = {"msprof"};
    const char** argv = myArray;
    int argc = 1;
    int opt = ARGS_HELP;

    // 设置期望值和返回值
    MOCKER(MmGetOptLong).stubs().will(returnValue(opt));
    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetPlatformType)
        .stubs()
        .will(returnValue(Analysis::Dvvp::Common::Config::PlatformType::CHIP_V4_1_0));
    EXPECT_NE(nullptr, parser.MsprofGetOpts(argc, argv));
}

TEST_F(INPUT_PARSER_UTEST, MsprofGetOptsWillReturnNullptrWhenCheckInputDataValidityFail)
{
    GlobalMockObject::verify();
    InputParser parser = InputParser();
    int argc = 1;
    EXPECT_EQ(nullptr, parser.MsprofGetOpts(argc, nullptr));
}

TEST_F(INPUT_PARSER_UTEST, HasHelpParamOnlyWillReturnFalseWhenParamsIsNull)
{
    GlobalMockObject::verify();
    InputParser parser = InputParser();
    parser.params_ == nullptr;
    EXPECT_EQ(false, parser.HasHelpParamOnly());
}

TEST_F(INPUT_PARSER_UTEST, HasHelpParamOnlyWillReturnTrueWhenParamsHasHelpOnly)
{
    GlobalMockObject::verify();
    InputParser parser = InputParser();
    parser.params_->usedParams.size() == 1;
    parser.params_->usedParams = {ARGS_HELP};
    EXPECT_EQ(true, parser.HasHelpParamOnly());
}

TEST_F(INPUT_PARSER_UTEST, HasHelpParamOnlyWillReturnFalseWhenParamsHasNoHelp)
{
    GlobalMockObject::verify();
    InputParser parser = InputParser();
    parser.params_->usedParams.size() == 1;
    parser.params_->usedParams = {ARGS_OUTPUT};
    EXPECT_EQ(false, parser.HasHelpParamOnly());
}

TEST_F(INPUT_PARSER_UTEST, HasHelpParamOnlyWillReturnFalseWhenParamsHasMoreThanHelp)
{
    GlobalMockObject::verify();
    InputParser parser = InputParser();
    parser.params_->usedParams.size() == 2; // 2 : help + output
    parser.params_->usedParams = {ARGS_HELP, ARGS_OUTPUT};
    EXPECT_EQ(false, parser.HasHelpParamOnly());
}

TEST_F(INPUT_PARSER_UTEST, CheckInstrAndTaskParamBothSetWillReturnFalseWhenNotSetInstrSwitch)
{
    GlobalMockObject::verify();
    InputParser parser = InputParser();
    std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> argvMap;
    EXPECT_EQ(false, parser.CheckInstrAndTaskParamBothSet(argvMap));
}

TEST_F(INPUT_PARSER_UTEST, CheckInstrAndTaskParamBothSetWillReturnFalseWhenSetInstrSwitchAndNotSetTaskParams)
{
    GlobalMockObject::verify();
    InputParser parser = InputParser();
    std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> argvMap;
    struct MsprofCmdInfo info = { {nullptr} };
    info.args[ARGS_INSTR_PROFILING] = "on";
    argvMap.insert(std::make_pair(ARGS_INSTR_PROFILING, std::make_pair(info, "instr-profiling")));
    EXPECT_EQ(false, parser.CheckInstrAndTaskParamBothSet(argvMap));
}

TEST_F(INPUT_PARSER_UTEST, CheckInstrAndTaskParamBothSetWillReturnTrueWhenSetInstrSwitchAndOtherTaskParams)
{
    GlobalMockObject::verify();
    InputParser parser = InputParser();
    std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> argvMap;
    struct MsprofCmdInfo info = { {nullptr} };
    info.args[ARGS_INSTR_PROFILING] = "on";
    info.args[ARGS_TASK_TIME] = "on";
    info.args[ARGS_AIC_FREQ] = "10";
    info.args[ARGS_AIC_MODE] = "task-based";
    info.args[ARGS_AIC_METRICE] = "PipeUtilization";
    argvMap.insert(std::make_pair(ARGS_INSTR_PROFILING, std::make_pair(info, "instr-profiling")));
    argvMap.insert(std::make_pair(ARGS_TASK_TIME, std::make_pair(info, "task-time")));
    argvMap.insert(std::make_pair(ARGS_AIC_FREQ, std::make_pair(info, "aic-freq")));
    argvMap.insert(std::make_pair(ARGS_AIC_MODE, std::make_pair(info, "aic-mode")));
    argvMap.insert(std::make_pair(ARGS_AIC_METRICE, std::make_pair(info, "aic-metrics")));
    EXPECT_EQ(true, parser.CheckInstrAndTaskParamBothSet(argvMap));
}

TEST_F(INPUT_PARSER_UTEST, CheckInputDataValidityWillReturnFalseWhenParamsIsNotSet)
{
    GlobalMockObject::verify();
    InputParser parser = InputParser();
    const char* myArray[] = {"msprof"};
    const char** argv = myArray;
    int argc = 1;
    parser.params_ = nullptr;
    EXPECT_EQ(false, parser.CheckInputDataValidity(argc, argv));
}

TEST_F(INPUT_PARSER_UTEST, CheckInputDataValidityWillReturnFalseWhenInputInvalidArgc)
{
    GlobalMockObject::verify();
    InputParser parser = InputParser();
    const char* myArray[] = {"msprof"};
    const char** argv = myArray;
    int argc = INPUT_MAX_LENTH + 1;
    EXPECT_EQ(false, parser.CheckInputDataValidity(argc, argv));
}

TEST_F(INPUT_PARSER_UTEST, CheckInputDataValidityWillReturnFalseWhenInputNullArgv)
{
    GlobalMockObject::verify();
    InputParser parser = InputParser();
    int argc = 1;
    EXPECT_EQ(false, parser.CheckInputDataValidity(argc, nullptr));
}

TEST_F(INPUT_PARSER_UTEST, CheckInputDataValidityWillReturnFalseWhenInputInvalidArgvValue)
{
    GlobalMockObject::verify();
    InputParser parser = InputParser();
    int argc = 1;
    int invalidLen = INPUT_MAX_LENTH + 1;
    char invalid[1025] = {};
    for (int i = 0; i < invalidLen; i++) {
        invalid[i] = 'x';
    }
    invalid[INPUT_MAX_LENTH] = '\0';
    const char* myArray[] = {invalid};
    const char** argv = myArray;
    EXPECT_EQ(false, parser.CheckInputDataValidity(argc, argv));
}

TEST_F(INPUT_PARSER_UTEST, CheckInputDataValidityWillReturnTrueWhenInputValidArgcAndArgv)
{
    GlobalMockObject::verify();
    InputParser parser = InputParser();
    const char* myArray[] = {"msprof"};
    const char** argv = myArray;
    int argc = 1;
    EXPECT_EQ(true, parser.CheckInputDataValidity(argc, argv));
}