#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include <fstream>
#include <iostream>
#include <sys/inotify.h>
#include "prof_host_core.h"
#include "proto/msprofiler.pb.h"
#include <google/protobuf/util/json_util.h>
#include "utils/utils.h"
#include "message/codec.h"
#include "message/prof_params.h"
#include "prof_inotify.h"
#include "prof_manager.h"
#include "msprof_dlog.h"
#include "prof_params_adapter.h"
#include "config/config_manager.h"

#ifndef false
#define false                     0
#endif

#ifndef true
#define true                      1
#endif

using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::host;

class PROF_INOTIFY_UTEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
        GlobalMockObject::verify();
    }

    void InotifyInitMock(std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify) {
    MOCKER(inotify_init)
        .stubs()
        .will(returnValue(0));

    MOCKER(analysis::dvvp::common::utils::Utils::ReadIniBlockByPath)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    MOCKER(analysis::dvvp::common::utils::Utils::RemoveDir)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    //InotifyEventsInit ClearWatchFile
    MOCKER(analysis::dvvp::common::utils::Utils::CreateDir)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    MOCKER(inotify_add_watch)
        .stubs()
        .then(returnValue(1));

    inotify->Init();
    }

    void InotifyCloudInitMock(std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify) {
        InotifyInitMock(inotify);
        char event_buf[MAX_EVENT_BUFFER_SIZE] = {0};
        struct inotify_event *event = (struct inotify_event *)(event_buf);
        event->wd = 1;
        std::string eventName = "JOBAAAAAAAAAAAAAC" + std::string(INOTIFY_START_EVENT);
        event->len = eventName.size();
        event->mask = IN_CLOSE_WRITE;
        strcpy(event->name, eventName.c_str());
        MOCKER_CPP(&analysis::dvvp::host::ProfInotify::ReadFileByPath)
            .stubs()
            .will(returnValue(PROFILING_SUCCESS));

        MOCKER_CPP(&analysis::dvvp::host::ProfInotify::CloudTask::Init)
            .stubs()
            .will(returnValue(PROFILING_SUCCESS));

        // MOCKER_CPP(&analysis::dvvp::host::ProfInotify::CloudTask::Start)
        // .stubs()
        // .will(returnValue(0));
        MOCKER(mmCreateTaskWithThreadAttr)
            .stubs()
            .will(returnValue(PROFILING_SUCCESS));

        inotify->InotifyCloseEventsProcess(*event);
    }
};

TEST_F(PROF_INOTIFY_UTEST, DeviceResponse)
{
    std::shared_ptr<std::string> jobId(new std::string("abcd"));
    std::shared_ptr<std::string> value(new std::string("0"));
    EXPECT_NE(nullptr, jobId);
    EXPECT_NE(nullptr, value);
    const std::string path = "./tmp";
    MOCKER_CPP(&analysis::dvvp::host::ProfManager::GetPodNameDir)
        .stubs()
        .will(returnValue(path));
    //device id
    analysis::dvvp::host::DeviceResponse(1, *(jobId.get()), *(value.get()));
}

TEST_F(PROF_INOTIFY_UTEST, WriteValueToFile)
{
    std::string path = "WriteValueToFileUtest";
    std::string value = "0";
    analysis::dvvp::host::WriteValueToFile(path, value);
    remove(path.c_str());

    MOCKER(mmChmod)
        .stubs()
        .will(returnValue(-1));
	EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::host::WriteValueToFile(path, value));
	remove(path.c_str());

    std::string installForAll = "1";
    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetInstallMode)
        .stubs()
        .will(returnValue(installForAll));
    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::host::WriteValueToFile(path, value));
	remove(path.c_str());
}


TEST_F(PROF_INOTIFY_UTEST, CloudTaskInit)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify::CloudTask> task
        (new analysis::dvvp::host::ProfInotify::CloudTask());
    analysis::dvvp::host::ProfSampleCfgT cfg;
    cfg.defCfg="{\"profiling_mode\": \"def_mode\", \
        \"system_wide\": \"off\", \"job_id\": \"a4dcbef99afc\", \"trace_id\":\"b77b2306\", \
        \"ai_core_profiling\": \"off\", \"result_dir\": \"/opt/davinci/log/profiling/\", \
        \"aicore_sampling_interval\": 10, \"ai_core_profiling_mode\": \"task-based\", \
        \"ts_timeline\": \"on\", \
        \"ts_task_track\": \"off\", \"devices\": \"0\", \"ai_core_profiling_events\": \"0x3,0x8,0x9,0xe,0x3a,0x3b,0x4a,0x49\", \
        \"is_cancel\": false, \"ai_core_status\": \"off\", \"hwts_profiling\": \"off\"}";
    std::string ucfg = "{ \"job_id\": \"RTJXZERIUZKGZMNYLKRAQQYDAXGDTH\", \
     \"ts_fw_training\": \"on\", \"task_base\": \"on\", \
     \"hwts_log\": \"on\", \"ts_timeline\": \"on\" }";

    const std::string invalid;
    const std::string path = "/tmp/PROFILER_UT/";
    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetDataDir)
        .stubs()
        .will(returnValue(invalid))
        .then(returnValue(path));

    MOCKER(analysis::dvvp::common::utils::Utils::CreateDir)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS))
        .then(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    //ReadIniBlockByPath failed
    EXPECT_EQ(PROFILING_FAILED, task->Init(cfg, invalid));
    // GetDataDir failed
    EXPECT_EQ(PROFILING_FAILED, task->Init(cfg, ucfg));
    // Create jobDataPath failed
    EXPECT_EQ(PROFILING_FAILED, task->Init(cfg, ucfg));
    // Craete dataPath failed
    EXPECT_EQ(PROFILING_FAILED, task->Init(cfg, ucfg));
    // ok
    EXPECT_EQ(PROFILING_SUCCESS, task->Init(cfg, ucfg));
}

TEST_F(PROF_INOTIFY_UTEST, CloudTaskUinit)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify::CloudTask> task
        (new analysis::dvvp::host::ProfInotify::CloudTask());

    MOCKER(mmJoinTask)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    // MOCKER_CPP(&analysis::dvvp::host::ProfInotify::CloudTask::Stop)
    //     .stubs()
    //     .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::CloudTask::GenerateCancelConfig)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::CloudTask::CloudTaskProcess)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    //ReadIniBlockByPath failed
    EXPECT_EQ(PROFILING_FAILED, task->Uinit());
    EXPECT_EQ(PROFILING_FAILED, task->Uinit());
    EXPECT_EQ(PROFILING_SUCCESS, task->Uinit());
}

TEST_F(PROF_INOTIFY_UTEST, GenerateSampleConfig)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify::CloudTask> task(new analysis::dvvp::host::ProfInotify::CloudTask());
    std::string ucfg = "{\"profiling_mode\": \"def_mode\", \
                        \"system_wide\": \"off\", \"job_id\": \"a4dcbef99afc\", \"trace_id\":\"b77b2306\", \
                        \"ai_core_profiling\": \"off\", \"result_dir\": \"/opt/davinci/log/profiling/\", \
                        \"aicore_sampling_interval\": 10, \"ai_core_profiling_mode\": \"task-based\", \
                        \"ts_timeline\": \"on\", \
                        \"ts_task_track\": \"off\", \"devices\": \"0\", \"ai_core_profiling_events\": \"0x3,0x8,0x9,0xe,0x3a,0x3b,0x4a,0x49\", \
                        \"is_cancel\": false, \"ai_core_status\": \"off\", \"hwts_profiling\": \"off\"}";
    std::string vcfg;
    //ReadIniBlockByPath failed
    EXPECT_EQ(PROFILING_FAILED, task->GenerateSampleConfig(vcfg));
    EXPECT_EQ(PROFILING_SUCCESS, task->GenerateSampleConfig(ucfg));
}


TEST_F(PROF_INOTIFY_UTEST, GenerateCancelConfig)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify::CloudTask> task
        (new analysis::dvvp::host::ProfInotify::CloudTask());
    std::string ucfg = "{\"profiling_mode\": \"def_mode\", \
        \"system_wide\": \"off\", \"job_id\": \"a4dcbef99afc\", \"trace_id\":\"b77b2306\", \
        \"ai_core_profiling\": \"off\", \"result_dir\": \"/opt/davinci/log/profiling/\", \
        \"aicore_sampling_interval\": 10, \"ai_core_profiling_mode\": \"task-based\", \
        \"ts_timeline\": \"on\", \
        \"ts_task_track\": \"off\", \"devices\": \"0\", \"ai_core_profiling_events\": \"0x3,0x8,0x9,0xe,0x3a,0x3b,0x4a,0x49\", \
        \"is_cancel\": false, \"ai_core_status\": \"off\", \"hwts_profiling\": \"off\"}";
    std::string vcfg;
    //ReadIniBlockByPath failed
    EXPECT_EQ(PROFILING_FAILED, task->GenerateCancelConfig(vcfg));
    EXPECT_EQ(PROFILING_SUCCESS, task->GenerateCancelConfig(ucfg));
}


TEST_F(PROF_INOTIFY_UTEST, CloudTaskProcess)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify::CloudTask> task
        (new analysis::dvvp::host::ProfInotify::CloudTask());
    std::string cfg = "{\"profiling_mode\": \"def_mode\", \
        \"system_wide\": \"off\", \"job_id\": \"a4dcbef99afc\", \"trace_id\":\"b77b2306\", \
        \"ai_core_profiling\": \"off\", \"result_dir\": \"/opt/davinci/log/profiling/\", \
        \"aicore_sampling_interval\": 10, \"ai_core_profiling_mode\": \"task-based\", \
        \"ts_timeline\": \"on\", \
        \"ts_task_track\": \"off\", \"devices\": \"0\", \"ai_core_profiling_events\": \"0x3,0x8,0x9,0xe,0x3a,0x3b,0x4a,0x49\", \
        \"is_cancel\": false, \"ai_core_status\": \"off\", \"hwts_profiling\": \"off\"}";
    std::string vcfg;
    //ReadIniBlockByPath failed
    EXPECT_EQ(PROFILING_FAILED, task->CloudTaskProcess(vcfg));
    EXPECT_EQ(PROFILING_FAILED, task->CloudTaskProcess(cfg));
}

TEST_F(PROF_INOTIFY_UTEST, CloudTaskRun)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify::CloudTask> task
        (new analysis::dvvp::host::ProfInotify::CloudTask());
    EXPECT_NE(nullptr, task);

    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::CloudTask::GenerateSampleConfig)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::CloudTask::CloudTaskProcess)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    //ReadIniBlockByPath failed
    task->Run();
    task->Run();
    task->Run();
    task->Run();
}


TEST_F(PROF_INOTIFY_UTEST, InotifyEnable)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());
    MOCKER(analysis::dvvp::common::utils::Utils::ReadIniBlockByPath)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    //ReadIniBlockByPath failed
    EXPECT_FALSE(inotify->InotifyEnable());
    //startBlock.value.compare false
    EXPECT_FALSE(inotify->InotifyEnable());
}

TEST_F(PROF_INOTIFY_UTEST, InotifyGetDataStoreAbility)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());
    MOCKER(analysis::dvvp::common::utils::Utils::ReadIniBlockByPath)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS))
        .then(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    //ReadIniBlockByPath failed
    EXPECT_FALSE(inotify->InotifyGetDataStoreAbility());
    //startBlock.value.compare false
    EXPECT_FALSE(inotify->InotifyGetDataStoreAbility());
    EXPECT_TRUE(inotify->InotifyGetDataStoreAbility());
}

TEST_F(PROF_INOTIFY_UTEST, ReadFileByPathVliadPath)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());
    std::string vpath;
    std::string cfg;
    //ReadFileByPath failed
   EXPECT_EQ(PROFILING_FAILED, inotify->ReadFileByPath(vpath, cfg));
}

TEST_F(PROF_INOTIFY_UTEST, ReadFileByPath)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());
    std::string path = "prof_inotify_read_utest";
    std::ofstream out;
    out.open(path, std::ios::out | std::ios::trunc);
    if (out.is_open()) {
        out.close();
    }
    std::string vpath;
    std::string cfg;
    //ReadFileByPath failed
    EXPECT_EQ(PROFILING_FAILED, inotify->ReadFileByPath(vpath, cfg));
    EXPECT_EQ(PROFILING_FAILED, inotify->ReadFileByPath(path, cfg));
    out.open(path, std::ios::out | std::ios::trunc);
    if (out.is_open()) {
        out << "12345" << std::endl << std::flush;
        out.close();
    }
    EXPECT_EQ(PROFILING_SUCCESS, inotify->ReadFileByPath(path, cfg));
    remove(path.c_str());
}

TEST_F(PROF_INOTIFY_UTEST, Uninit)
{

    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());
    MOCKER(inotify_init)
        .stubs()
        .will(returnValue(0));
    const std::string path = "/tmp/PROFILER_UT/";
    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetInotifyDir)
        .stubs()
        .will(returnValue(path));

    MOCKER(analysis::dvvp::common::utils::Utils::ReadIniBlockByPath)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    MOCKER(analysis::dvvp::common::utils::Utils::RemoveDir)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    //InotifyEventsInit ClearWatchFile
    MOCKER(analysis::dvvp::common::utils::Utils::CreateDir)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    MOCKER(inotify_add_watch)
        .stubs()
        .then(returnValue(1));

    MOCKER_CPP(&analysis::dvvp::message::ProfileParams::FromString)
        .stubs()
        .will(returnValue(true));

    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::GetInotifyStopped)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));

    MOCKER_CPP(&Analysis::Dvvp::Host::Adapter::ProfParamsAdapter::Init)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    EXPECT_EQ(PROFILING_FAILED, inotify->Init());
    EXPECT_EQ(PROFILING_SUCCESS, inotify->Init());
}

TEST_F(PROF_INOTIFY_UTEST, InotifyDeleteDir)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());

    char event_buf[MAX_EVENT_BUFFER_SIZE] = {0};
    struct inotify_event *event = (struct inotify_event*)(event_buf);
    strcpy(event->name,"..");
    EXPECT_EQ(PROFILING_FAILED, inotify->InotifyDeleteDir(*event));
    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::InotifyCreateDefaultPodDir)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    strcpy(event->name, "prof_default_inotify_dir");
    EXPECT_EQ(PROFILING_SUCCESS, inotify->InotifyDeleteDir(*event));
    strcpy(event->name, "test");
    EXPECT_EQ(PROFILING_SUCCESS, inotify->InotifyDeleteDir(*event));
}

TEST_F(PROF_INOTIFY_UTEST, Init)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());
    MOCKER(inotify_init)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));
     MOCKER(analysis::dvvp::common::utils::Utils::IsFileExist)
        .stubs()
        .will(returnValue(true));
    std::string path1 = "/var/log/npu/conf/profiling/profile.cfg";
    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetProfCfgPath)
        .stubs()
        .will(returnValue(path1));

    const std::string path = "/tmp/PROFILER_UT/";
    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetInotifyDir)
        .stubs()
        .will(returnValue(path));

    MOCKER(analysis::dvvp::common::utils::Utils::ReadIniBlockByPath)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    MOCKER(analysis::dvvp::common::utils::Utils::RemoveDir)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    //InotifyEventsInit ClearWatchFile
    MOCKER(analysis::dvvp::common::utils::Utils::CreateDir)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS))
        .then(returnValue(PROFILING_SUCCESS))
        .then(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    MOCKER(inotify_add_watch)
        .stubs()
        .will(returnValue(1));

    MOCKER_CPP(&analysis::dvvp::message::ProfileParams::FromString)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));
    MOCKER_CPP(&Analysis::Dvvp::Host::Adapter::ProfParamsAdapter::Init)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    //inotifyRootFd_ error
    EXPECT_EQ(PROFILING_FAILED, inotify->Init());
    //InotifyStartCfgProcess error
    EXPECT_EQ(PROFILING_FAILED, inotify->Init());
    //InotifyEventsInit AddWatchFile event error
    EXPECT_EQ(PROFILING_FAILED, inotify->Init());
    //InotifyEventsInit ProfileParams::FromString error
    EXPECT_EQ(PROFILING_FAILED, inotify->Init());
    //InotifyEventsInit AddWatchFile data error
    EXPECT_EQ(PROFILING_SUCCESS, inotify->Init());

    EXPECT_EQ(PROFILING_SUCCESS, inotify->Init());
    EXPECT_EQ(PROFILING_FAILED, inotify->Init());
}

TEST_F(PROF_INOTIFY_UTEST, InotifyEventsInit)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());

    std::string scfg = "{\"profiling_mode\": \"def_mode\", \
        \"system_wide\": \"off\", \"job_id\": \"a4dcbef99afc\", \"trace_id\":\"b77b2306\", \
        \"ai_core_profiling\": \"off\", \"result_dir\": \"/opt/davinci/log/profiling/\", \
        \"aicore_sampling_interval\": 10, \"ai_core_profiling_mode\": \"task-based\", \
        \"ts_timeline\": \"on\", \
        \"ts_task_track\": \"off\", \"devices\": \"0\", \"ai_core_profiling_events\": \"0x3,0x8,0x9,0xe,0x3a,0x3b,0x4a,0x49\", \
        \"is_cancel\": false, \"ai_core_status\": \"off\", \"hwts_profiling\": \"off\"}";

    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::ClearWatchFile)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    //inotifyRootFd_ error
    EXPECT_EQ(PROFILING_FAILED, inotify->InotifyEventsInit(-1, std::string("/tmp")));
    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::AddWatchFile)
            .stubs()
            .will(returnValue(0))
            .then(returnValue(0))
            .then(returnValue(-1));
    EXPECT_EQ(PROFILING_SUCCESS, inotify->InotifyEventsInit(-1, std::string("/tmp")));

    inotify->defSampleCfg_.defCfg = scfg;
    EXPECT_EQ(PROFILING_SUCCESS, inotify->InotifyEventsInit(-1, std::string("/tmp")));

}

TEST_F(PROF_INOTIFY_UTEST, ClearWatchFile)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());

    std::string dirName = "/tmp/ClearWatchFile";
    analysis::dvvp::common::utils::Utils::CreateDir(dirName);

    //inotifyRootFd_ error
    EXPECT_EQ(PROFILING_SUCCESS, inotify->ClearWatchFile(dirName));
    analysis::dvvp::common::utils::Utils::RemoveDir(dirName);
}

TEST_F(PROF_INOTIFY_UTEST, RmWatchEvent)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());
    //inotifyRootFd_ error
    EXPECT_EQ(PROFILING_FAILED, inotify->RmWatchEvent(-1, 0));
    //inotifyRootWd_ error
    EXPECT_EQ(PROFILING_FAILED, inotify->RmWatchEvent(-0, -1));

    EXPECT_EQ(PROFILING_FAILED, inotify->RmWatchEvent(1, 1));
}

TEST_F(PROF_INOTIFY_UTEST, AddWatchFile)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());
    //inotifyRootFd_ error
    EXPECT_EQ(PROFILING_FAILED, inotify->AddWatchFile(-1, std::string("/tmp"), 0));
       MOCKER(analysis::dvvp::common::utils::Utils::CreateDir)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
     MOCKER(mmChmod)
        .stubs()
        .will(returnValue(-1));

    EXPECT_EQ(PROFILING_FAILED, inotify->AddWatchFile(1, std::string("/tmp"), 0));
}

TEST_F(PROF_INOTIFY_UTEST, ReadEvents)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());
    char buf[10] = {0};
    //inotifyRootFd_ error
    EXPECT_EQ(PROFILING_FAILED, inotify->ReadEvents(-1, NULL, 0));
    EXPECT_EQ(PROFILING_FAILED, inotify->ReadEvents(1, NULL, 3));
    EXPECT_EQ(PROFILING_FAILED, inotify->ReadEvents(2, buf, 0));
}


TEST_F(PROF_INOTIFY_UTEST, run)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());
    EXPECT_NE(nullptr, inotify);

    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::NotifyEvents)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS))
        .then(returnValue(PROFILING_FAILED));
    
    //IdeCloudDeviceTransInit
    inotify->Run();
    //NotifyEvents
    inotify->Run();
    //inotify error
    inotify->Uinit();
    inotify->Run();
    inotify->Run();
}

int pool_stub_num = 0;
int pool_stub(struct pollfd *fds, unsigned int nfds, int timeout) {
    fds[0].revents = POLLRDNORM;
    fds[1].revents = POLLRDNORM;
    pool_stub_num++;
    if (pool_stub_num == 1) {
        return -1;
    } else if (pool_stub_num == 2 || pool_stub_num == 3) {
        return 0;
    } else {
        return 1;
    }
}

TEST_F(PROF_INOTIFY_UTEST, NotifyEvents)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());
    MOCKER(poll)
        .stubs()
        .will(invoke(pool_stub));
    MOCKER(read)
        .stubs()
        .will(returnValue(-1));

    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::InotifyBeatProcess)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS))
        .then(returnValue(PROFILING_FAILED));
    //IdeCloudDeviceTransInit
    //inotifyRootFd_ error
    EXPECT_EQ(PROFILING_FAILED, inotify->NotifyEvents(-1,1, 0));
    //beat_error
    EXPECT_EQ(PROFILING_FAILED, inotify->NotifyEvents(1, 1, -1));
    //poll error
    EXPECT_EQ(PROFILING_FAILED, inotify->NotifyEvents(1, 1, 1));
    //poll timeout
    EXPECT_EQ(PROFILING_SUCCESS, inotify->NotifyEvents(1, 1, 1));
    //poll timeout
    EXPECT_EQ(PROFILING_SUCCESS, inotify->NotifyEvents(1, 1, 1));
    //WatchInotifyEvents error
    EXPECT_EQ(PROFILING_FAILED, inotify->NotifyEvents(1, 1, 1));
}

int read_stub(int fd, void *buf, int len) {
    char event_buf[MAX_EVENT_BUFFER_SIZE] = {0};
    struct inotify_event *event = (struct inotify_event*)(event_buf);
    event->wd = 1;
    event->len = strlen("start")+1;
    event->mask = IN_CLOSE_WRITE;
    strcpy(event->name,"start");

    struct inotify_event *event1 = (struct inotify_event*)(event_buf + sizeof(struct inotify_event)+event->len);
    event1->wd = 1;
    event1->len = strlen("stop")+2;
    event1->mask = IN_CREATE;
    strcpy(event1->name,"stop");

    struct inotify_event *event2 = (struct inotify_event*)(event_buf + ((sizeof(struct inotify_event)+event->len) *2));
    event2->wd = 1;
    event2->len = 0;
    event2->mask = IN_DELETE_SELF;
    int bn = 2 * (sizeof(struct inotify_event)+event->len) + sizeof(struct inotify_event);
    // size > len
    struct inotify_event *event3 = (struct inotify_event*)(event_buf + bn);
    event3->wd = -1;
    event3->len = 1000;
    event3->mask = IN_MOVE_SELF;
    bn += sizeof(struct inotify_event);

    memcpy(buf, event_buf, bn);
    return bn;
}

TEST_F(PROF_INOTIFY_UTEST, WatchInotifyEvents)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());

    MOCKER(read)
        .stubs()
        .will(returnValue(-1))
        .then(invoke(read_stub));

    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::InotifyBeatProcess)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS))
        .then(returnValue(PROFILING_FAILED));
    //fd error
    EXPECT_EQ(PROFILING_FAILED, inotify->WatchInotifyEvents(-1));
    //read error
    EXPECT_EQ(PROFILING_FAILED, inotify->WatchInotifyEvents(1));

    EXPECT_EQ(PROFILING_SUCCESS, inotify->WatchInotifyEvents(1));
}

TEST_F(PROF_INOTIFY_UTEST, InotifyDataProcess)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());
    char event_buf[MAX_EVENT_BUFFER_SIZE] = {0};
    struct inotify_event *event = (struct inotify_event*)(event_buf);
    event->wd = 1;
    event->len = strlen("mm.done");
    event->mask = IN_CREATE;
    strcpy(event->name,"mm.done");
    InotifyInitMock(inotify);
    inotify->inotifyPathMap_[event->wd] = "./tmp";
    //IdeCloudDeviceTransInit
    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::MoveFileTask::WriteMoveFileBufData)
        .stubs()
        .will(returnValue(true));
    //inofifyDevIdMap
    EXPECT_EQ(PROFILING_SUCCESS, inotify->InotifyDataProcess(*event));
}

TEST_F(PROF_INOTIFY_UTEST, MoveFileTask_Init)
{
    std::string hostLocalDir = "";
    analysis::dvvp::host::ProfInotify::MoveFileTask moveFileTask;
    int ret = moveFileTask.Init();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(PROF_INOTIFY_UTEST, MoveFileTask_ReadMoveFileBufData)
{
    std::string hostLocalDir = "";
    analysis::dvvp::host::ProfInotify::MoveFileTask moveFileTask;
    int ret = moveFileTask.Init();

    std::shared_ptr<std::string> data(new std::string("test"));
    std::string writeData("test1234");
    ret = moveFileTask.WriteMoveFileBufData(writeData);
    EXPECT_EQ(true, ret);
    
    ret = moveFileTask.ReadMoveFileBufData(data);
    EXPECT_EQ(true, ret);
}

TEST_F(PROF_INOTIFY_UTEST, MoveFileTask_run)
{
    std::string hostLocalDir = "";
    analysis::dvvp::host::ProfInotify::MoveFileTask moveFileTask;
    int ret = moveFileTask.Init();
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    MOCKER_CPP(&analysis::dvvp::common::thread::Thread::IsQuit)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(false))
        .then(returnValue(false))
        .then(returnValue(false))
        .then(returnValue(false))
        .then(returnValue(false))
        .then(returnValue(false))
        .then(returnValue(true));
    
    MOCKER(analysis::dvvp::common::utils::Utils::CreateDir)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    
    MOCKER(rename)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0))
        .then(returnValue(-1));

    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::MoveFileTask::CheckCanMoveData)
        .stubs()
        .will(returnValue(false));
    
    std::string writeData("test1234");
    ret = moveFileTask.WriteMoveFileBufData(writeData);
    writeData = "/home/test/test.host";
    ret = moveFileTask.WriteMoveFileBufData(writeData);
    writeData = "/home/test/test.done";
    ret = moveFileTask.WriteMoveFileBufData(writeData);
    writeData = "/home/test/test.1234.done";
    ret = moveFileTask.WriteMoveFileBufData(writeData);
    writeData = "/home/test/test.1234.slice_cxxx.done";
    ret = moveFileTask.WriteMoveFileBufData(writeData);
    ret = moveFileTask.WriteMoveFileBufData(writeData);
    ret = moveFileTask.WriteMoveFileBufData(writeData);
    ret = moveFileTask.WriteMoveFileBufData(writeData);
    
    moveFileTask.Run();
}

TEST_F(PROF_INOTIFY_UTEST, InotifyBeatTimeout)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());
    EXPECT_NE(nullptr, inotify);

    //InotifyCloudInitMock(inotify);
    MOCKER(analysis::dvvp::common::utils::Utils::GetClockMonotonicRaw)
        .stubs()
        .will(returnValue((unsigned long long)0))
        .then(returnValue((unsigned long long)0xFFFFFFFFFFFFFFFF));

    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::CloudTask::Uinit)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    //inofifyDevIdMap

    analysis::dvvp::host::ProfSampleCfg sma;
    sma.isStart = 1;
    sma.devId = 1;
    sma.jobId = "111";
    sma.defCfg = "{}";
    sma.rawTime = 0;
    sma.wdPath = "";

    inotify->inotifyDevIdMap_["JOBAAAAAAAAAAAAAC"] = sma;
    inotify->InotifyBeatTimeout();
    std::shared_ptr<analysis::dvvp::host::ProfInotify::CloudTask> task
        (new analysis::dvvp::host::ProfInotify::CloudTask());
    inotify->cloudTaskTransMap_["JOBAAAAAAAAAAAAAC"] = task;
    inotify->InotifyBeatTimeout();
    inotify->InotifyBeatTimeout();
}

TEST_F(PROF_INOTIFY_UTEST, InotifyBeatProcess)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());

    char event_buf[MAX_EVENT_BUFFER_SIZE] = {0};
    struct inotify_event *event = (struct inotify_event*)(event_buf);
    event->wd = 1;
    event->len = strlen("JOBAAAAAAAAAAAAAC_done");
    event->mask = IN_CREATE;
    strcpy(event->name,"JOBAAAAAAAAAAAAAC_done");
    InotifyCloudInitMock(inotify);
    MOCKER(analysis::dvvp::common::utils::Utils::GetClockMonotonicRaw)
        .stubs()
        .will(returnValue((unsigned long long)0))
        .then(returnValue((unsigned long long)0xFFFFFFFFFFFFFFFF));
    //inofifyDevIdMap

    analysis::dvvp::host::ProfSampleCfg sma;
    sma.isStart = 1;
    sma.devId = 1;
    sma.jobId = "111";
    sma.defCfg = "{}";
    sma.rawTime = 0;
    sma.wdPath = "";

    std::shared_ptr<analysis::dvvp::host::ProfInotify::CloudTask> task
        (new analysis::dvvp::host::ProfInotify::CloudTask());
    inotify->cloudTaskTransMap_["JOBAAAAAAAAAAAAAC"] = task;
    inotify->inotifyDevIdMap_["JOBAAAAAAAAAAAAAC"] = sma;
    EXPECT_EQ(PROFILING_SUCCESS, inotify->InotifyBeatProcess(*event));
    EXPECT_EQ(PROFILING_SUCCESS, inotify->InotifyBeatProcess(*event));
    inotify->inotifyDevIdMap_["JOBAAAAAAAAAAAAAC"] = sma;
    EXPECT_EQ(PROFILING_SUCCESS, inotify->InotifyBeatProcess(*event));

}

TEST_F(PROF_INOTIFY_UTEST, InotifyBeatProcessNoWd)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());
    char event_buf[MAX_EVENT_BUFFER_SIZE] = {0};
    struct inotify_event *event = (struct inotify_event*)(event_buf);
    event->wd = 1;
    event->len = strlen("mm.done");
    event->mask = IN_CREATE;
    strcpy(event->name,"mm.done");
    //inofifyDevIdMap

    EXPECT_EQ(PROFILING_FAILED, inotify->InotifyBeatProcess(*event));
}

TEST_F(PROF_INOTIFY_UTEST, InotifyCreateEventsProcess)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());
    std::shared_ptr<analysis::dvvp::host::ProfInotify::CloudTask> task
        (new analysis::dvvp::host::ProfInotify::CloudTask());

    inotify->cloudTaskTransMap_["JOBAAAAAAAAAAAAAC"] = task;
    char event_buf[MAX_EVENT_BUFFER_SIZE] = {0};
    struct inotify_event *event = (struct inotify_event*)(event_buf);
    event->wd = 1;
    event->len = INOTIFY_STOP_EVENT.length();
    event->mask = IN_CREATE;
    strcpy(event->name, INOTIFY_STOP_EVENT.c_str());

    //inofifyDevIdMap

    EXPECT_EQ(PROFILING_FAILED, inotify->InotifyCreateEventsProcess(*event));
    std::string eventName = "JOBAAAAAAAAAAAAAC_";
    eventName.append(INOTIFY_STOP_EVENT);
    strcpy(event->name, eventName.c_str());
    EXPECT_EQ(PROFILING_FAILED, inotify->InotifyCreateEventsProcess(*event));
    inotify->cloudTaskTransMap_.erase("JOBAAAAAAAAAAAAAC");
    EXPECT_EQ(PROFILING_FAILED, inotify->InotifyCreateEventsProcess(*event));
    InotifyInitMock(inotify);
    event->wd = 1;
    event->len = INOTIFY_BEAT_EVENT.length();
    event->mask = IN_CREATE;
    strcpy(event->name, INOTIFY_BEAT_EVENT.c_str());
    EXPECT_EQ(PROFILING_FAILED, inotify->InotifyCreateEventsProcess(*event));
    InotifyCloudInitMock(inotify);
    event->wd = 1;
    event->len = INOTIFY_STOP_EVENT.length();
    event->mask = IN_CREATE;
    strcpy(event->name, INOTIFY_STOP_EVENT.c_str());
    EXPECT_EQ(PROFILING_FAILED, inotify->InotifyCreateEventsProcess(*event));
    InotifyCloudInitMock(inotify);
    event->wd = 1;
    event->mask = IN_CREATE;
    eventName = "JOBAAAAAAAAAAAAAC_";
    eventName.append(INOTIFY_BEAT_EVENT);
    event->len = eventName.size();
    strcpy(event->name, eventName.c_str());
    inotify->cloudTaskTransMap_["JOBAAAAAAAAAAAAAC"] = task;
    EXPECT_EQ(PROFILING_SUCCESS, inotify->InotifyCreateEventsProcess(*event));
    inotify->cloudTaskTransMap_.erase("JOBAAAAAAAAAAAAAC");
    EXPECT_EQ(PROFILING_FAILED, inotify->InotifyCreateEventsProcess(*event));
    event->wd = 1;
    event->len = strlen("mm_done");
    event->mask = IN_CREATE;
    strcpy(event->name, "mm_done");

    EXPECT_EQ(PROFILING_SUCCESS, inotify->InotifyCreateEventsProcess(*event));

}

TEST_F(PROF_INOTIFY_UTEST, InotifyCloseEventsProcess)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());

    inotify->inotifyPathMap_[1] = "./tmp";

    char event_buf[MAX_EVENT_BUFFER_SIZE] = {0};
    struct inotify_event *event = (struct inotify_event*)(event_buf);
    std::string eventName = "JOBAAAAAAAAAAAAAC";
    eventName.append("_");
    eventName.append(INOTIFY_START_EVENT);
    event->wd = 1;
    event->len = eventName.size();
    event->mask = IN_CLOSE_WRITE;
    strcpy(event->name, eventName.c_str());
    EXPECT_EQ(PROFILING_FAILED, inotify->InotifyCloseEventsProcess(*event));
    analysis::dvvp::host::ProfSampleCfg tCfg;
    tCfg.wdPath  = "./tmp";
    tCfg.devId   = 0;
    tCfg.rawTime = 0;
    tCfg.isStart = 0;

    inotify->inotifyDevIdMap_["JOBAAAAAAAAAAAAAC"] = tCfg;
    InotifyInitMock(inotify);
    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::ReadFileByPath)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    //ReadFileByPath error
    EXPECT_EQ(PROFILING_FAILED, inotify->InotifyCloseEventsProcess(*event));
    EXPECT_EQ(PROFILING_FAILED, inotify->InotifyCloseEventsProcess(*event));

    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::CloudTask::Init)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    // MOCKER_CPP(&analysis::dvvp::host::ProfInotify::CloudTask::Start)
    //     .stubs()
    //     .will(returnValue(0));
    MOCKER(mmCreateTaskWithThreadAttr)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::GetMsProfCfgJobId)
            .stubs()
            .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::CheckStartFile)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    EXPECT_EQ(PROFILING_FAILED, inotify->InotifyCloseEventsProcess(*event));
    EXPECT_EQ(PROFILING_SUCCESS, inotify->InotifyCloseEventsProcess(*event));
    EXPECT_EQ(PROFILING_SUCCESS, inotify->InotifyCloseEventsProcess(*event));
    event->wd = 1;
    event->len = INOTIFY_UUID_EVENT.length();
    event->mask = IN_CLOSE_WRITE;
    strcpy(event->name, INOTIFY_UUID_EVENT.c_str());
    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::InotifyUuidEventProcess)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, inotify->InotifyCloseEventsProcess(*event));
    EXPECT_EQ(PROFILING_SUCCESS, inotify->InotifyCloseEventsProcess(*event));
}

TEST_F(PROF_INOTIFY_UTEST, InotifySelfEventsProcess)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());
    char event_buf[MAX_EVENT_BUFFER_SIZE] = {0};
    struct inotify_event *event = (struct inotify_event*)(event_buf);
    event->wd = 1;
    event->len = INOTIFY_START_EVENT.length();
    event->mask = IN_CREATE;
    strcpy(event->name, INOTIFY_START_EVENT.c_str());
    //inofifyDevIdMap
    inotify->inotifyPathMap_[1] = "./tmp";
    MOCKER(inotify_rm_watch)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));
    EXPECT_EQ(PROFILING_SUCCESS, inotify->InotifySelfEventsProcess(*event));
}

TEST_F(PROF_INOTIFY_UTEST, InotifyUuidEventProcessFaild)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());

    char event_buf[MAX_EVENT_BUFFER_SIZE] = {0};
    struct inotify_event *event = (struct inotify_event*)(event_buf);
    event->wd = 1;
    event->mask = IN_CLOSE_WRITE;
    strcpy(event->name, INOTIFY_UUID_EVENT.c_str());
    //inofifyDevIdMap

    EXPECT_EQ(PROFILING_FAILED, inotify->InotifyUuidEventProcess(*event));

    InotifyInitMock(inotify);
    std::string uuid = "12345678";
    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::ReadFileByPath)
        .stubs()
        .with(any(), outBound(uuid))
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    inotify->inotifyPathMap_[1] = "./tmp";
    std::string eventName = std::string(INOTIFY_UUID_EVENT);
    event->len = eventName.size();
    strcpy(event->name, eventName.c_str());
    EXPECT_EQ(PROFILING_FAILED, inotify->InotifyUuidEventProcess(*event));
    EXPECT_EQ(PROFILING_FAILED, inotify->InotifyUuidEventProcess(*event));
    eventName.append("_");
    std::string invalidDevId = eventName + "-1";
    event->len = invalidDevId.size();
    strcpy(event->name, invalidDevId.c_str());
    EXPECT_EQ(PROFILING_FAILED, inotify->InotifyUuidEventProcess(*event));
    eventName.append("0");
    event->len = eventName.size();
    strcpy(event->name, eventName.c_str());
    EXPECT_EQ(PROFILING_FAILED, inotify->InotifyUuidEventProcess(*event));
    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::InotifySetProfSampleCfg)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::InotifyStartCfgProcess)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    EXPECT_EQ(PROFILING_FAILED, inotify->InotifyUuidEventProcess(*event));
}

TEST_F(PROF_INOTIFY_UTEST, InotifyUuidEventProcess)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());

    inotify->inotifyPathMap_[1] = "./tmp";

    std::string scfg = "{\"profiling_mode\": \"def_mode\", \
        \"system_wide\": \"off\", \"job_id\": \"a4dcbef99afc\", \"trace_id\":\"b77b2306\", \
        \"ai_core_profiling\": \"off\", \"result_dir\": \"/opt/davinci/log/profiling/\", \
        \"aicore_sampling_interval\": 10, \"ai_core_profiling_mode\": \"task-based\", \
        \"ts_timeline\": \"on\", \
        \"ts_task_track\": \"off\", \"devices\": \"0\", \"ai_core_profiling_events\": \"0x3,0x8,0x9,0xe,0x3a,0x3b,0x4a,0x49\", \
        \"is_cancel\": false, \"ai_core_status\": \"off\", \"hwts_profiling\": \"off\"}";


    char event_buf[MAX_EVENT_BUFFER_SIZE] = {0};
    struct inotify_event *event = (struct inotify_event*)(event_buf);
    event->wd = 1;
    event->len = INOTIFY_UUID_EVENT.length();
    event->mask = IN_CLOSE_WRITE;
    strcpy(event->name, INOTIFY_UUID_EVENT.c_str());
    InotifyInitMock(inotify);
    std::string uuid = "JOBBAAAAAAAAAAACAAAAA";
    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::ReadFileByPath)
        .stubs()
        .with(any(), outBound(uuid))
        .will(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, inotify->InotifyUuidEventProcess(*event));
    std::string eventName = std::string(INOTIFY_UUID_EVENT);
    eventName.append("_");
    eventName.append("0");
    event->len = eventName.size();
    strcpy(event->name, eventName.c_str());

    MOCKER_CPP(&analysis::dvvp::message::ProfileParams::FromString)
        .stubs()
        .will(returnValue(1))
        .then(returnValue(0))
        .then(returnValue(1));
    inotify->defSampleCfg_.defCfg = scfg;
    const std::string path = "/tmp/PROFILER_UT/";
    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetInotifyDir)
        .stubs()
        .will(returnValue(path));
    EXPECT_EQ(PROFILING_SUCCESS, inotify->InotifyUuidEventProcess(*event));
    EXPECT_EQ(PROFILING_FAILED, inotify->InotifyUuidEventProcess(*event));
    analysis::dvvp::host::ProfSampleCfg sma;
    sma.isStart = 1;
    sma.devId = 0;
    sma.jobId = "111";
    sma.defCfg = "{}";
    sma.rawTime = 0;
    sma.wdPath = "";
    inotify->inotifyDevIdMap_["JOBAAAAAAAAAAAAAC"] = sma;
    EXPECT_EQ(PROFILING_FAILED, inotify->InotifyUuidEventProcess(*event));
}

TEST_F(PROF_INOTIFY_UTEST, GetMsProfCfgJobId)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());
    std::string jobId;
    std::string valid = "";
    std::string cfg = "{ \"job_id\": \"RTJXZERIUZKGZMNYLKRAQQYDAXGDTH\", \
     \"ts_fw_training\": \"on\", \"task_base\": \"on\", \
     \"hwts_log\": \"on\", \"ts_timeline\": \"on\" }";

    EXPECT_EQ(PROFILING_FAILED, inotify->GetMsProfCfgJobId(jobId, valid));
    EXPECT_EQ(PROFILING_SUCCESS, inotify->GetMsProfCfgJobId(jobId, cfg));
}

TEST_F(PROF_INOTIFY_UTEST, InotifyRootDirEvents)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());
    EXPECT_EQ(PROFILING_FAILED, inotify->InotifyRootDirEvents(-1));
    // EXPECT_EQ(PROFILING_FAILED, inotify->InotifyRootDirEvents(1));
    MOCKER(read)
        .stubs()
        .will(invoke(read_stub));
    EXPECT_EQ(PROFILING_SUCCESS, inotify->InotifyRootDirEvents(1));

}


TEST_F(PROF_INOTIFY_UTEST, InotifyCreateDir)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());

    char event_buf[MAX_EVENT_BUFFER_SIZE] = {0};
    struct inotify_event *event = nullptr;
    event = (struct inotify_event*)(event_buf);
    event->wd = 1;
    event->len = INOTIFY_UUID_EVENT.length();
    event->mask = IN_CLOSE_WRITE;
    strcpy(event->name, INOTIFY_UUID_EVENT.c_str());
    EXPECT_EQ(PROFILING_FAILED, inotify->InotifyCreateDir(*event));
    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::AddWatchFile)
        .stubs()
        .will(returnValue(1));

    EXPECT_EQ(PROFILING_SUCCESS, inotify->InotifyCreateDir(*event));

    MOCKER(mmChmod)
        .stubs()
        .will(returnValue(-1));
	EXPECT_EQ(PROFILING_FAILED, inotify->InotifyCreateDir(*event));

    MOCKER(mmChmod)
        .stubs()
        .will(returnValue(0))
        .then(returnValue(-1));
	EXPECT_EQ(PROFILING_FAILED, inotify->InotifyCreateDir(*event));

}


TEST_F(PROF_INOTIFY_UTEST, InotifyCreateResponseDir)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());
    const std::string path = "test";
   
    MOCKER(analysis::dvvp::common::utils::Utils::CreateDir)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    
    MOCKER(mmChmod)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));
  
	EXPECT_EQ(PROFILING_FAILED, inotify->InotifyCreateResponseDir(path));
    EXPECT_EQ(PROFILING_FAILED, inotify->InotifyCreateResponseDir(path));
    EXPECT_EQ(PROFILING_SUCCESS, inotify->InotifyCreateResponseDir(path));

}

TEST_F(PROF_INOTIFY_UTEST, GetPodNameDir)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());
    inotify->devIdPathMap_[1] ="./tmp";

    EXPECT_EQ(".", inotify->GetPodNameDir(1));
}

TEST_F(PROF_INOTIFY_UTEST, CheckPodName)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());
    inotify->devIdPathMap_[1] ="./tmp";
    std::string dockerId = "asdasdad";
    std::string podName = "0";
    std::string devId = "0";

    EXPECT_EQ(PROFILING_FAILED, inotify->CheckPodName("", podName, devId));

    std::string rankFile = "./jobstarthccl.json";
    std::string json = "{\"status\": \"completed\",\"group_count\": \"2\",\"group_list\": [{\"group_name\": \"training\", \
    \"instance_count\": \"1024\",\"instance_list\": [{\"pod_name\": \"tf-bae43\",\"server_id\": \"10.0.0.10\",\"devices\": \
     [{\"device_id\": \"0\",\"device_ip\": \"192.168.0.11\"}]}]}]}";


    // test exception 
    EXPECT_EQ(PROFILING_FAILED, inotify->CheckPodName(rankFile, podName, devId));
    std::ofstream ofs(rankFile);
    ofs << json << std::endl;
    ofs.close();
    
    EXPECT_EQ(PROFILING_FAILED, inotify->CheckPodName(rankFile, podName, devId));
    json = "{\"status\": \"completed\",\"group_count\": \"2\",\"group_list\": [{\"group_name\": \"training\", \
    \"instance_count\": \"1024\",\"instance_list\": [{\"pod_name\": \"0\",\"server_id\": \"10.0.0.10\",\"devices\": \
     [{\"device_id\": \"0\",\"device_ip\": \"192.168.0.11\"}]}]}]}";
    std::ofstream ofs1(rankFile);
    ofs1 << json << std::endl;
    ofs1.close();
    EXPECT_EQ(PROFILING_SUCCESS, inotify->CheckPodName(rankFile, podName, devId));
    json = "{\"status\": \"completed\",\"group_count\": \"2\",\"group_list\": [{\"group_name\": \"training\", \
    \"instance_count\": \"1024\",\"instance_list1\": [{\"pod_name\": \"0\",\"server_id\": \"10.0.0.10\",\"devices\": \
     [{\"device_id\": \"0\",\"device_ip\": \"192.168.0.11\"}]}]}]}";
    std::ofstream ofs2(rankFile);
    ofs2 << json << std::endl;
    ofs2.close();
    EXPECT_EQ(PROFILING_FAILED, inotify->CheckPodName(rankFile, podName, devId));
    json = "{\"status\": \"completed\",\"group_count\": \"2\",\"group_list\": [{\"group_name\": \"training\", \
    \"instance_count\": \"1024\",\"instance_list\": [{\"pod_name\": \"0\",\"server_id\": \"10.0.0.10\",\"devices\": \
     [{\"device_id\": \"3\",\"device_ip\": \"192.168.0.11\"}]}]}]}";
    std::ofstream ofs3(rankFile);
    ofs3 << json << std::endl;
    ofs3.close();
    EXPECT_EQ(PROFILING_FAILED, inotify->CheckPodName(rankFile, podName, devId));
    ::remove(rankFile.c_str());
}

TEST_F(PROF_INOTIFY_UTEST, GetMsProfCfgDockerId)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());
    std::string jobId;
    std::string valid = "";
    std::string cfg = "{ \"job_id\": \"RTJXZERIUZKGZMNYLKRAQQYDAXGDTH\", \
     \"ts_fw_training\": \"on\", \"task_base\": \"on\", \
     \"hwts_log\": \"on\", \"ts_timeline\": \"on\" ,\"docker_id\": \"asdasdasdadad\"}";

    EXPECT_EQ(PROFILING_FAILED, inotify->GetMsProfCfgDockerId(jobId, valid));
    EXPECT_EQ(PROFILING_SUCCESS, inotify->GetMsProfCfgDockerId(jobId, cfg));
}

TEST_F(PROF_INOTIFY_UTEST, CheckStartFile)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());
    std::string jobId;
    std::string valid = "";

    std::string cfg = "{ \"job_id\": \"RTJXZERIUZKGZMNYLKRAQQYDAXGDTH\", \
     \"ts_fw_training\": \"on\", \"task_base\": \"on\", \
     \"hwts_log\": \"on\", \"ts_timeline\": \"on\" ,\"docker_id\": \"\"}";

    EXPECT_EQ(PROFILING_SUCCESS, inotify->CheckStartFile(cfg, 0));
    cfg = "{ \"job_id\": \"RTJXZERIUZKGZMNYLKRAQQYDAXGDTH\", \
     \"ts_fw_training\": \"on\", \"task_base\": \"on\", \
     \"hwts_log\": \"on\", \"ts_timeline\": \"on\" ,\"docker_id\": \"asdasdasdadad\"}";

    EXPECT_EQ(PROFILING_FAILED, inotify->CheckStartFile(cfg, 0));
    std::string rankFile = "./jobstarthccl.json";
    std::vector<std::string> files;
    files.push_back(rankFile);
    MOCKER(analysis::dvvp::common::utils::Utils::GetFiles)
        .stubs()
        .with(any(), any(), outBound(files));

    const std::string path = "/tmp/asdasdasd";

    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::GetPodNameDir)
        .stubs()
        .will(returnValue(path));

    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::GetMsProfCfgJobId)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::CheckPodName)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, inotify->CheckStartFile(cfg, 0));
    EXPECT_EQ(PROFILING_SUCCESS, inotify->CheckStartFile(cfg, 0));
}

TEST_F(PROF_INOTIFY_UTEST, InotifySetProfSampleCfg)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
        (new analysis::dvvp::host::ProfInotify());
    char event_buf[MAX_EVENT_BUFFER_SIZE] = {0};
    struct inotify_event *event = (struct inotify_event*)(event_buf);
    event->wd = 1;
    std::string eventName = "JOBAAAAAAAAAAAAAC" + std::string(INOTIFY_START_EVENT);
    event->len = eventName.size();
    event->mask = IN_CLOSE_WRITE;
        strcpy(event->name, eventName.c_str());

    MOCKER_CPP(&analysis::dvvp::host::ProfInotify::ReadFileByPath)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    analysis::dvvp::host::ProfSampleCfgT cfg;
    std::string uuidPath = "b";
    // test readfile failed
    EXPECT_EQ(PROFILING_FAILED, inotify->InotifySetProfSampleCfg(*event,
                                                                 cfg,
                                                                 1));
    inotify->inotifyPathMap_[1] = "a";
    EXPECT_EQ(PROFILING_FAILED, inotify->InotifySetProfSampleCfg(*event,
                                                                 cfg,
                                                                 1));
    EXPECT_EQ(PROFILING_FAILED, inotify->InotifySetProfSampleCfg(*event,
                                                                 cfg,
                                                                 1));
    cfg.jobId= "JOBAAAAAAAAAAAAAAAAA";
    EXPECT_EQ(PROFILING_SUCCESS, inotify->InotifySetProfSampleCfg(*event,
                                                                 cfg,
                                                                 1));
}


TEST_F(PROF_INOTIFY_UTEST, WriteJobInfo)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
    (new analysis::dvvp::host::ProfInotify());

    inotify->WriteJobInfo("", "");

    const std::string defcfg = "{ \"job_id\": \"RTJXZERIUZKGZMNYLKRAQQYDAXGDTH\", \
     \"ts_fw_training\": \"on\", \"task_base\": \"on\", \
     \"hwts_log\": \"on\", \"ts_timeline\": \"on\", \"job_info\":\"123123123\",\"feature_name\":\"op_trace\"\
      ,\"ai_core_events\":\"asdasd\",\"l2_cache_events\":\"asda\"}";

    const std::string uuid = "JOBXAAA";
    inotify->WriteJobInfo(defcfg, uuid);

}

TEST_F(PROF_INOTIFY_UTEST, InotifyCreateDefaultPodDir)
{
    std::shared_ptr<analysis::dvvp::host::ProfInotify> inotify
    (new analysis::dvvp::host::ProfInotify());

    MOCKER(analysis::dvvp::common::utils::Utils::IsDir)
        .stubs()
        .will(returnValue(1));


    MOCKER(analysis::dvvp::common::utils::Utils::CreateDir)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

     MOCKER(mmChmod)
        .stubs()
        .will(returnValue(-1));
	
    
    inotify->InotifyCreateDefaultPodDir();
}


