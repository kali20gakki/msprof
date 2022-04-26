#include <iostream>
#include <stdexcept>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "message/prof_params.h"

using namespace analysis::dvvp::message;

///////////////////////////////////////////////////////////////////
class MESSAGE_MESSAGE_STATUSINFO_TEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(MESSAGE_MESSAGE_STATUSINFO_TEST, ToObject) {
    GlobalMockObject::verify();

    std::string info = "this is test";
    StatusInfo status("1", ERR, info);

    nlohmann::json object;
    status.ToObject(object);
    EXPECT_STREQ("{\"dev_id\":\"1\",\"info\":\"this is test\",\"status\":1}", object.dump().c_str());
}

TEST_F(MESSAGE_MESSAGE_STATUSINFO_TEST, FromObject) {
    GlobalMockObject::verify();

    std::string info = "this is test";
    StatusInfo status("1", ERR, info);

    nlohmann::json object;
    status.ToObject(object);

    StatusInfo status1;
    status1.FromObject(object);

    EXPECT_STREQ(status.info.c_str(), status1.info.c_str());
    EXPECT_EQ(status.dev_id, status1.dev_id);
    EXPECT_EQ(status.status, status1.status);
}

TEST_F(MESSAGE_MESSAGE_STATUSINFO_TEST, ToString) {
    GlobalMockObject::verify();

    std::string info = "this is test";
    StatusInfo status("1", ERR, info);

    EXPECT_STREQ("{\"dev_id\":\"1\",\"info\":\"this is test\",\"status\":1}", status.ToString().c_str());
}

TEST_F(MESSAGE_MESSAGE_STATUSINFO_TEST, FromString) {
    GlobalMockObject::verify();

    std::string info = "this is test";
    StatusInfo status("1", ERR, info);

    StatusInfo status1;
    EXPECT_FALSE(status1.FromString("{"));
    EXPECT_FALSE(status1.FromString(""));
    EXPECT_TRUE(status1.FromString(status.ToString()));
}

///////////////////////////////////////////////////////////////////
class MESSAGE_MESSAGE_STATUS_TEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(MESSAGE_MESSAGE_STATUS_TEST, ToObject) {
    GlobalMockObject::verify();

    std::string info = "this is test";
    StatusInfo status_info("1", ERR, info);

    Status status;
    status.AddStatusInfo(status_info);

    nlohmann::json object;
    status.ToObject(object);
    std::string str = "{\"info\":[{\"dev_id\":\"1\",\"info\":\"this is test\",\"status\":1}],\"status\":1}";
    EXPECT_STREQ(str.c_str(), object.dump().c_str());
}

TEST_F(MESSAGE_MESSAGE_STATUS_TEST, FromObject) {
    GlobalMockObject::verify();

    std::string info = "this is test";
    StatusInfo status_info("1", ERR, info);

    Status status;
    status.AddStatusInfo(status_info);

    nlohmann::json object;
    status.ToObject(object);

    Status status1;
    status1.FromObject(object);

    EXPECT_EQ(status.status, status1.status);
}

TEST_F(MESSAGE_MESSAGE_STATUS_TEST, ToString) {
    GlobalMockObject::verify();

    std::string info = "this is test";
    StatusInfo status_info("1", ERR, info);

    Status status;
    status.AddStatusInfo(status_info);
    std::string str = "{\"info\":[{\"dev_id\":\"1\",\"info\":\"this is test\",\"status\":1}],\"status\":1}";
    EXPECT_STREQ(str.c_str(), status.ToString().c_str());
}

TEST_F(MESSAGE_MESSAGE_STATUS_TEST, FromString) {
    GlobalMockObject::verify();

    std::string info = "this is test";
    StatusInfo status_info("1", ERR, info);

    Status status;
    status.AddStatusInfo(status_info);

    Status status1;
    EXPECT_FALSE(status1.FromString("{"));
    EXPECT_FALSE(status1.FromString(""));
    EXPECT_TRUE(status1.FromString(status.ToString()));
}

///////////////////////////////////////////////////////////////////
class MESSAGE_MESSAGE_PROFILEPARAMS_TEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(MESSAGE_MESSAGE_PROFILEPARAMS_TEST, ToObject) {
    GlobalMockObject::verify();

    ProfileParams params;

    nlohmann::json object;
    params.ToObject(object);
    std::string str =  "{\"acc_pmu_mode\":\"\",\"acl\":\"\",\"aiCtrlCpuProfiling\":\"off\",\"ai_core_metrics\":\"\",\"ai_core_profiling\":\"\",\"ai_core_profiling_events\":\"\",\"ai_core_profiling_metrics\":\"\",\"ai_core_profiling_mode\":\"\",\"ai_core_status\":\"\",\"ai_ctrl_cpu_profiling_events\":\"\",\"ai_vector_status\":\"\",\"aicore_sampling_interval\":10,\"aicpuTrace\":\"\",\"aiv_metrics\":\"\",\"aiv_profiling\":\"\",\"aiv_profiling_events\":\"\",\"aiv_profiling_metrics\":\"\",\"aiv_profiling_mode\":\"\",\"aiv_sampling_interval\":10,\"app\":\"\",\"app_dir\":\"\",\"app_env\":\"\",\"app_location\":\"\",\"app_parameters\":\"\",\"biu\":\"\",\"biu_freq\":1000,\"cpu_profiling\":\"off\",\"cpu_sampling_interval\":20,\"ddr_interval\":20,\"ddr_master_id\":0,\"ddr_profiling\":\"\",\"ddr_profiling_events\":\"\",\"devices\":\"\",\"dvpp_profiling\":\"off\",\"dvpp_sampling_interval\":20,\"ffts_block\":\"\",\"ffts_thread_task\":\"\",\"hardware_mem\":\"off\",\"hardware_mem_sampling_interval\":20,\"hbmInterval\":20,\"hbmProfiling\":\"\",\"hbm_profiling_events\":\"\",\"hcclTrace\":\"\",\"hccsInterval\":20,\"hccsProfiling\":\"off\",\"host_cpu_profiling\":\"off\",\"host_disk_freq\":50,\"host_disk_profiling\":\"off\",\"host_mem_profiling\":\"off\",\"host_network_profiling\":\"off\",\"host_osrt_profiling\":\"off\",\"host_profiling\":0,\"host_sys\":\"\",\"host_sys_pid\":-1,\"hwts_log\":\"\",\"hwts_log1\":\"\",\"interconnection_profiling\":\"off\",\"interconnection_sampling_interval\":20,\"io_profiling\":\"off\",\"io_sampling_interval\":10,\"is_cancel\":0,\"jobInfo\":\"\",\"job_id\":\"\",\"l2CacheTaskProfiling\":\"\",\"l2CacheTaskProfilingEvents\":\"\",\"llc_interval\":20,\"llc_profiling\":\"\",\"llc_profiling_events\":\"\",\"low_power\":\"\",\"modelExecution\":\"off\",\"modelLoad\":\"\",\"msprof\":\"off\",\"msprofBinPid\":-1,\"msprof_llc_profiling\":\"\",\"msproftx\":\"off\",\"nicInterval\":10,\"nicProfiling\":\"off\",\"pcieInterval\":20,\"pcieProfiling\":\"\",\"pid_profiling\":\"off\",\"pid_sampling_interval\":100,\"profiling_mode\":\"\",\"profiling_options\":\"\",\"profiling_period\":-1,\"result_dir\":\"\",\"roceInterval\":10,\"roceProfiling\":\"off\",\"runtimeApi\":\"off\",\"runtimeTrace\":\"\",\"stars_acsq_task\":\"\",\"stars_sub_task\":\"\",\"storageLimit\":\"\",\"stream_enabled\":\"on\",\"sys_profiling\":\"off\",\"sys_sampling_interval\":100,\"tsCpuProfiling\":\"off\",\"ts_cpu_hot_function\":\"\",\"ts_cpu_profiling_events\":\"\",\"ts_cpu_usage\":\"\",\"ts_fw_training\":\"\",\"ts_keypoint\":\"\",\"ts_memcpy\":\"\",\"ts_task_track\":\"\",\"ts_timeline\":\"\"}";
    EXPECT_STREQ(str.c_str(), object.dump().c_str());
}

TEST_F(MESSAGE_MESSAGE_PROFILEPARAMS_TEST, FromObject) {
    GlobalMockObject::verify();

    ProfileParams params;

    nlohmann::json object;
    params.ToObject(object);

    ProfileParams params1;
    params1.FromObject(object);

    EXPECT_EQ(params1.aicore_sampling_interval, params.aicore_sampling_interval);
    EXPECT_EQ(params1.cpu_sampling_interval, params.cpu_sampling_interval);
    EXPECT_EQ(params1.sys_sampling_interval, params.sys_sampling_interval);
    EXPECT_EQ(params1.pid_sampling_interval, params.pid_sampling_interval);
    EXPECT_EQ(params1.hardware_mem_sampling_interval, params.hardware_mem_sampling_interval);
    EXPECT_EQ(params1.llc_interval, params.llc_interval);
    EXPECT_EQ(params1.ddr_interval, params.ddr_interval);
    EXPECT_EQ(params1.hbmInterval, params.hbmInterval);
    EXPECT_EQ(params1.io_sampling_interval, params.io_sampling_interval);
    EXPECT_EQ(params1.nicInterval, params.nicInterval);
    EXPECT_EQ(params1.roceInterval, params.roceInterval);
    EXPECT_EQ(params1.interconnection_sampling_interval, params.interconnection_sampling_interval);
    EXPECT_EQ(params1.hccsInterval, params.hccsInterval);
    EXPECT_EQ(params1.pcieInterval, params.pcieInterval);
    EXPECT_EQ(params1.dvpp_sampling_interval, params.dvpp_sampling_interval);
}

TEST_F(MESSAGE_MESSAGE_PROFILEPARAMS_TEST, ToString) {
    GlobalMockObject::verify();

    ProfileParams params;
    std::string str =
          "{\"acc_pmu_mode\":\"\",\"acl\":\"\",\"aiCtrlCpuProfiling\":\"off\",\"ai_core_metrics\":\"\",\"ai_core_profiling\":\"\",\"ai_core_profiling_events\":\"\",\"ai_core_profiling_metrics\":\"\",\"ai_core_profiling_mode\":\"\",\"ai_core_status\":\"\",\"ai_ctrl_cpu_profiling_events\":\"\",\"ai_vector_status\":\"\",\"aicore_sampling_interval\":10,\"aicpuTrace\":\"\",\"aiv_metrics\":\"\",\"aiv_profiling\":\"\",\"aiv_profiling_events\":\"\",\"aiv_profiling_metrics\":\"\",\"aiv_profiling_mode\":\"\",\"aiv_sampling_interval\":10,\"app\":\"\",\"app_dir\":\"\",\"app_env\":\"\",\"app_location\":\"\",\"app_parameters\":\"\",\"biu\":\"\",\"biu_freq\":1000,\"cpu_profiling\":\"off\",\"cpu_sampling_interval\":20,\"ddr_interval\":20,\"ddr_master_id\":0,\"ddr_profiling\":\"\",\"ddr_profiling_events\":\"\",\"devices\":\"\",\"dvpp_profiling\":\"off\",\"dvpp_sampling_interval\":20,\"ffts_block\":\"\",\"ffts_thread_task\":\"\",\"hardware_mem\":\"off\",\"hardware_mem_sampling_interval\":20,\"hbmInterval\":20,\"hbmProfiling\":\"\",\"hbm_profiling_events\":\"\",\"hcclTrace\":\"\",\"hccsInterval\":20,\"hccsProfiling\":\"off\",\"host_cpu_profiling\":\"off\",\"host_disk_freq\":50,\"host_disk_profiling\":\"off\",\"host_mem_profiling\":\"off\",\"host_network_profiling\":\"off\",\"host_osrt_profiling\":\"off\",\"host_profiling\":0,\"host_sys\":\"\",\"host_sys_pid\":-1,\"hwts_log\":\"\",\"hwts_log1\":\"\",\"interconnection_profiling\":\"off\",\"interconnection_sampling_interval\":20,\"io_profiling\":\"off\",\"io_sampling_interval\":10,\"is_cancel\":0,\"jobInfo\":\"\",\"job_id\":\"\",\"l2CacheTaskProfiling\":\"\",\"l2CacheTaskProfilingEvents\":\"\",\"llc_interval\":20,\"llc_profiling\":\"\",\"llc_profiling_events\":\"\",\"low_power\":\"\",\"modelExecution\":\"off\",\"modelLoad\":\"\",\"msprof\":\"off\",\"msprofBinPid\":-1,\"msprof_llc_profiling\":\"\",\"msproftx\":\"off\",\"nicInterval\":10,\"nicProfiling\":\"off\",\"pcieInterval\":20,\"pcieProfiling\":\"\",\"pid_profiling\":\"off\",\"pid_sampling_interval\":100,\"profiling_mode\":\"\",\"profiling_options\":\"\",\"profiling_period\":-1,\"result_dir\":\"\",\"roceInterval\":10,\"roceProfiling\":\"off\",\"runtimeApi\":\"off\",\"runtimeTrace\":\"\",\"stars_acsq_task\":\"\",\"stars_sub_task\":\"\",\"storageLimit\":\"\",\"stream_enabled\":\"on\",\"sys_profiling\":\"off\",\"sys_sampling_interval\":100,\"tsCpuProfiling\":\"off\",\"ts_cpu_hot_function\":\"\",\"ts_cpu_profiling_events\":\"\",\"ts_cpu_usage\":\"\",\"ts_fw_training\":\"\",\"ts_keypoint\":\"\",\"ts_memcpy\":\"\",\"ts_task_track\":\"\",\"ts_timeline\":\"\"}";
    EXPECT_STREQ(str.c_str(), params.ToString().c_str());
}

TEST_F(MESSAGE_MESSAGE_PROFILEPARAMS_TEST, FromString) {
    GlobalMockObject::verify();

    ProfileParams params;

    ProfileParams params1;
    EXPECT_FALSE(params1.FromString("{"));
    EXPECT_FALSE(params1.FromString(""));
    EXPECT_TRUE(params1.FromString(params.ToString()));
}

///////////////////////////////////////////////////////////////////
class MESSAGE_MESSAGE_JOBCONTEXT_TEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(MESSAGE_MESSAGE_JOBCONTEXT_TEST, ToObject) {
    GlobalMockObject::verify();

    JobContext ctx;

    nlohmann::json object;
    ctx.ToObject(object);
    std::string str =
        "{\"chunkEndTime\":0,\"chunkStartTime\":0,\"dataModule\":0,\"dev_id\":\"\",\"job_id\":\"\",\"module\":\"\","
        "\"result_dir\":\"\",\"stream_enabled\":\"\",\"tag\":\"\"}";
    EXPECT_STREQ(str.c_str(), object.dump().c_str());
}

TEST_F(MESSAGE_MESSAGE_JOBCONTEXT_TEST, ToString) {
    GlobalMockObject::verify();

    JobContext ctx;
    std::string str =
        "{\"chunkEndTime\":0,\"chunkStartTime\":0,\"dataModule\":0,\"dev_id\":\"\",\"job_id\":\"\",\"module\":\"\","
        "\"result_dir\":\"\",\"stream_enabled\":\"\",\"tag\":\"\"}";
    EXPECT_STREQ(str.c_str(), ctx.ToString().c_str());
}

TEST_F(MESSAGE_MESSAGE_JOBCONTEXT_TEST, FromString) {
    GlobalMockObject::verify();

    JobContext ctx;

    JobContext ctx1;
    EXPECT_FALSE(ctx1.FromString("{"));
    EXPECT_FALSE(ctx1.FromString(""));
    EXPECT_TRUE(ctx1.FromString(ctx.ToString()));
}
