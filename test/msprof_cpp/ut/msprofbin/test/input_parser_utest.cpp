#include "mockcpp/mockcpp.hpp"
#include "gtest/gtest.h"
#include <iostream>
#include "errno/error_code.h"
#include "input_parser.h"
#include "config/config_manager.h"
#include "mmpa_plugin.h"

using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::Msprof;
using namespace Collector::Dvvp::Plugin;

class INPUT_PARSER_UTEST : public testing::Test {
protected:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

TEST_F(INPUT_PARSER_UTEST, ProcessOptions) {
    GlobalMockObject::verify();
    InputParser parser = InputParser();
    struct MsprofCmdInfo cmdInfo = { {nullptr} };
    // invalid options
    EXPECT_EQ(PROFILING_FAILED, parser.ProcessOptions(-1, cmdInfo));

    char* resArgs = "on";
    MOCKER(&MmpaPlugin::MsprofMmGetOptArg)
        .stubs()
        .will(returnValue(resArgs));
    MOCKER_CPP(&InputParser::MsprofHostCheckValid)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&InputParser::MsprofCmdCheckValid)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&InputParser::MsprofSwitchCheckValid)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&InputParser::MsprofFreqCheckValid)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    
    EXPECT_EQ(PROFILING_SUCCESS, parser.ProcessOptions(ARGS_OUTPUT, cmdInfo));
    EXPECT_EQ(PROFILING_SUCCESS, parser.ProcessOptions(ARGS_ASCENDCL, cmdInfo));
    EXPECT_EQ(PROFILING_SUCCESS, parser.ProcessOptions(ARGS_AIC_FREQ, cmdInfo));
    EXPECT_EQ(PROFILING_SUCCESS, parser.ProcessOptions(ARGS_HOST_SYS, cmdInfo));
    EXPECT_EQ(PROFILING_FAILED, parser.ProcessOptions(100, cmdInfo));
}


TEST_F(INPUT_PARSER_UTEST, MsprofHostCheckValid) {
    GlobalMockObject::verify();
    MOCKER(analysis::dvvp::common::utils::Utils::ExecCmd)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    InputParser parser = InputParser();
    struct MsprofCmdInfo cmdInfo = { {nullptr} };
    // invalid options
    EXPECT_EQ(PROFILING_FAILED, parser.MsprofHostCheckValid(cmdInfo, 999));

    EXPECT_EQ(PROFILING_FAILED, parser.MsprofHostCheckValid(cmdInfo, NR_ARGS));

    EXPECT_EQ(PROFILING_FAILED, parser.MsprofHostCheckValid(cmdInfo, ARGS_HOST_SYS));

    cmdInfo.args[ARGS_HOST_SYS] = "";
    EXPECT_EQ(PROFILING_FAILED, parser.MsprofHostCheckValid(cmdInfo, ARGS_HOST_SYS));

    cmdInfo.args[ARGS_HOST_SYS] = "cpu,mem";
    EXPECT_EQ(PROFILING_SUCCESS, parser.MsprofHostCheckValid(cmdInfo, ARGS_HOST_SYS));

    cmdInfo.args[ARGS_HOST_SYS] = "cpu,mem,network,osrt";
    EXPECT_EQ(PROFILING_FAILED, parser.MsprofHostCheckValid(cmdInfo, ARGS_HOST_SYS));

    cmdInfo.args[ARGS_HOST_SYS] = "cpu,mem,disk,network,osrt";
    parser.params_->result_dir = "./input_parser_utest";
    EXPECT_EQ(PROFILING_FAILED, parser.MsprofHostCheckValid(cmdInfo, ARGS_HOST_SYS));

    EXPECT_EQ(PROFILING_FAILED, parser.MsprofHostCheckValid(cmdInfo, ARGS_HOST_SYS_PID));
    cmdInfo.args[ARGS_HOST_SYS_PID] = "121312312123";
    EXPECT_EQ(PROFILING_FAILED, parser.MsprofHostCheckValid(cmdInfo, ARGS_HOST_SYS_PID));
}


TEST_F(INPUT_PARSER_UTEST, CheckHostSysCmdOutIsExist) {
    GlobalMockObject::verify();
    MOCKER(analysis::dvvp::common::utils::Utils::ExecCmd)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    InputParser parser = InputParser();
    std::string tempFile = "./CheckHostSysCmdOutIsExist";
    std::ofstream file(tempFile);
    file << "command not found" << std::endl;
    file.close();
    std::string toolName = "iotop";
    mmProcess tmpProcess = 1;
    // invalid options
    EXPECT_EQ(PROFILING_FAILED, parser.CheckHostSysCmdOutIsExist(tempFile, toolName, tmpProcess));
}

TEST_F(INPUT_PARSER_UTEST, CheckHostOutString) {
    GlobalMockObject::verify();
    InputParser parser = InputParser();
    std::string tmpStr = "";
    std::string toolName = "iotop";
    EXPECT_EQ(PROFILING_FAILED, parser.CheckHostOutString(tmpStr, toolName));
    tmpStr = "sudo";
    EXPECT_EQ(PROFILING_FAILED, parser.CheckHostOutString(tmpStr, toolName));
    tmpStr = "iotop";
    EXPECT_EQ(PROFILING_SUCCESS, parser.CheckHostOutString(tmpStr, toolName));
}

TEST_F(INPUT_PARSER_UTEST, UninitCheckHostSysCmd) {
    GlobalMockObject::verify();

    MOCKER(analysis::dvvp::common::utils::Utils::ExecCmd)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    MOCKER(analysis::dvvp::common::utils::Utils::ProcessIsRuning)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));

    MOCKER(analysis::dvvp::common::utils::Utils::WaitProcess)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    InputParser parser = InputParser();
    mmProcess checkProcess = 1;

    EXPECT_EQ(PROFILING_SUCCESS, parser.UninitCheckHostSysCmd(checkProcess));
    EXPECT_EQ(PROFILING_FAILED, parser.UninitCheckHostSysCmd(checkProcess));
    EXPECT_EQ(PROFILING_SUCCESS, parser.UninitCheckHostSysCmd(checkProcess));
}

TEST_F(INPUT_PARSER_UTEST, CheckOutputValid) {
    GlobalMockObject::verify();
    InputParser parser = InputParser();
    struct MsprofCmdInfo cmdInfo = { {nullptr} };

    EXPECT_EQ(PROFILING_FAILED, parser.CheckOutputValid(cmdInfo));
    cmdInfo.args[ARGS_OUTPUT] = "";
    EXPECT_EQ(PROFILING_FAILED, parser.CheckOutputValid(cmdInfo));
    cmdInfo.args[ARGS_OUTPUT] = "./";
    EXPECT_EQ(PROFILING_SUCCESS, parser.CheckOutputValid(cmdInfo));
}

TEST_F(INPUT_PARSER_UTEST, CheckAppValid) {
    GlobalMockObject::verify();
    InputParser parser = InputParser();
    struct MsprofCmdInfo cmdInfo = { {nullptr} };
    std::remove("./CheckAppValid");
    EXPECT_EQ(PROFILING_FAILED, parser.CheckAppValid(cmdInfo));
    cmdInfo.args[ARGS_APPLICATION] = "";
    EXPECT_EQ(PROFILING_FAILED, parser.CheckAppValid(cmdInfo));
    cmdInfo.args[ARGS_APPLICATION] = "./CheckAppValid a";
    EXPECT_EQ(PROFILING_FAILED, parser.CheckAppValid(cmdInfo));
    std::ofstream file("CheckAppValid");
    file << "command not found" << std::endl;
    file.close();
    EXPECT_EQ(PROFILING_SUCCESS, parser.CheckAppValid(cmdInfo));
    std::remove("./CheckAppValid");
}

TEST_F(INPUT_PARSER_UTEST, CheckEnvironmentValid) {
    GlobalMockObject::verify();
    InputParser parser = InputParser();
    struct MsprofCmdInfo cmdInfo = { {nullptr} };

    EXPECT_EQ(PROFILING_FAILED, parser.CheckEnvironmentValid(cmdInfo));
    cmdInfo.args[ARGS_ENVIRONMENT] = "";

    EXPECT_EQ(PROFILING_FAILED, parser.CheckEnvironmentValid(cmdInfo));
    cmdInfo.args[ARGS_ENVIRONMENT] = "aa";
    EXPECT_EQ(PROFILING_SUCCESS, parser.CheckEnvironmentValid(cmdInfo));
}

TEST_F(INPUT_PARSER_UTEST, CheckPythonPathValid) {
    GlobalMockObject::verify();
    InputParser parser = InputParser();
    struct MsprofCmdInfo cmdInfo = { {nullptr} };

    EXPECT_EQ(PROFILING_FAILED, parser.CheckPythonPathValid(cmdInfo));
    cmdInfo.args[ARGS_PYTHON_PATH] = "";

    EXPECT_EQ(PROFILING_FAILED, parser.CheckPythonPathValid(cmdInfo));
    
    parser.params_->pythonPath.clear();
    std::string tests = std::string(1025, 'c');
    char* test = const_cast<char*>(tests.c_str()); 
    cmdInfo.args[ARGS_PYTHON_PATH] = test;
    EXPECT_EQ(PROFILING_FAILED, parser.CheckPythonPathValid(cmdInfo));

    cmdInfo.args[ARGS_PYTHON_PATH] = "@";
    EXPECT_EQ(PROFILING_FAILED, parser.CheckPythonPathValid(cmdInfo));

    cmdInfo.args[ARGS_PYTHON_PATH] = "testpython";
    EXPECT_EQ(PROFILING_FAILED, parser.CheckPythonPathValid(cmdInfo));
    
    Utils::CreateDir("TestPython");
    MOCKER(&MmpaPlugin::MsprofMmAccess2).stubs().will(returnValue(-1)).then(returnValue(0));
    cmdInfo.args[ARGS_PYTHON_PATH] = "TestPython";
    EXPECT_EQ(PROFILING_FAILED, parser.CheckPythonPathValid(cmdInfo));
    EXPECT_EQ(PROFILING_FAILED, parser.CheckPythonPathValid(cmdInfo));
    Utils::RemoveDir("TestPython");
    cmdInfo.args[ARGS_PYTHON_PATH] = "testpython";
    std::ofstream ftest("testpython");
    ftest << "test";
    ftest.close();
    EXPECT_EQ(PROFILING_SUCCESS, parser.CheckPythonPathValid(cmdInfo));
    remove("testpython");
}

TEST_F(INPUT_PARSER_UTEST, ParamsCheck) {
    GlobalMockObject::verify();
    InputParser parser = InputParser();
    auto pp = parser.params_;
    parser.params_.reset();
    EXPECT_EQ(PROFILING_FAILED, parser.ParamsCheck());
    parser.params_ = pp;
    parser.params_->app_dir="./test";
    parser.params_->result_dir="";
    EXPECT_EQ(PROFILING_SUCCESS, parser.ParamsCheck());
    EXPECT_EQ(parser.params_->app_dir, parser.params_->result_dir);
}

TEST_F(INPUT_PARSER_UTEST, SetHostSysParam) {
    GlobalMockObject::verify();
    InputParser parser = InputParser();
    parser.SetHostSysParam("123");
    parser.SetHostSysParam("osrt");
}

TEST_F(INPUT_PARSER_UTEST, CheckHostSysValid) {
    GlobalMockObject::verify();

    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckHostSysOptionsIsValid)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));

    MOCKER_CPP(&Analysis::Dvvp::Msprof::InputParser::CheckHostSysToolsIsExist)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    InputParser parser = InputParser();
    struct MsprofCmdInfo cmdInfo = { {nullptr} };
    cmdInfo.args[ARGS_HOST_SYS] = "osrt,disk";
    EXPECT_EQ(PROFILING_FAILED, parser.CheckHostSysValid(cmdInfo));
    EXPECT_EQ(PROFILING_SUCCESS, parser.CheckHostSysValid(cmdInfo));
}

TEST_F(INPUT_PARSER_UTEST, CheckBaseInfo) {
    GlobalMockObject::verify();
    InputParser parser = InputParser();

    struct MsprofCmdInfo cmdInfo = { {nullptr} };

    EXPECT_EQ(PROFILING_FAILED, parser.CheckSampleModeValid(cmdInfo, ARGS_AIV_MODE));
    EXPECT_EQ(PROFILING_FAILED, parser.CheckSampleModeValid(cmdInfo, ARGS_AIC_MODE));
    cmdInfo.args[ARGS_AIV_MODE] = "aa";
    cmdInfo.args[ARGS_AIC_MODE] = "aa";
    EXPECT_EQ(PROFILING_FAILED, parser.CheckSampleModeValid(cmdInfo, ARGS_AIV_MODE));
    EXPECT_EQ(PROFILING_FAILED, parser.CheckSampleModeValid(cmdInfo, ARGS_AIC_MODE));
    cmdInfo.args[ARGS_AIV_MODE] = "sample-based";
    cmdInfo.args[ARGS_AIC_MODE] = "sample-based";
    EXPECT_EQ(PROFILING_SUCCESS, parser.CheckSampleModeValid(cmdInfo, ARGS_AIV_MODE));
    EXPECT_EQ(PROFILING_SUCCESS, parser.CheckSampleModeValid(cmdInfo, ARGS_AIC_MODE));

    // check aic metrics valid
    EXPECT_EQ(PROFILING_FAILED, parser.CheckAiCoreMetricsValid(cmdInfo, ARGS_AIC_METRICE));
    EXPECT_EQ(PROFILING_FAILED, parser.CheckAiCoreMetricsValid(cmdInfo, ARGS_AIV_METRICS));

    cmdInfo.args[ARGS_AIC_METRICE] = "";
    cmdInfo.args[ARGS_AIV_METRICS] = "";
    EXPECT_EQ(PROFILING_FAILED, parser.CheckAiCoreMetricsValid(cmdInfo, ARGS_AIC_METRICE));
    EXPECT_EQ(PROFILING_FAILED, parser.CheckAiCoreMetricsValid(cmdInfo, ARGS_AIV_METRICS));

    cmdInfo.args[ARGS_AIC_METRICE] = "1";
    cmdInfo.args[ARGS_AIV_METRICS] = "1";
    EXPECT_EQ(PROFILING_FAILED, parser.CheckAiCoreMetricsValid(cmdInfo, ARGS_AIC_METRICE));
    EXPECT_EQ(PROFILING_FAILED, parser.CheckAiCoreMetricsValid(cmdInfo, ARGS_AIV_METRICS));

    cmdInfo.args[ARGS_AIC_METRICE] = "Memory";
    cmdInfo.args[ARGS_AIV_METRICS] = "Memory";
    EXPECT_EQ(PROFILING_SUCCESS, parser.CheckAiCoreMetricsValid(cmdInfo, ARGS_AIC_METRICE));
    EXPECT_EQ(PROFILING_SUCCESS, parser.CheckAiCoreMetricsValid(cmdInfo, ARGS_AIV_METRICS));

    // check summary format
    EXPECT_EQ(PROFILING_FAILED, parser.CheckExportSummaryFormat(cmdInfo));
    cmdInfo.args[ARGS_SUMMARY_FORMAT] = "aaa";
    EXPECT_EQ(PROFILING_FAILED, parser.CheckExportSummaryFormat(cmdInfo));
    cmdInfo.args[ARGS_SUMMARY_FORMAT] = "csv";
    EXPECT_EQ(PROFILING_SUCCESS, parser.CheckExportSummaryFormat(cmdInfo));
    cmdInfo.args[ARGS_SUMMARY_FORMAT] = "json";
    EXPECT_EQ(PROFILING_SUCCESS, parser.CheckExportSummaryFormat(cmdInfo));

    // check llc
    EXPECT_EQ(PROFILING_FAILED, parser.CheckLlcProfilingVaild(cmdInfo));
    cmdInfo.args[ARGS_LLC_PROFILING] = "";
    EXPECT_EQ(PROFILING_FAILED, parser.CheckLlcProfilingVaild(cmdInfo));
    cmdInfo.args[ARGS_LLC_PROFILING] = "read";
    EXPECT_EQ(PROFILING_FAILED, parser.CheckLlcProfilingVaild(cmdInfo));
    cmdInfo.args[ARGS_LLC_PROFILING] = "capacity";
    EXPECT_EQ(PROFILING_SUCCESS, parser.CheckLlcProfilingVaild(cmdInfo));

    // check period
    EXPECT_EQ(PROFILING_FAILED, parser.CheckSysPeriodVaild(cmdInfo));
    cmdInfo.args[ARGS_SYS_PERIOD] = "capacity";
    EXPECT_EQ(PROFILING_FAILED, parser.CheckSysPeriodVaild(cmdInfo));
    cmdInfo.args[ARGS_SYS_PERIOD] = "-1";
    EXPECT_EQ(PROFILING_FAILED, parser.CheckSysPeriodVaild(cmdInfo));
    cmdInfo.args[ARGS_SYS_PERIOD] = "2";
    EXPECT_EQ(PROFILING_SUCCESS, parser.CheckSysPeriodVaild(cmdInfo));

    // check devices
    EXPECT_EQ(PROFILING_FAILED, parser.CheckSysDevicesVaild(cmdInfo));
    cmdInfo.args[ARGS_SYS_DEVICES] = "";
    EXPECT_EQ(PROFILING_FAILED, parser.CheckSysDevicesVaild(cmdInfo));
    cmdInfo.args[ARGS_SYS_DEVICES] = "all";
    EXPECT_EQ(PROFILING_SUCCESS, parser.CheckSysDevicesVaild(cmdInfo));
    cmdInfo.args[ARGS_SYS_DEVICES] = "A";
    EXPECT_EQ(PROFILING_FAILED, parser.CheckSysDevicesVaild(cmdInfo));

    // check arg on off
    EXPECT_EQ(PROFILING_FAILED, parser.CheckArgOnOff(cmdInfo, ARGS_ASCENDCL));
    cmdInfo.args[ARGS_ASCENDCL] = "A";
    EXPECT_EQ(PROFILING_FAILED, parser.CheckArgOnOff(cmdInfo, ARGS_ASCENDCL));

    // check arg range
    EXPECT_EQ(PROFILING_FAILED, parser.CheckArgRange(cmdInfo,ARGS_INTERCONNECTION_PROFILING, 1, 100));

    cmdInfo.args[ARGS_INTERCONNECTION_PROFILING] = "A";
    EXPECT_EQ(PROFILING_FAILED, parser.CheckArgRange(cmdInfo,ARGS_INTERCONNECTION_PROFILING, 1, 100));
    cmdInfo.args[ARGS_INTERCONNECTION_PROFILING] = "111";
    EXPECT_EQ(PROFILING_FAILED, parser.CheckArgRange(cmdInfo,ARGS_INTERCONNECTION_PROFILING, 1, 100));
    cmdInfo.args[ARGS_INTERCONNECTION_PROFILING] = "1";
    EXPECT_EQ(PROFILING_SUCCESS, parser.CheckArgRange(cmdInfo,ARGS_INTERCONNECTION_PROFILING, 1, 100));

    // check args is number
    EXPECT_EQ(PROFILING_FAILED, parser.CheckArgsIsNumber(cmdInfo, ARGS_EXPORT_ITERATION_ID));
    cmdInfo.args[ARGS_EXPORT_ITERATION_ID] = "a";
    EXPECT_EQ(PROFILING_FAILED, parser.CheckArgsIsNumber(cmdInfo, ARGS_EXPORT_ITERATION_ID));
    cmdInfo.args[ARGS_EXPORT_ITERATION_ID] = "1";
    EXPECT_EQ(PROFILING_SUCCESS, parser.CheckArgsIsNumber(cmdInfo, ARGS_EXPORT_ITERATION_ID));

    // params switch valid
    cmdInfo.args[ARGS_IO_PROFILING] = "1";
    cmdInfo.args[ARGS_MODEL_EXECUTION] = "1";
    cmdInfo.args[ARGS_RUNTIME_API] = "1";
    cmdInfo.args[ARGS_AI_CORE] = "1";
    cmdInfo.args[ARGS_AIV] = "1";
    cmdInfo.args[ARGS_CPU_PROFILING] = "1";
    cmdInfo.args[ARGS_SYS_PROFILING] = "1";
    cmdInfo.args[ARGS_PID_PROFILING] = "1";
    cmdInfo.args[ARGS_HARDWARE_MEM] = "1";
    cmdInfo.args[ARGS_INTERCONNECTION_PROFILING] = "1";
    cmdInfo.args[ARGS_DVPP_PROFILING] = "1";
    cmdInfo.args[ARGS_L2_PROFILING] = "1";
    cmdInfo.args[ARGS_AICPU] = "1";
    cmdInfo.args[ARGS_FFTS_BLOCK] = "1";
    cmdInfo.args[ARGS_LOW_POWER] = "1";
    cmdInfo.args[ARGS_DVPP_FREQ] = "1";
    cmdInfo.args[ARGS_IO_SAMPLING_FREQ] = "1";
    cmdInfo.args[ARGS_PID_SAMPLING_FREQ] = "1";
    cmdInfo.args[ARGS_SYS_SAMPLING_FREQ] = "1";
    cmdInfo.args[ARGS_CPU_SAMPLING_FREQ] = "1";
    cmdInfo.args[ARGS_HCCL] = "1";
    cmdInfo.args[ARGS_PARSE] = "1";
    cmdInfo.args[ARGS_QUERY] = "1";
    cmdInfo.args[ARGS_EXPORT] = "1";
    parser.ParamsSwitchValid(cmdInfo, ARGS_IO_PROFILING);
    parser.ParamsSwitchValid(cmdInfo, ARGS_MODEL_EXECUTION);
    parser.ParamsSwitchValid(cmdInfo, ARGS_RUNTIME_API);
    parser.ParamsSwitchValid(cmdInfo, ARGS_AI_CORE);
    parser.ParamsSwitchValid(cmdInfo, ARGS_AIV);
    parser.ParamsSwitchValid(cmdInfo, ARGS_CPU_PROFILING);
    parser.ParamsSwitchValid(cmdInfo, ARGS_SYS_PROFILING);
    parser.ParamsSwitchValid(cmdInfo, ARGS_PID_PROFILING);
    parser.ParamsSwitchValid(cmdInfo, ARGS_HARDWARE_MEM);
    parser.ParamsSwitchValid(cmdInfo, ARGS_HCCL);
    parser.ParamsSwitchValid(cmdInfo, 111);
    parser.ParamsSwitchValid2(cmdInfo, 111);
    parser.ParamsSwitchValid2(cmdInfo, ARGS_INTERCONNECTION_PROFILING);
    parser.ParamsSwitchValid2(cmdInfo, ARGS_DVPP_PROFILING);
    parser.ParamsSwitchValid2(cmdInfo, ARGS_L2_PROFILING);
    parser.ParamsSwitchValid2(cmdInfo, ARGS_AICPU);
    parser.ParamsSwitchValid2(cmdInfo, ARGS_FFTS_BLOCK);
    parser.ParamsSwitchValid2(cmdInfo, ARGS_LOW_POWER);
    parser.ParamsSwitchValid2(cmdInfo, ARGS_PARSE);
    parser.ParamsSwitchValid2(cmdInfo, ARGS_QUERY);
    parser.ParamsSwitchValid2(cmdInfo, ARGS_EXPORT);

    // ParamsFreqValid
    parser.ParamsFreqValid(cmdInfo, 1, 111);
    parser.ParamsFreqValid(cmdInfo, 1, ARGS_DVPP_FREQ);
    parser.ParamsFreqValid(cmdInfo, 1, ARGS_IO_SAMPLING_FREQ);
    parser.ParamsFreqValid(cmdInfo, 1, ARGS_PID_SAMPLING_FREQ);
    parser.ParamsFreqValid(cmdInfo, 1, ARGS_SYS_SAMPLING_FREQ);
    parser.ParamsFreqValid(cmdInfo, 1, ARGS_CPU_SAMPLING_FREQ);
}

TEST_F(INPUT_PARSER_UTEST, PreCheckPlatform) {
    InputParser parser = InputParser();
    struct MsprofCmdInfo cmdInfo = { {nullptr} };
    const char * argv[] = {"aiv-me"};
    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetPlatformType)
        .stubs()
        .will(returnValue(Analysis::Dvvp::Common::Config::PlatformType::END_TYPE))
        .then(returnValue(Analysis::Dvvp::Common::Config::PlatformType::DC_TYPE));
    EXPECT_EQ(PROFILING_FAILED, parser.PreCheckPlatform(ARGS_AIV, argv));
    MOCKER(&MmpaPlugin::MsprofMmGetOptInd)
        .stubs()
        .will(returnValue(1));
    parser.PreCheckPlatform(ARGS_AIV, argv);
}

TEST_F(INPUT_PARSER_UTEST, MsprofCmdCheckValid) {
    InputParser parser = InputParser();
    struct MsprofCmdInfo cmdInfo = { {nullptr} };
    cmdInfo.args[ARGS_AIV_MODE] = "sample-baseddddd";
    cmdInfo.args[ARGS_ACC_PMU_MODE] = "sample-baseddddd";
    cmdInfo.args[ARGS_AIC_METRICE] = "PipeUtilization";
    cmdInfo.args[ARGS_FFTS_BLOCK] = "aa";
    cmdInfo.args[ARGS_LOW_POWER] = "bb";
    cmdInfo.args[ARGS_SUMMARY_FORMAT] = "csv";
    cmdInfo.args[ARGS_PYTHON_PATH] = "123";
    MOCKER(&MmpaPlugin::MsprofMmGetOptInd)
        .stubs()
        .will(returnValue(1));
    parser.MsprofCmdCheckValid(cmdInfo, ARGS_AIV_MODE);
    parser.MsprofCmdCheckValid(cmdInfo, ARGS_ACC_PMU_MODE);
    parser.MsprofCmdCheckValid(cmdInfo, ARGS_AIC_METRICE);
    parser.MsprofCmdCheckValid(cmdInfo, ARGS_FFTS_BLOCK);
    parser.MsprofCmdCheckValid(cmdInfo, ARGS_LOW_POWER);
    parser.MsprofCmdCheckValid(cmdInfo, ARGS_STARS_ACSQ_TASK);
    parser.MsprofCmdCheckValid(cmdInfo, ARGS_SUMMARY_FORMAT);
    parser.MsprofCmdCheckValid(cmdInfo, ARGS_PYTHON_PATH);
}

TEST_F(INPUT_PARSER_UTEST, MsprofFreqCheckValid) {
    InputParser parser = InputParser();
    struct MsprofCmdInfo cmdInfo = { {nullptr} };
    EXPECT_EQ(PROFILING_FAILED, parser.MsprofFreqCheckValid(cmdInfo, 100));
    cmdInfo.args[ARGS_SYS_PERIOD] = "100";
    cmdInfo.args[ARGS_SYS_SAMPLING_FREQ] = "1";
    cmdInfo.args[ARGS_PID_SAMPLING_FREQ] = "1";
    cmdInfo.args[ARGS_CPU_SAMPLING_FREQ] = "10";
    cmdInfo.args[ARGS_INTERCONNECTION_FREQ] = "10";
    cmdInfo.args[ARGS_IO_SAMPLING_FREQ] = "60";
    cmdInfo.args[ARGS_DVPP_FREQ] = "60";
    cmdInfo.args[ARGS_HARDWARE_MEM_SAMPLING_FREQ] = "600";
    cmdInfo.args[ARGS_AIC_FREQ] = "20";
    cmdInfo.args[ARGS_AIV_FREQ] = "20";
    cmdInfo.args[ARGS_EXPORT_ITERATION_ID] = "1";
    cmdInfo.args[ARGS_EXPORT_MODEL_ID] = "1";

    EXPECT_EQ(PROFILING_SUCCESS, parser.MsprofFreqCheckValid(cmdInfo, ARGS_SYS_PERIOD));
    EXPECT_EQ(PROFILING_SUCCESS, parser.MsprofFreqCheckValid(cmdInfo, ARGS_SYS_SAMPLING_FREQ));
    EXPECT_EQ(PROFILING_SUCCESS, parser.MsprofFreqCheckValid(cmdInfo, ARGS_PID_SAMPLING_FREQ));
    EXPECT_EQ(PROFILING_SUCCESS, parser.MsprofFreqCheckValid(cmdInfo, ARGS_CPU_SAMPLING_FREQ));
    EXPECT_EQ(PROFILING_SUCCESS, parser.MsprofFreqCheckValid(cmdInfo, ARGS_INTERCONNECTION_FREQ));
    EXPECT_EQ(PROFILING_SUCCESS, parser.MsprofFreqCheckValid(cmdInfo, ARGS_IO_SAMPLING_FREQ));
    EXPECT_EQ(PROFILING_SUCCESS, parser.MsprofFreqCheckValid(cmdInfo, ARGS_DVPP_FREQ));
    EXPECT_EQ(PROFILING_SUCCESS, parser.MsprofFreqCheckValid(cmdInfo, ARGS_HARDWARE_MEM_SAMPLING_FREQ));
    EXPECT_EQ(PROFILING_SUCCESS, parser.MsprofFreqCheckValid(cmdInfo, ARGS_AIC_FREQ));
    EXPECT_EQ(PROFILING_SUCCESS, parser.MsprofFreqCheckValid(cmdInfo, ARGS_AIV_FREQ));
    EXPECT_EQ(PROFILING_SUCCESS, parser.MsprofFreqCheckValid(cmdInfo, ARGS_EXPORT_ITERATION_ID));
    EXPECT_EQ(PROFILING_SUCCESS, parser.MsprofFreqCheckValid(cmdInfo, ARGS_EXPORT_MODEL_ID));

}