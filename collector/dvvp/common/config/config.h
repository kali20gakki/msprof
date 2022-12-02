/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 * Description: handle profiling request
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2018-06-13
 */

#ifndef ANALYSIS_DVVP_COMMON_CONFIG_CONFIG_H
#define ANALYSIS_DVVP_COMMON_CONFIG_CONFIG_H

#include <cstdint>
#include <string>
#include <map>

namespace analysis {
namespace dvvp {
namespace common {
namespace config {
// /////////////////////common//////////////////////////////////
const std::string DEVICE_APP_DIR = "/usr/local/profiler/";
const std::string PROF_SCRIPT_FILE_PATH = "/usr/bin/msprof_data_collection.sh";
const std::string PROF_MSPROF_PY_PATH = "profiler/profiler_tool/analysis/msprof/msprof.py";
const std::string PROF_MSPROF_PY_NAME = "msprof.py";
const std::string PROF_MSPROF_SO_NAME = "libmsprofiler.so";
const std::string PROF_MSPROF_BIN_NAME = "msprof";

const char * const HOST_TAG_KEY = "Host";
const char * const DEVICE_TAG_KEY = "Device";
const char * const CLOCK_REALTIME_KEY = "clock_realtime";
const char * const CLOCK_MONOTONIC_RAW_KEY = "clock_monotonic_raw";
const char * const CLOCK_CNTVCT_KEY = "cntvct";

const std::string HOST_APP_DIR = "~/profiler-app";

const int MAX_PATH_LENGTH = 1024;

const int PROFILING_PACKET_MAX_LEN = (3 * 1024 * 1024);  // 3 * 1024 *1024 means 3mb

const int MSVP_BATCH_MAX_LEN = 2621440;  // 2621440 : 2.5 * 1024 *1024 means 2.5MB

const int RECEIVE_CHUNK_SIZE = 320; // chunk size:320

const int HASH_DATA_MAX_LEN = 1024; // hash data max len:1024

const int MSVP_COLLECT_CLIENT_INPUT_MAX_LEN = 128;  // 128 is max string size

const int MSVP_PROFILER_THREADNAME_MAXNUM = 16;

const int MSVP_DECODE_MESSAGE_MAX_LEN = (128 * 1024 * 1024);  // 128 * 1024 *1024 means 128mb

const int MSVP_MESSAGE_TYPE_NAME_MAX_LEN = 1024;  // 1024 means 1KB

const int MSVP_CFG_MAX_SIZE = 64 * 1024 * 1024;  // 64 * 1024 *1024 means 64mb

const long long MSVP_LARGE_FILE_MAX_LEN = 512 * 1024 * 1024; // 512 * 1024 * 1024 means 512mb
const long long MSVP_SMALL_FILE_MAX_LEN = 2 * 1024 * 1024; // 2 * 1024 * 1024 means 2mb

const int MSVP_CLN_SENDER_QUEUE_CAPCITY = 1024;
const int MSVP_CLN_SENDER_POOL_THREAD_NUM = 2;

const int STORAGE_LIMIT_DOWN_THD = 200; // 200MB

const int MAX_ASCEND_INSTALL_INFO_FILE_SIZE = 1024; // 1024 Byte

const int THOUSAND = 1000; // 1000 : 1k

const int MS_TO_NS = 1000000; // 1ms = 1000000ns
#if (defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER))
const int MSVP_MMPROCESS = NULL;
const char * const MSVP_ENV_DELIMITER = ":";
const std::string LIBRARY_PATH = "Path";
#else
const char * const MSVP_ENV_DELIMITER = ";";
const int MSVP_MMPROCESS = -1;
const std::string LIBRARY_PATH = "LD_LIBRARY_PATH";
#endif

const char * const MSVP_UNDERLINE = "_";

const int HOST_PID_DEFAULT = -1;

const std::string SYSTEM_ROOT = "systemroot";

const char * const MSVP_CHANNEL_POOL_NAME_PREFIX = "MSVP_ChanPool_";
const char * const MSVP_SENDER_POOL_NAME_PREFIX = "MSVP_SndPool_";

const char * const msvpChannelThreadName = "MSVP_ChanPoll";
const char * const MSVP_CTRL_RECEIVER_THREAD_NAME = "MSVP_CtrlRecv";
const char * const MSVP_UPLOADER_THREAD_NAME = "MSVP_Upld";
const char * const MSVP_DEVICE_TRANSPORT_THREAD_NAME = "MSVP_DevTrans";
const char * const MSVP_PROF_TASK_THREAD_NAME = "MSVP_ProfTask";
const char * const MSVP_DEVICE_THREAD_NAME_PREFIX = "MSVP_Dev_";
const char * const MSVP_COLLECT_SERVER_THREAD_NAME = "MSVP_CollectSrv";
const char * const MSVP_COLLECT_PERF_SCRIPT_THREAD_NAME = "MSVP_PerfScript";
const char * const MSVP_COLLECT_PROF_TIMER_THREAD_NAME = "MSVP_ProfTimer";
const char * const MSVP_CLOUD_TASK_THREAD_NAME = "MSVP_Task";
const char * const MSVP_HEART_BEAT_THREAD_NAME = "MSVP_HeartBeat";
const char * const MSVP_UPLOADER_DUMPER_THREAD_NAME = "MSVP_UploaderDumper";
const char * const MSVP_HDC_DUMPER_THREAD_NAME = "MSVP_HdcDumper";
const char * const MSVP_RPC_DUMPER_THREAD_NAME = "MSVP_RpcDumper";

// cloud prof config
const std::string SAMPLE_KEY = "sample";
const std::string FILE_UNDERLINE = "_";
const int MAX_BEAT_TIME = 30;
const int MAX_DEVICE_NUMS = 8;

const size_t THREAD_QUEUE_SIZE_DEFAULT = 64;

// prof peripheral job config
const uint32_t DEFAULT_INTERVAL            = 100;
const uint32_t DEAFULT_MASTER_ID           = 0xFFFFFFFF;
const uint32_t PERIPHERAL_EVENT_READ       = 0;
const uint32_t PERIPHERAL_EVENT_WRITE      = 1;
const int32_t PERIPHERAL_INTERVAL_MS_SMIN = 10;
const int32_t PERIPHERAL_INTERVAL_MS_MIN  = 20;
const int32_t PERIPHERAL_INTERVAL_MS_MAX  = 1000;

// prof job config
const char * const PROF_DEVICE_SYS_CPU_USAGE_FILE = "SystemCpuUsage.data";
const char * const PROF_DEVICE_SYS_MEM_USAGE_FILE = "Memory.data";
const char * const PROF_HOST_PID_CPU_USAGE_FILE = "host_cpu.data";
const char * const PROF_HOST_PID_MEM_USAGE_FILE = "host_mem.data";
const char * const PROF_HOST_SYS_CPU_USAGE_FILE = "host_sys_cpu.data";
const char * const PROF_HOST_SYS_MEM_USAGE_FILE = "host_sys_mem.data";
const char * const PROF_HOST_SYS_NETWORK_USAGE_FILE = "host_network.data";
const char * const MSVP_PROF_DATA_DIR = "/data";
const char * const MSVP_PROF_PERF_DATA_FILE = "ai_ctrl_cpu.data.";
const char * const MSVP_PROF_PERF_RET_FILE_SUFFIX = ".txt";
const std::string PROF_AICORE_SAMPLE = "aicore sample based";
const std::string PROF_AIV_SAMPLE = "ai vector core sample based";
const std::string PROF_AICORE_TASK = "aicore task based";
const std::string PROF_AIV_TASK = "ai vector core task based";

// prof task config
const unsigned long PROCESS_WAIT_TIME = 500000;  // should not modify
const std::string PROF_TASK_STREAMTASK_QUEUE_NAME = "ProfTaskStreamBuffer";

// uploader config
const std::string UPLOADER_QUEUE_NAME = "UploaderQueue";
const size_t UPLOADER_QUEUE_CAPACITY = 512;

// prof channel config
const std::string SPEED_PERFCOUNT_MODULE_NAME = std::string("ChannelReaderSpeed");
const std::string SPEEDALL_PERFCOUNT_MODULE_NAME = std::string("ChannelReaderSpeedAll");
const size_t CHANNELPOLL_THREAD_QUEUE_SIZE = 8192;

// prof mgr config
const std::string PROF_FEATURE_TASK      = "task_trace";
const int PROF_MGR_TRACE_ID_DEFAULT_LEN  = 27;

// receive data config
const size_t MAX_LOOP_TIMES = 1400; // the max send package nums of once Dump()
const int SLEEP_INTEVAL_US = 1000; // the interval of Run()
const size_t RING_BUFF_CAPACITY = 16384; // 16384:16K. Note:capacity value must be 2^n
const size_t GE_RING_BUFF_CAPACITY = 262144; // 262144:256K. Note:capacity value must be 2^n
const size_t MSPROF_RING_BUFF_CAPACITY = 262144; // 262144:256K. Note:capacity value must be 2^n

// sender config
const int SEND_BUFFER_LEN = 64 * 1024; // 64 * 1024 menas 64k
const size_t SENDERPOOL_THREAD_QUEUE_SIZE = 8192;

// transport config config
const std::string IDE_PERFCOUNT_MODULE_NAME = std::string("IdeTransport");
const std::string HDC_PERFCOUNT_MODULE_NAME = std::string("HdcTransport");
const std::string FILE_PERFCOUNT_MODULE_NAME = std::string("FileTransport");
const uint64_t TRANSPORT_PRI_FREQ = 128;

const char * const MSVP_PROF_ON = "on";
const std::string MSPROF_SWITCH_ON = "on";

// ai core metrics type
const std::string ARITHMETIC_UTILIZATION = "ArithmeticUtilization";
const std::string PIPE_UTILIZATION = "PipeUtilization";
const std::string MEMORY_BANDWIDTH = "Memory";
const std::string L0B_AND_WIDTH = "MemoryL0";
const std::string RESOURCE_CONFLICT_RATIO = "ResourceConflictRatio";
const std::string PROFILER_SAMPLE_CONFIG_ENV = "PROFILER_SAMPLECONFIG";
const std::string MEMORY_UB = "MemoryUB";
const std::string PROFILING_RESULT_PATH_ENV = "PROFILING_RESULT_PATH";
const std::string PROFILING_AICPU_MODE_ENV = "AICPU_PROFILING_MODE";
const std::string RANK_TABLE_FILE_ENV = "RANK_TABLE_FILE";
const std::string RANK_ID_ENV = "RANK_ID";

// llc  profiling events type
const std::string LLC_PROFILING_CAPACITY = "capacity";
const std::string LLC_PROFILING_BANDWIDTH = "bandwidth";
const std::string LLC_PROFILING_READ = "read";
const std::string LLC_PROFILING_WRITE = "write";

// host sys events type
const std::string HOST_SYS_CPU = "cpu";
const std::string HOST_SYS_MEM = "mem";
const std::string HOST_SYS_DISK = "disk";
const std::string HOST_SYS_NETWORK = "network";
const std::string HOST_SYS_OSRT = "osrt"; // os_runtime: system call && pthread

const std::string OUTPUT_RECORD = "profiling_output_record";

// keypoint op's name/type
const char * const KEYPOINT_OP_NAME = "keypoint_op";
const char * const KEYPOINT_OP_TYPE = "na";

// hash data file tag
const char * const HASH_TAG = "hash_dic";
const char * const HASH_DIC_DELIMITER = ":";

// need paired ageing file
const char * const HWTS_DATA = "hwts.data";
const char * const AICORE_DATA = "aicore.data";
const char * const STORAGE_LIMIT_UNIT = "MB";

// analysis script param
const char * const DEFAULT_INTERATION_ID    = "1";
const char * const DEFAULT_MODEL_ID         = "-1";
const char * const PROFILING_SUMMARY_FORMAT = "csv";

// used for init param
constexpr int DEFAULT_PROFILING_INTERVAL_5MS   = 5;
constexpr int DEFAULT_PROFILING_INTERVAL_10MS  = 10;
constexpr int DEFAULT_PROFILING_INTERVAL_20MS  = 20;
constexpr int DEFAULT_PROFILING_INTERVAL_50MS  = 50;
constexpr int DEFAULT_PROFILING_INTERVAL_100MS = 100;
constexpr int DEFAULT_PROFILING_BIU_FREQ = 1000;    // 1000 cycle

enum FileChunkDataModule {
    PROFILING_DEFAULT_DATA_MODULE = 0,
    PROFILING_IS_FROM_MSPROF,
    PROFILING_IS_CTRL_DATA,
    PROFILING_IS_FROM_DEVICE,
    PROFILING_IS_FROM_MSPROF_DEVICE,
    PROFILING_IS_FROM_MSPROF_HOST
};

// biu perf
constexpr int BIU_SAMPLE_FREQ_MIN = 300;    // biu sampling frequency min value
constexpr int BIU_SAMPLE_FREQ_MAX = 30000;  // biu sampling frequency max value

// device-sys
constexpr int SYS_SAMPLING_FREQ_MIN_NUM = 1;
constexpr int SYS_SAMPLING_FREQ_MAX_NUM = 10;
constexpr int PID_SAMPLING_FREQ_MIN_NUM = 1;
constexpr int PID_SAMPLING_FREQ_MAX_NUM = 10;
constexpr int CPU_SAMPLING_FREQ_MIN_NUM = 1;
constexpr int CPU_SAMPLING_FREQ_MAX_NUM = 50;
constexpr int INTERCONNECTION_SAMPLING_FREQ_MIN_NUM = 1;
constexpr int INTERCONNECTION_SAMPLING_FREQ_MAX_NUM = 50;
constexpr int IO_SAMPLING_FREQ_MIN_NUM = 1;
constexpr int IO_SAMPLING_FREQ_MAX_NUM = 100;
constexpr int DVPP_SAMPLING_FREQ_MIN_NUM = 1;
constexpr int DVPP_SAMPLING_FREQ_MAX_NUM = 100;
constexpr int HARDWARE_MEM_SAMPLING_FREQ_MIN_NUM = 1;
constexpr int HARDWARE_MEM_SAMPLING_FREQ_MAX_NUM = 1000;
constexpr int AIC_SAMPLING_FREQ_MIN_NUM = 1;
constexpr int AIC_SAMPLING_FREQ_MAX_NUM = 100;
constexpr int AIV_SAMPLING_FREQ_MIN_NUM = 1;
constexpr int AIV_SAMPLING_FREQ_MAX_NUM = 100;
constexpr int L2_SAMPLING_FREQ_MIN_NUM = 1;
constexpr int L2_SAMPLING_FREQ_MAX_NUM = 100;
}  // namespace config
}  // namespace common
}  // namespace dvvp
}  // namespace analysis

#endif
