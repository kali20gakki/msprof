#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include <fstream>
#include <iostream>
#include "utils/utils.h"
#include "errno/error_code.h"
#include "app/application.h"
#include "app/env_manager.h"
#include "message/prof_params.h"
#include "config/config_manager.h"
#include "message/codec.h"
#include "uploader_mgr.h"
#include "transport.h"
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::message;

class PROF_APPLICATION_TEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    
    }
};

/////////////////////////////////////////////////////////////
TEST_F(PROF_APPLICATION_TEST, PrepareLaunchAppCmd) {
	GlobalMockObject::verify();
	std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
		new analysis::dvvp::message::ProfileParams());

	std::stringstream ss_perf_cmd_app;
	params->app = "$$$";
	EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::app::Application::PrepareLaunchAppCmd(ss_perf_cmd_app, params));

	params->app_dir = "/appdir";
	EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::app::Application::PrepareLaunchAppCmd(ss_perf_cmd_app, params));
	params->app_parameters = "rm -rf";
	EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::app::Application::PrepareLaunchAppCmd(ss_perf_cmd_app, params));

	params->app = "test";
	params->app_parameters = "app_parameters";

	EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::app::Application::PrepareLaunchAppCmd(ss_perf_cmd_app, params));

	EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::app::Application::PrepareLaunchAppCmd(ss_perf_cmd_app, params));

	EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::app::Application::PrepareLaunchAppCmd(ss_perf_cmd_app, params));

	params->app_parameters = "app_parameters";
	EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::app::Application::PrepareLaunchAppCmd(ss_perf_cmd_app, params));
}

/////////////////////////////////////////////////////////////
TEST_F(PROF_APPLICATION_TEST, PrepareAppArgs) {
	GlobalMockObject::verify();

	std::vector<std::string> params;
	params.push_back("params1");
	params.push_back("params2");
	std::vector<std::string> args_v;
	analysis::dvvp::app::Application::PrepareAppArgs(params, args_v);
	std::string newStr = args_v[0];
	EXPECT_STREQ("params2", newStr.c_str());
}

/////////////////////////////////////////////////////////////
TEST_F(PROF_APPLICATION_TEST, PrepareAppEnvs) {
	GlobalMockObject::verify();
	
	std::string result_dir("./");
	std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
		new analysis::dvvp::message::ProfileParams());
	EXPECT_NE(nullptr, params);
	std::string cmd("/path/to/exe");
	std::string ai_core_events_str("ai_core_events_str");
	std::string dev_id("0");

	std::vector<std::string> envs_v;

	params->result_dir = "/tmp/PrepareAppEnvs";

    int ret = analysis::dvvp::app::Application::PrepareAppEnvs(nullptr, envs_v);
    EXPECT_EQ(PROFILING_FAILED, ret);
	ret = analysis::dvvp::app::Application::PrepareAppEnvs(params, envs_v);
	EXPECT_EQ(PROFILING_SUCCESS, ret);
	params->profiling_options = "task_trace";
	ret = analysis::dvvp::app::Application::PrepareAppEnvs(params, envs_v);
	EXPECT_EQ(PROFILING_SUCCESS, ret);
	ret = analysis::dvvp::app::Application::PrepareAppEnvs(params, envs_v);
	EXPECT_EQ(PROFILING_SUCCESS, ret);
}

int launchAppCmdCnt = 0;

int PrepareLaunchAppCmdStub(std::stringstream &ssCmdApp, SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    ssCmdApp << "test";
    launchAppCmdCnt++;
    if (launchAppCmdCnt == 1) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int CanonicalizePathStubCnt = 0;

std::string CanonicalizePathStub(const std::string &path)
{
	std::string tmp;
    CanonicalizePathStubCnt++;
    if (CanonicalizePathStubCnt == 1) {
        return tmp;
    }
	tmp = "test";
    return tmp;
}

/////////////////////////////////////////////////////////////
TEST_F(PROF_APPLICATION_TEST, LaunchApp) {
	GlobalMockObject::verify();
	
	std::string ai_core_events_str("ai_core_events_str");
	std::string result_dir("./");
	std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
		new analysis::dvvp::message::ProfileParams());
	std::string resultDir = "/tmp";
	params->app = "ls";
	pid_t app_process;

	MOCKER(analysis::dvvp::app::Application::PrepareLaunchAppCmd)
		.stubs()
		.will(invoke(PrepareLaunchAppCmdStub));

	MOCKER(analysis::dvvp::app::Application::PrepareAppArgs)
		.stubs()
		.will(ignoreReturnValue());

	MOCKER(analysis::dvvp::app::Application::PrepareAppEnvs)
		.stubs()
		.will(returnValue(PROFILING_FAILED))
		.then(returnValue(PROFILING_SUCCESS));
	FILE *fp = (FILE *)0x12345;

	MOCKER(analysis::dvvp::common::utils::Utils::CanonicalizePath)
        .stubs()
        .will(invoke(CanonicalizePathStub));

    MOCKER(analysis::dvvp::common::utils::Utils::IsFileExist)
        .stubs()
        .will(returnValue(true));
    
    MOCKER(analysis::dvvp::common::utils::Utils::IsSoftLink)
        .stubs()
        .will(returnValue(false));

	EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::app::Application::LaunchApp(nullptr, app_process));
	EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::app::Application::LaunchApp(params, app_process));
	
	params->app_dir = resultDir;
	EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::app::Application::LaunchApp(params, app_process));
	
	EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::app::Application::LaunchApp(params, app_process));
	//check file path failed
	EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::app::Application::LaunchApp(params, app_process));
	EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::app::Application::LaunchApp(params, app_process));

	EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::app::Application::LaunchApp(params, app_process));

	std::vector<std::string> paramsCmd;
	MOCKER(analysis::dvvp::common::utils::Utils::Split)
		.stubs()
		.will(returnValue(paramsCmd));
	EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::app::Application::LaunchApp(params, app_process));
}


TEST_F(PROF_APPLICATION_TEST, SetAppEnv) {
	GlobalMockObject::verify();
	std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
		new analysis::dvvp::message::ProfileParams());
	std::vector<std::string> envs;
	std::string cmd = "asda/";
	params->app_env = "LD_LIBRARY_PATH=123;asd=1;PATH=123";
	analysis::dvvp::app::Application::SetAppEnv(params, envs);
}

TEST_F(PROF_APPLICATION_TEST, SetGlobalEnv) {
	GlobalMockObject::verify();
	std::vector<std::string> envList;
	Analysis::Dvvp::App::EnvManager::instance()->SetGlobalEnv(envList);
	Analysis::Dvvp::App::EnvManager::instance()->GetGlobalEnv();
}
