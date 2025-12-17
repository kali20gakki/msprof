/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "prof_manager.h"
#include "errno/error_code.h"
#include "platform.h"
#include "prof_params.h"
#include "platform_adapter.h"
#include "param_validation.h"
#include "utils.h"
#include "prof_task.h"
#include "uploader.h"
#include "uploader_mgr.h"

using namespace analysis::dvvp::host;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::transport;

class ProfManagerUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
        GlobalMockObject::verify();
    }
    virtual void TearDown() {}
};

TEST_F(ProfManagerUtest, AclInitWillReturnSuccWhenCalled)
{
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::SetPlatformSoc)
        .stubs();
    EXPECT_EQ(PROFILING_SUCCESS, ProfManager::instance()->AclInit());

    // repeat init
    EXPECT_EQ(PROFILING_SUCCESS, ProfManager::instance()->AclInit());
    ProfManager::instance()->AclUinit();
}

TEST_F(ProfManagerUtest, AclUinitWillReturnSuccWhenCalled)
{
    EXPECT_EQ(PROFILING_SUCCESS, ProfManager::instance()->AclUinit());
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::SetPlatformSoc)
        .stubs();
    ProfManager::instance()->AclInit();
    EXPECT_EQ(PROFILING_SUCCESS, ProfManager::instance()->AclUinit());
}

TEST_F(ProfManagerUtest, HandleProfilingParamsWillReturnNullWhenInputInvalidSampleconfig)
{
    EXPECT_EQ(nullptr, ProfManager::instance()->HandleProfilingParams(""));
}

TEST_F(ProfManagerUtest, HandleProfilingParamsWillReturnNullWhenPlatformAdapterInitFail)
{
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    MOCKER_CPP(&Collector::Dvvp::Common::PlatformAdapter::PlatformAdapter::Init)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    ProfManager::instance()->AclInit();
    EXPECT_EQ(nullptr, ProfManager::instance()->HandleProfilingParams(params->ToString()));
}

TEST_F(ProfManagerUtest, HandleProfilingParamsWillReturnNullWhenCheckProfilingParamsFail)
{
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    MOCKER_CPP(&Collector::Dvvp::Common::PlatformAdapter::PlatformAdapter::Init)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckProfilingParams)
        .stubs()
        .will(returnValue(false));
    ProfManager::instance()->AclInit();
    EXPECT_EQ(nullptr, ProfManager::instance()->HandleProfilingParams(params->ToString()));
}

TEST_F(ProfManagerUtest, HandleProfilingParamsWillReturnNullWhenStrToUint32Fail)
{
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    MOCKER_CPP(&Collector::Dvvp::Common::PlatformAdapter::PlatformAdapter::Init)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckProfilingParams)
        .stubs()
        .will(returnValue(true));
    MOCKER(analysis::dvvp::common::utils::Utils::StrToUint32)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    ProfManager::instance()->AclInit();
    EXPECT_EQ(nullptr, ProfManager::instance()->HandleProfilingParams(params->ToString()));
}

TEST_F(ProfManagerUtest, HandleProfilingParamsWillReturnNullWhenCreateSampleJsonFileFail)
{
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    params->result_dir = "x";
    MOCKER_CPP(&Collector::Dvvp::Common::PlatformAdapter::PlatformAdapter::Init)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckProfilingParams)
        .stubs()
        .will(returnValue(true));
    MOCKER(analysis::dvvp::common::utils::Utils::StrToUint32)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER(analysis::dvvp::common::utils::Utils::CreateDir)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    ProfManager::instance()->AclInit();
    EXPECT_EQ(nullptr, ProfManager::instance()->HandleProfilingParams(params->ToString()));
}

TEST_F(ProfManagerUtest, HandleProfilingParamsWillReturnParamsWhenCreateSampleJsonFileSucc)
{
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    params->job_id = "64";
    MOCKER_CPP(&Collector::Dvvp::Common::PlatformAdapter::PlatformAdapter::Init)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckProfilingParams)
        .stubs()
        .will(returnValue(true));
    MOCKER(analysis::dvvp::common::utils::Utils::CreateDir)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    ProfManager::instance()->AclInit();
    EXPECT_NE(nullptr, ProfManager::instance()->HandleProfilingParams(params->ToString()));

    params->hardware_mem = "on";
    EXPECT_NE(nullptr, ProfManager::instance()->HandleProfilingParams(params->ToString()));

    params->job_id = "1";
    EXPECT_NE(nullptr, ProfManager::instance()->HandleProfilingParams(params->ToString()));

    params->hardware_mem = "off";
    EXPECT_NE(nullptr, ProfManager::instance()->HandleProfilingParams(params->ToString()));
}

TEST_F(ProfManagerUtest, CreateSampleJsonFileWillReturnTrueWhenResultDirIsEmpty)
{
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    EXPECT_EQ(true, ProfManager::instance()->CreateSampleJsonFile(params, ""));
}

TEST_F(ProfManagerUtest, CreateSampleJsonFileWillReturnFalseWhenCreateDirFail)
{
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    MOCKER(analysis::dvvp::common::utils::Utils::CreateDir)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    EXPECT_EQ(false, ProfManager::instance()->CreateSampleJsonFile(params, "xx"));
}

TEST_F(ProfManagerUtest, CreateSampleJsonFileWillReturnFalseWhenWriteCtrlDataToFileFail)
{
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    MOCKER(analysis::dvvp::common::utils::Utils::CreateDir)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    MOCKER_CPP(&ProfManager::WriteCtrlDataToFile)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    EXPECT_EQ(false, ProfManager::instance()->CreateSampleJsonFile(params, "xx"));
}

TEST_F(ProfManagerUtest, CreateSampleJsonFileWillReturnTrueWhenWriteCtrlDataToFileSucc)
{
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    MOCKER_CPP(&ProfManager::WriteCtrlDataToFile)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(true, ProfManager::instance()->CreateSampleJsonFile(params, "xx"));
}

TEST_F(ProfManagerUtest, WriteCtrlDataToFileWillReturnDifferentValueWhenRunDifferently)
{
    std::string absolutePath = "./WriteCtrlDataToFileUtest";
    std::string data = "xxx";
    int dataLen = static_cast<int>(data.size());
    MOCKER(analysis::dvvp::common::utils::Utils::IsSoftLink)
        .stubs()
        .will(returnValue(true))
        .then(returnValue(false));
    MOCKER(analysis::dvvp::common::utils::Utils::IsFileExist)
        .stubs()
        .will(returnValue(true))
        .then(returnValue(false));
    EXPECT_EQ(PROFILING_FAILED, ProfManager::instance()->WriteCtrlDataToFile(absolutePath, data, dataLen));
    EXPECT_EQ(PROFILING_SUCCESS, ProfManager::instance()->WriteCtrlDataToFile(absolutePath, data, dataLen));
    EXPECT_EQ(PROFILING_FAILED, ProfManager::instance()->WriteCtrlDataToFile(absolutePath, "", dataLen));
    EXPECT_EQ(PROFILING_FAILED, ProfManager::instance()->WriteCtrlDataToFile(absolutePath, data, 0));

    // file open fail by input empty path
    EXPECT_EQ(PROFILING_FAILED, ProfManager::instance()->WriteCtrlDataToFile("", data, dataLen));
    MOCKER_CPP(&ProfManager::CreateDoneFile)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));
    EXPECT_EQ(PROFILING_FAILED, ProfManager::instance()->WriteCtrlDataToFile(absolutePath, data, dataLen));
    EXPECT_EQ(PROFILING_SUCCESS, ProfManager::instance()->WriteCtrlDataToFile(absolutePath, data, dataLen));
    std::remove(absolutePath.c_str());
}

TEST_F(ProfManagerUtest, CreateDoneFileWillReturnDifferentValueWhenRunDifferently)
{
    std::string absolutePath = "./CreateDoneFileUtest";
    std::string doneFilePath = "./CreateDoneFileUtest.done";
    std::string fileSize = "1";
    MOCKER(analysis::dvvp::common::utils::Utils::IsSoftLink)
        .stubs()
        .will(returnValue(true))
        .then(returnValue(false));
    EXPECT_EQ(false, ProfManager::instance()->CreateDoneFile(absolutePath, fileSize));

    // file open fail by input empty path
    EXPECT_EQ(false, ProfManager::instance()->CreateDoneFile("", fileSize));

    EXPECT_EQ(true, ProfManager::instance()->CreateDoneFile(absolutePath, fileSize));
    std::remove(doneFilePath.c_str());
    std::remove(absolutePath.c_str());
}

TEST_F(ProfManagerUtest, HandleWillReturnDifferentValueWhenRunDifferently)
{
    EXPECT_EQ(PROFILING_FAILED, ProfManager::instance()->Handle(nullptr));
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);

    // not init
    EXPECT_EQ(PROFILING_FAILED, ProfManager::instance()->Handle(params));
    
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::SetPlatformSoc)
        .stubs();
    params->job_id = "x";
    ProfManager::instance()->AclInit();
    params->host_profiling = false;
    MOCKER_CPP(&ProfManager::CheckIfDevicesOnline)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));
    MOCKER_CPP(&ProfManager::CheckHandleSuc)
        .stubs()
        .will(returnValue(true))
        .then(returnValue(false));
    MOCKER_CPP(&ProfManager::ProcessHandleFailed)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS))
        .then(returnValue(PROFILING_FAILED));
    EXPECT_EQ(PROFILING_FAILED, ProfManager::instance()->Handle(params));
    EXPECT_EQ(PROFILING_SUCCESS, ProfManager::instance()->Handle(params));
    EXPECT_EQ(PROFILING_FAILED, ProfManager::instance()->Handle(params));
}

TEST_F(ProfManagerUtest, LaunchTaskWillReturnFailWhenRunFail)
{
    std::string jobId = "1";
    std::string info;
    EXPECT_EQ(PROFILING_FAILED, ProfManager::instance()->LaunchTask(nullptr, jobId, info));

    std::vector<std::string> devices = {"0"};
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    SHARED_PTR_ALIA<analysis::dvvp::host::ProfTask> task;
    MSVP_MAKE_SHARED2_BREAK(task, analysis::dvvp::host::ProfTask, devices, params);

    // GetTaskNoLock fail
    ProfManager::instance()->_tasks.insert(std::make_pair(jobId, task));
    EXPECT_EQ(PROFILING_FAILED, ProfManager::instance()->LaunchTask(task, jobId, info));

    // input null params, so task init fail
    MSVP_MAKE_SHARED2_BREAK(task, analysis::dvvp::host::ProfTask, devices, nullptr);
    ProfManager::instance()->_tasks.clear();
    // task init fail
    EXPECT_EQ(PROFILING_FAILED, ProfManager::instance()->LaunchTask(task, jobId, info));
}

TEST_F(ProfManagerUtest, IdeCloudProfileProcessWillReturnFailWhenInputNull)
{
    EXPECT_EQ(PROFILING_FAILED, ProfManager::instance()->IdeCloudProfileProcess(nullptr));
}

TEST_F(ProfManagerUtest, CheckIfDevicesOnlineWillReturnTrueWhenDevicesIsAll)
{
    std::string statusInfo;
    EXPECT_EQ(true, ProfManager::instance()->CheckIfDevicesOnline("all", statusInfo));
}

TEST_F(ProfManagerUtest, CheckIfDevicesOnlineWillReturnFalseWhenPreGetDeviceListFail)
{
    MOCKER_CPP(&ProfManager::PreGetDeviceList)
        .stubs()
        .will(returnValue(false));
    std::string statusInfo;
    EXPECT_EQ(false, ProfManager::instance()->CheckIfDevicesOnline("1", statusInfo));
}

TEST_F(ProfManagerUtest, CheckIfDevicesOnlineWillReturnFalseWhenInputInvalidDevices)
{
    std::vector<int> preDevIds = {1};
    MOCKER_CPP(&ProfManager::PreGetDeviceList)
        .stubs()
        .with(outBound(preDevIds))
        .will(returnValue(true));
    std::string statusInfo;
    EXPECT_EQ(false, ProfManager::instance()->CheckIfDevicesOnline("xxxx", statusInfo));
}

TEST_F(ProfManagerUtest, CheckIfDevicesOnlineWillReturnFalseWhenStrToIntFail)
{
    std::vector<int> preDevIds = {1};
    MOCKER_CPP(&ProfManager::PreGetDeviceList)
        .stubs()
        .with(outBound(preDevIds))
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::utils::Utils::StrToInt)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    std::string statusInfo;
    EXPECT_EQ(false, ProfManager::instance()->CheckIfDevicesOnline("1", statusInfo));
}

TEST_F(ProfManagerUtest, CheckIfDevicesOnlineWillReturnFalseWhenInputDeviceNotSupport)
{
    std::vector<int> preDevIds = {0};
    MOCKER_CPP(&ProfManager::PreGetDeviceList)
        .stubs()
        .with(outBound(preDevIds))
        .will(returnValue(true));
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::RunSocSide)
        .stubs()
        .will(returnValue(false));
    std::string statusInfo;
    EXPECT_EQ(false, ProfManager::instance()->CheckIfDevicesOnline("1", statusInfo));
}

TEST_F(ProfManagerUtest, CheckIfDevicesOnlineWillReturnTrueWhenInputDeviceSupport)
{
    std::vector<int> preDevIds = {1};
    MOCKER_CPP(&ProfManager::PreGetDeviceList)
        .stubs()
        .with(outBound(preDevIds))
        .will(returnValue(true));
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::RunSocSide)
        .stubs()
        .will(returnValue(false));
    std::string statusInfo;
    EXPECT_EQ(true, ProfManager::instance()->CheckIfDevicesOnline("1", statusInfo));
}
