#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include <string.h>
#include <google/protobuf/util/json_util.h>
#include "config/config_manager.h"
#include "data_struct.h"
#include "errno/error_code.h"
#include "ge/ge_prof.h"
#include "message/codec.h"
#include "msprof_callback_handler.h"
#include "msprof_callback_impl.h"
#include "op_desc_parser.h"
#include "prof_api_common.h"
#include "prof_acl_mgr.h"
#include "profiling/ge_profiling.h"
#include "proto/profiler_ext.pb.h"
#include "proto/profiler.pb.h"
#include "uploader_dumper.h"
#include "uploader_mgr.h"
#include "utils/utils.h"
#include "prof_acl_core.h"
#include "prof_ge_core.h"
#include "analyzer_ge.h"
#include "uploader.h"
#include "uploader_mgr.h"
#include "analyzer_ts.h"
#include "analyzer.h"
#include "analyzer_hwts.h"
#include "analyzer_ffts.h"
#include "transport/parser_transport.h"
#include "transport/transport.h"
#include "prof_manager.h"
#include "prof_params_adapter.h"
#include "msprof_tx_manager.h"
#include "platform/platform.h"
#include "command_handle.h"
#include "acl_prof.h"
#include "data_struct.h"
#include "ai_drv_dev_api.h"
#include "mmpa_api.h"
#include "prof_api.h"
#include "toolchain/prof_acl_api.h"
#include "uploader.h"
#include "transport/hdc/hdc_transport.h"

using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::Analyze;
using namespace analysis::dvvp::transport;
using namespace Analysis::Dvvp::ProfilerCommon;
using namespace Analysis::Dvvp::Common::Platform;
using namespace Collector::Dvvp::Plugin;
using namespace Collector::Dvvp::Mmpa;
const int RECEIVE_CHUNK_SIZE = 320; // chunk size:320

class MSPROF_ACL_CORE_UTEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

int aclApiSubscribeCount = 0;

int mmWriteStub(int fd, void *buf, unsigned int bufLen) {
    using namespace Analysis::Dvvp::Analyze;
    MSPROF_LOGI("mmWriteStub, bufLen: %u", bufLen);
    aclApiSubscribeCount++;
    EXPECT_EQ(sizeof(ProfOpDesc), bufLen);
    EXPECT_EQ(sizeof(ProfOpDesc), OpDescParser::GetOpDescSize());
    uint32_t opNum = 0;

    EXPECT_EQ(ACL_SUCCESS, OpDescParser::GetOpNum(buf, bufLen, &opNum));
    EXPECT_EQ(1, opNum);

    uint32_t modelId = 0;
    OpDescParser::GetModelId(buf, bufLen, 0, &modelId);
    EXPECT_EQ(1, modelId);
    size_t opTypeLen = 0;
    EXPECT_EQ(ACL_SUCCESS, OpDescParser::instance()->GetOpTypeLen(buf, bufLen, &opTypeLen, 0));
    char opType[opTypeLen];
    EXPECT_EQ(ACL_SUCCESS, OpDescParser::instance()->GetOpType(buf, bufLen, opType, opTypeLen, 0));
    EXPECT_EQ("OpType", std::string(opType));
    size_t opNameLen = 0;
    EXPECT_EQ(ACL_SUCCESS, OpDescParser::instance()->GetOpNameLen(buf, bufLen, &opNameLen, 0));
    char opName[opNameLen];
    EXPECT_EQ(ACL_SUCCESS, OpDescParser::instance()->GetOpName(buf, bufLen, opName, opNameLen, 0));
    EXPECT_EQ("OpName", std::string(opName));
    EXPECT_EQ(true, OpDescParser::GetOpStart(buf, bufLen, 0) > 0);
    EXPECT_EQ(true, OpDescParser::GetOpEnd(buf, bufLen, 0) > 0);
    EXPECT_EQ(true, OpDescParser::GetOpDuration(buf, bufLen, 0) > 0);
    //EXPECT_EQ(true, Msprofiler::Api::ProfGetOpExecutionTime(buf, bufLen, 0) >= 0);
    // EXPECT_EQ(true, ProfGetOpExecutionTime(buf, bufLen, 0) >= 0); XXX
    EXPECT_EQ(true, OpDescParser::GetOpCubeFops(buf, bufLen, 0) >= 0);
    EXPECT_EQ(true, OpDescParser::GetOpVectorFops(buf, bufLen, 0) >= 0);
    EXPECT_EQ(0, OpDescParser::GetOpFlag(buf, bufLen, 0));
    EXPECT_NE(nullptr, OpDescParser::GetOpAttriValue(buf, bufLen, 0, ACL_SUBSCRIBE_ATTRI_THREADID));   //
    EXPECT_EQ(nullptr, OpDescParser::GetOpAttriValue(buf, bufLen, 0, ACL_SUBSCRIBE_ATTRI_NONE));

    // for aclapi
    size_t opSize = 0;
    aclprofGetOpDescSize(&opSize);
    aclprofGetOpNum(buf, bufLen, &opNum);
    aclprofGetOpTypeLen(buf, bufLen, 0, &opTypeLen);
    aclprofGetOpType(buf, bufLen, 0, opType, opTypeLen);
    aclprofGetOpNameLen(buf, bufLen, 0, &opNameLen);
    aclprofGetOpName(buf, bufLen, 0, opName, opNameLen);
    aclprofGetOpStart(buf, bufLen, 0);
    aclprofGetOpEnd(buf, bufLen, 0);
    aclprofGetOpDuration(buf, bufLen, 0);
    aclprofGetModelId(buf, bufLen, 0);
    aclprofGetGraphId(buf, bufLen, 0);
    aclprofGetModelId(nullptr, 0, 0);
    aclprofGetModelId(nullptr, 0, 0);
    return bufLen;
}

TEST_F(MSPROF_ACL_CORE_UTEST, get_op_xxx) {
    ProfOpDesc data;
    uint32_t bufLen = sizeof(ProfOpDesc);
    data.modelId = 0;
    data.flag = ACL_SUBSCRIBE_OP_THREAD;
    data.threadId = 0;
    data.modelId = 0;
    data.opIndex = OpDescParser::instance()->SetOpTypeAndOpName("OpType", "OpName");
    data.executionTime = 0;
    data.signature = analysis::dvvp::common::utils::Utils::GenerateSignature(
        reinterpret_cast<const uint8_t *>(&data) + sizeof(uint32_t), bufLen - sizeof(uint32_t));
    
    EXPECT_EQ(ACL_SUBSCRIBE_OP_THREAD, OpDescParser::GetOpFlag(&data, sizeof(data), 0));
    EXPECT_NE(nullptr, OpDescParser::GetOpAttriValue(&data, sizeof(data), 0, ACL_SUBSCRIBE_ATTRI_THREADID));
    EXPECT_EQ(nullptr, OpDescParser::GetOpAttriValue(&data, sizeof(data), 0, ACL_SUBSCRIBE_ATTRI_NONE));

    EXPECT_EQ(sizeof(ProfOpDesc), OpDescParser::GetOpDescSize());
    uint32_t opNum = 0;
    EXPECT_EQ(ACL_SUCCESS, OpDescParser::GetOpNum(&data, bufLen, &opNum));
    EXPECT_EQ(1, opNum);

    uint32_t modelId = 0;
    OpDescParser::GetModelId(&data, bufLen, 0, &modelId);
    EXPECT_EQ(0, modelId);
    size_t opTypeLen = 0;
    EXPECT_EQ(ACL_SUCCESS, OpDescParser::instance()->GetOpTypeLen(&data, bufLen, &opTypeLen, 0));
    char opType[opTypeLen];
    EXPECT_EQ(ACL_SUCCESS, OpDescParser::instance()->GetOpType(&data, bufLen, opType, opTypeLen, 0));
    EXPECT_EQ("OpType", std::string(opType));
    size_t opNameLen = 0;
    EXPECT_EQ(ACL_SUCCESS, OpDescParser::instance()->GetOpNameLen(&data, bufLen, &opNameLen, 0));
    char opName[opNameLen];
    EXPECT_EQ(ACL_SUCCESS, OpDescParser::instance()->GetOpName(&data, bufLen, opName, opNameLen, 0));
    EXPECT_EQ("OpName", std::string(opName));
    EXPECT_EQ(true, OpDescParser::GetOpStart(&data, bufLen, 0) > 0);
    EXPECT_EQ(true, OpDescParser::GetOpEnd(&data, bufLen, 0) > 0);
    EXPECT_EQ(true, OpDescParser::GetOpDuration(&data, bufLen, 0) > 0);
    EXPECT_EQ(true, OpDescParser::GetOpCubeFops(&data, bufLen, 0) >= 0);
    EXPECT_EQ(true, OpDescParser::GetOpVectorFops(&data, bufLen, 0) >= 0);
    EXPECT_EQ(ACL_SUBSCRIBE_OP_THREAD, OpDescParser::GetOpFlag(&data, bufLen, 0));
    EXPECT_NE(nullptr, OpDescParser::GetOpAttriValue(&data, bufLen, 0, ACL_SUBSCRIBE_ATTRI_THREADID));
    EXPECT_EQ(nullptr, OpDescParser::GetOpAttriValue(&data, bufLen, 0, ACL_SUBSCRIBE_ATTRI_NONE));
    EXPECT_EQ(0, OpDescParser::GetOpExecutionTime(&data, bufLen, 0));
}

TEST_F(MSPROF_ACL_CORE_UTEST, ConstructAndUploadData) {
    GlobalMockObject::verify();

    std::shared_ptr<Analyzer> analyzer(new Analyzer(nullptr));
    const uint64_t start = 1;
    const uint64_t end = 2;
    const uint64_t startAicore = 3;
    const uint64_t endAicore = 4;
    OpTime opTime;
    opTime.start = start;
    opTime.end = end;
    opTime.startAicore = startAicore;
    opTime.endAicore = endAicore;
    opTime.indexId = 1;
    opTime.threadId = 1;
    const std::string nullStr = "";
    analyzer->ConstructAndUploadData(nullStr, opTime);
    const std::string opId = "123";
    analyzer->ConstructAndUploadData(opId, opTime);
}

TEST_F(MSPROF_ACL_CORE_UTEST, acl_api) {
    GlobalMockObject::verify();
    std::string result = "/tmp/acl_api_stest_new";
    analysis::dvvp::common::utils::Utils::RemoveDir(result);
    analysis::dvvp::common::utils::Utils::CreateDir(result);
    EXPECT_EQ(ge::SUCCESS, ge::aclgrphProfInit(result.c_str(), result.size()));
    std::string empty = "";
    EXPECT_EQ(ge::FAILED, ge::aclgrphProfInit(empty.c_str(), empty.size()));

    ProfConfig config;
    config.devNums = 1;
    config.devIdList[0] = 0;
    config.aicoreMetrics = PROF_AICORE_ARITHMETIC_UTILIZATION;
    config.dataTypeConfig = 0x7d7f001f;

    ge::aclgrphProfConfig *aclConfig = ge::aclgrphProfCreateConfig(
        config.devIdList, config.devNums, (ge::ProfilingAicoreMetrics)config.aicoreMetrics, nullptr, config.dataTypeConfig);
    ge::aclgrphProfConfig *zeroConfig = ge::aclgrphProfCreateConfig(
        config.devIdList, config.devNums, (ge::ProfilingAicoreMetrics)config.aicoreMetrics, nullptr, config.dataTypeConfig);
    ge::aclgrphProfConfig *invalidConfig = ge::aclgrphProfCreateConfig(
        config.devIdList, config.devNums, (ge::ProfilingAicoreMetrics)config.aicoreMetrics, nullptr, 0xffffffff);
    memset(zeroConfig, 0, sizeof(ProfConfig));

    EXPECT_NE(nullptr, aclConfig);

    EXPECT_EQ(ge::FAILED, ge::aclgrphProfStart(zeroConfig));
    EXPECT_EQ(ge::FAILED, ge::aclgrphProfStart(invalidConfig));
    EXPECT_EQ(ge::SUCCESS, ge::aclgrphProfStart(aclConfig));

    ProfConfig largeConfig;
    largeConfig.devNums = MSVP_MAX_DEV_NUM + 1;
    largeConfig.devIdList[0] = 0;
    largeConfig.aicoreMetrics = PROF_AICORE_ARITHMETIC_UTILIZATION;
    largeConfig.dataTypeConfig = 0x7d7f001f;
    ge::aclgrphProfConfig *largeAclConfig = ge::aclgrphProfCreateConfig(
        largeConfig.devIdList, largeConfig.devNums, (ge::ProfilingAicoreMetrics)largeConfig.aicoreMetrics, nullptr, largeConfig.dataTypeConfig);

    EXPECT_EQ(nullptr, largeAclConfig);

    Msprof::Engine::MsprofCallbackHandler::reporters_.clear();
    EXPECT_EQ(PROFILING_SUCCESS, Analysis::Dvvp::ProfilerCommon::RegisterReporterCallback());
    Analysis::Dvvp::ProfilerCommon::MsprofReporterCallbackImpl(MSPROF_MODULE_DATA_PREPROCESS, MSPROF_REPORTER_INIT, nullptr, 0);
    MsprofHashData hData = {0};
    std::string hashData = "profiling_ut_data";
    hData.dataLen = hashData.size();
    hData.data = reinterpret_cast<unsigned char *>(const_cast<char *>(hashData.c_str()));
    hData.hashId = 0;
    Analysis::Dvvp::ProfilerCommon::MsprofReporterCallbackImpl(MSPROF_MODULE_DATA_PREPROCESS, MSPROF_REPORTER_HASH, (void *)&hData, sizeof(hData));
    EXPECT_NE(0, hData.hashId);

    ReporterData data = {0};
    data.tag[0] = 't';
    data.deviceId = 0;
    std::string reportData = "profiling_st_data";
    data.dataLen = reportData.size();
    data.data = const_cast<unsigned char *>((const unsigned char *)reportData.c_str());
    Analysis::Dvvp::ProfilerCommon::MsprofReporterCallbackImpl(MSPROF_MODULE_DATA_PREPROCESS, MSPROF_REPORTER_REPORT, (void *)&data, sizeof(data));
    uint32_t dataMaxLen = 0;
    int ret = Msprof::Engine::MsprofCallbackHandler::reporters_[MSPROF_MODULE_DATA_PREPROCESS].GetDataMaxLen(&dataMaxLen, 1);
    EXPECT_EQ(PROFILING_FAILED, ret);
    ret = Msprof::Engine::MsprofCallbackHandler::reporters_[MSPROF_MODULE_DATA_PREPROCESS].GetDataMaxLen(&dataMaxLen, 16);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    EXPECT_EQ(RECEIVE_CHUNK_SIZE, dataMaxLen);
    Analysis::Dvvp::ProfilerCommon::MsprofReporterCallbackImpl(MSPROF_MODULE_DATA_PREPROCESS, MSPROF_REPORTER_UNINIT, nullptr, 0);
    Msprof::Engine::MsprofCallbackHandler::reporters_[MSPROF_MODULE_DATA_PREPROCESS].ForceFlush("0");
    ret = Msprof::Engine::MsprofCallbackHandler::reporters_[MSPROF_MODULE_DATA_PREPROCESS].GetDataMaxLen(&dataMaxLen, 1);
    EXPECT_EQ(PROFILING_FAILED, ret);

    EXPECT_EQ(ge::FAILED, ge::aclgrphProfStop(nullptr));
    EXPECT_EQ(ge::FAILED, ge::aclgrphProfStop(zeroConfig));
    EXPECT_EQ(ge::SUCCESS, ge::aclgrphProfStop(aclConfig));

    EXPECT_EQ(ge::SUCCESS, ge::aclgrphProfFinalize());

    EXPECT_EQ(ge::SUCCESS, ge::aclgrphProfDestroyConfig(zeroConfig));
    EXPECT_EQ(ge::SUCCESS, ge::aclgrphProfDestroyConfig(invalidConfig));
    EXPECT_EQ(ge::SUCCESS, ge::aclgrphProfDestroyConfig(aclConfig));

    config.devNums = 8;
    config.devIdList[0] = 0;
    config.aicoreMetrics = PROF_AICORE_ARITHMETIC_UTILIZATION;
    config.dataTypeConfig = 0x7d7f001f;
    MOCKER_CPP(&analysis::dvvp::driver::DrvGetDevNum)
        .stubs()
        .will(returnValue(5));
    invalidConfig = ge::aclgrphProfCreateConfig(
        config.devIdList, config.devNums, (ge::ProfilingAicoreMetrics)config.aicoreMetrics, nullptr, config.dataTypeConfig);
    EXPECT_EQ(nullptr, invalidConfig);

    config.devNums = 3;
    config.devIdList[0] = 6;
    invalidConfig = ge::aclgrphProfCreateConfig(
        config.devIdList, config.devNums, (ge::ProfilingAicoreMetrics)config.aicoreMetrics, nullptr, config.dataTypeConfig);
    EXPECT_EQ(nullptr, invalidConfig);

    analysis::dvvp::common::utils::Utils::RemoveDir(result);
}

TEST_F(MSPROF_ACL_CORE_UTEST, GeOpenDeviceHandle) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::MsprofSetDeviceImpl)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    MOCKER_CPP(&Analysis::Dvvp::ProfilerCommon::CommandHandleProfInit)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::GetCmdModeDataTypeConfig)
        .stubs()
        .will(returnValue((uint64_t)0));

    MOCKER_CPP(&Analysis::Dvvp::ProfilerCommon::CommandHandleProfStart)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    ge::GeOpenDeviceHandle(1);
    ge::GeOpenDeviceHandle(1);
    ge::GeOpenDeviceHandle(1);
    ge::GeOpenDeviceHandle(1);
}

TEST_F(MSPROF_ACL_CORE_UTEST, aclgrphProfInit_failed) {
    GlobalMockObject::verify();

    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::ProfInitPrecheck)
        .stubs()
        .will(returnValue(ACL_SUCCESS))
        .then(returnValue(ACL_SUCCESS));

    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::Init)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS))
        .then(returnValue(PROFILING_SUCCESS));

    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::ProfAclInit)
        .stubs()
        .will(returnValue(ACL_ERROR_PROFILING_FAILURE))
        .then(returnValue(ACL_SUCCESS));

    std::string result = "/tmp/aclgrphProfInit_failed";
    analysis::dvvp::common::utils::Utils::RemoveDir(result);
    analysis::dvvp::common::utils::Utils::CreateDir(result);
    EXPECT_EQ(ge::FAILED, ge::aclgrphProfInit(result.c_str(), result.size()));
    analysis::dvvp::common::utils::Utils::RemoveDir(result);
}

int RelativePathToAbsolutePath_stub_cnt = 0;
std::string RelativePathToAbsolutePath_stub(const std::string &path)
{
    RelativePathToAbsolutePath_stub_cnt++;
    if (RelativePathToAbsolutePath_stub_cnt == 1) {
        return "";
    } else {
        return "/tmp/MSPROF_ACL_CORE_UTEST_ProfAclInit_failed";
    }
}

TEST_F(MSPROF_ACL_CORE_UTEST, ProfAclInit_failed) {
    GlobalMockObject::verify();
    Msprofiler::Api::ProfAclMgr::instance()->Init();
    std::string result = "/tmp/MSPROF_ACL_CORE_UTEST_ProfAclInit_failed";

    MOCKER(analysis::dvvp::common::utils::Utils::RelativePathToAbsolutePath)
        .stubs()
        .will(invoke(RelativePathToAbsolutePath_stub));

    MOCKER(analysis::dvvp::common::utils::Utils::CreateDir)
        .stubs()
        .will(returnValue(PROFILING_FAILED));

    Msprofiler::Api::ProfAclMgr::instance()->isReady_ = true;
    Msprofiler::Api::ProfAclMgr::instance()->mode_ = Msprofiler::Api::WORK_MODE_OFF;

    analysis::dvvp::common::utils::Utils::CreateDir(result);
    EXPECT_EQ(ACL_ERROR_INVALID_FILE, Msprofiler::Api::ProfAclMgr::instance()->ProfAclInit(result.c_str()));
    EXPECT_EQ(ACL_ERROR_INVALID_FILE, Msprofiler::Api::ProfAclMgr::instance()->ProfAclInit(result.c_str()));
    analysis::dvvp::common::utils::Utils::RemoveDir(result);
}

TEST_F(MSPROF_ACL_CORE_UTEST, acl_prof_api) {
    GlobalMockObject::verify();

    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::Init)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    std::string result = "/tmp/acl_prof_api_stest_new";
    analysis::dvvp::common::utils::Utils::RemoveDir(result);
    analysis::dvvp::common::utils::Utils::CreateDir(result);
    EXPECT_EQ(ACL_SUCCESS, aclprofInit(result.c_str(), result.size()));
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, aclprofInit(nullptr, result.size()));
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, aclprofInit(result.c_str(), 0));
    std::string empty = "";
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, aclprofInit(empty.c_str(), 0));

    ProfConfig config;
    config.devNums = 1;
    config.devIdList[0] = 0;
    config.aicoreMetrics = PROF_AICORE_ARITHMETIC_UTILIZATION;
    config.dataTypeConfig = 0x7d7f001f;

    aclprofConfig *aclConfig = aclprofCreateConfig(
        config.devIdList, config.devNums, (aclprofAicoreMetrics)config.aicoreMetrics, nullptr, config.dataTypeConfig);
    aclprofConfig *zeroConfig = aclprofCreateConfig(
        config.devIdList, config.devNums, (aclprofAicoreMetrics)config.aicoreMetrics, nullptr, config.dataTypeConfig);
    aclprofConfig *invalidConfig = aclprofCreateConfig(
        config.devIdList, config.devNums, (aclprofAicoreMetrics)config.aicoreMetrics, nullptr, 0xffffffff);
    memset(zeroConfig, 0, sizeof(ProfConfig));

    EXPECT_NE(nullptr, aclConfig);

    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, aclprofStart(nullptr));
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, aclprofStart(zeroConfig));
    EXPECT_EQ(200007, aclprofStart(invalidConfig));
    EXPECT_EQ(0, aclprofStart(aclConfig));

    ProfConfig largeConfig;
    largeConfig.devNums = MSVP_MAX_DEV_NUM + 1;
    largeConfig.devIdList[0] = 0;
    largeConfig.aicoreMetrics = PROF_AICORE_ARITHMETIC_UTILIZATION;
    largeConfig.dataTypeConfig = 0x7d7f001f;
    aclprofConfig *largeAclConfig = aclprofCreateConfig(
        largeConfig.devIdList, largeConfig.devNums, (aclprofAicoreMetrics)largeConfig.aicoreMetrics, nullptr, largeConfig.dataTypeConfig);
    EXPECT_EQ(nullptr, largeAclConfig);

    Analysis::Dvvp::ProfilerCommon::MsprofReporterCallbackImpl(MSPROF_MODULE_DATA_PREPROCESS, MSPROF_REPORTER_INIT, nullptr, 0);
    ReporterData data = {0};
    data.tag[0] = 't';
    data.deviceId = 0;
    std::string reportData = "profiling_st_data";
    data.dataLen = reportData.size();
    data.data = const_cast<unsigned char *>((const unsigned char *)reportData.c_str());
    Analysis::Dvvp::ProfilerCommon::MsprofReporterCallbackImpl(MSPROF_MODULE_DATA_PREPROCESS, MSPROF_REPORTER_REPORT, (void *)&data, sizeof(data));
    uint32_t dataMaxLen = 0;
    int ret = Msprof::Engine::MsprofCallbackHandler::reporters_[MSPROF_MODULE_DATA_PREPROCESS].GetDataMaxLen(&dataMaxLen, 1);
    EXPECT_EQ(PROFILING_FAILED, ret);
    ret = Msprof::Engine::MsprofCallbackHandler::reporters_[MSPROF_MODULE_DATA_PREPROCESS].GetDataMaxLen(&dataMaxLen, 16);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(RECEIVE_CHUNK_SIZE, dataMaxLen);
    Analysis::Dvvp::ProfilerCommon::MsprofReporterCallbackImpl(MSPROF_MODULE_DATA_PREPROCESS, MSPROF_REPORTER_UNINIT, nullptr, 0);
    Msprof::Engine::MsprofCallbackHandler::reporters_[MSPROF_MODULE_DATA_PREPROCESS].ForceFlush("0");
    ret = Msprof::Engine::MsprofCallbackHandler::reporters_[MSPROF_MODULE_DATA_PREPROCESS].GetDataMaxLen(&dataMaxLen, 1);
    EXPECT_EQ(PROFILING_FAILED, ret);

    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, aclprofStop(nullptr));
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, aclprofStop(zeroConfig));
    EXPECT_EQ( 0, aclprofStop(aclConfig));

    EXPECT_EQ(MSPROF_ERROR_NONE, aclprofFinalize());

    EXPECT_EQ(ge::SUCCESS, aclprofDestroyConfig(zeroConfig));
    EXPECT_EQ(ge::SUCCESS, aclprofDestroyConfig(invalidConfig));
    EXPECT_EQ(ge::SUCCESS, aclprofDestroyConfig(aclConfig));
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, aclprofDestroyConfig(nullptr));

    config.devNums = 8;
    config.devIdList[0] = 0;
    config.aicoreMetrics = PROF_AICORE_ARITHMETIC_UTILIZATION;
    config.dataTypeConfig = 0x7d7f001f;
    MOCKER_CPP(&analysis::dvvp::driver::DrvGetDevNum)
        .stubs()
        .will(returnValue(5));
    invalidConfig = aclprofCreateConfig(
        config.devIdList, config.devNums, (aclprofAicoreMetrics)config.aicoreMetrics, nullptr, config.dataTypeConfig);
    EXPECT_EQ(nullptr, invalidConfig);

    config.devNums = 3;
    config.devIdList[0] = 6;
    invalidConfig = aclprofCreateConfig(
        config.devIdList, config.devNums, (aclprofAicoreMetrics)config.aicoreMetrics, nullptr, config.dataTypeConfig);
    EXPECT_EQ(nullptr, invalidConfig);
    using namespace Msprofiler::Api;
    ProfAclMgr::instance()->mode_ = WORK_MODE_OFF;
    analysis::dvvp::common::utils::Utils::RemoveDir(result);
}

TEST_F(MSPROF_ACL_CORE_UTEST, prof_acl_api_helper) {
    GlobalMockObject::verify();

    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::PlatformIsHelperHostSide)
        .stubs()
        .will(returnValue(true));
    const char *path = "/home/user/workSpace";
    size_t len = strlen(path); 
    EXPECT_EQ(ACL_ERROR_FEATURE_UNSUPPORTED, aclprofInit(path, len));
    EXPECT_EQ(ACL_ERROR_FEATURE_UNSUPPORTED, aclprofFinalize());
    ProfConfig config;
    config.devNums = 1;
    config.devIdList[0] = 0;
    config.aicoreMetrics = PROF_AICORE_ARITHMETIC_UTILIZATION;
    config.dataTypeConfig = 0x7d7f001f;
    EXPECT_EQ(nullptr, aclprofCreateConfig(config.devIdList, config.devNums, (aclprofAicoreMetrics)config.aicoreMetrics, nullptr, config.dataTypeConfig));
    EXPECT_EQ(ACL_ERROR_FEATURE_UNSUPPORTED, aclprofDestroyConfig(nullptr));
    EXPECT_EQ(ACL_ERROR_FEATURE_UNSUPPORTED, aclprofStart(nullptr));
    EXPECT_EQ(ACL_ERROR_FEATURE_UNSUPPORTED, aclprofStop(nullptr));
    EXPECT_EQ(ACL_ERROR_FEATURE_UNSUPPORTED, aclprofModelSubscribe(1, nullptr));
    EXPECT_EQ(ACL_ERROR_FEATURE_UNSUPPORTED, aclprofModelUnSubscribe(1));
    EXPECT_EQ((size_t)ACL_ERROR_FEATURE_UNSUPPORTED, aclprofGetModelId(nullptr, 0, 0));
    // EXPECT_EQ(0, ProfGetOpExecutionTime(nullptr, 0, 0)); XXX
    aclrtStream stream;
    EXPECT_EQ(ACL_ERROR_FEATURE_UNSUPPORTED, aclprofGetStepTimestamp(nullptr, ACL_STEP_START, stream));
    EXPECT_EQ(nullptr, aclprofCreateStepInfo());
    aclprofDestroyStepInfo(nullptr);
    EXPECT_EQ(nullptr, aclprofCreateStamp());
    aclprofDestroyStamp(nullptr);
    EXPECT_EQ(ACL_ERROR_FEATURE_UNSUPPORTED, aclprofSetCategoryName(0, nullptr));
    EXPECT_EQ(ACL_ERROR_FEATURE_UNSUPPORTED, aclprofSetStampCategory(nullptr, 0));
    EXPECT_EQ(ACL_ERROR_FEATURE_UNSUPPORTED, aclprofSetStampPayload(nullptr, 0, nullptr));
    EXPECT_EQ(ACL_ERROR_FEATURE_UNSUPPORTED, aclprofSetStampTraceMessage(nullptr, nullptr, 0));
    EXPECT_EQ(ACL_ERROR_FEATURE_UNSUPPORTED, aclprofMark(nullptr));
    EXPECT_EQ(ACL_ERROR_FEATURE_UNSUPPORTED, aclprofPush(nullptr));
    EXPECT_EQ(ACL_ERROR_FEATURE_UNSUPPORTED, aclprofPop());
    EXPECT_EQ(ACL_ERROR_FEATURE_UNSUPPORTED, aclprofRangeStart(nullptr, nullptr));
    EXPECT_EQ(ACL_ERROR_FEATURE_UNSUPPORTED, aclprofRangeStop(0));
}

TEST_F(MSPROF_ACL_CORE_UTEST, acl_prof_init_failed) {
    GlobalMockObject::verify();

    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::ProfInitPrecheck)
        .stubs()
        .will(returnValue(ACL_SUCCESS))
        .then(returnValue(ACL_SUCCESS));

    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::Init)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS))
        .then(returnValue(PROFILING_SUCCESS));

    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::ProfAclInit)
        .stubs()
        .will(returnValue(ACL_ERROR_PROFILING_FAILURE))
        .then(returnValue(ACL_SUCCESS));

    std::string result = "/tmp/acl_prof_init_failed";
    analysis::dvvp::common::utils::Utils::RemoveDir(result);
    analysis::dvvp::common::utils::Utils::CreateDir(result);
    EXPECT_EQ(ACL_ERROR_PROFILING_FAILURE, aclprofInit(result.c_str(), result.size()));
}

TEST_F(MSPROF_ACL_CORE_UTEST, aclprofCreateSubscribeConfig) {
    GlobalMockObject::verify();

    using namespace Msprofiler::Api;

    int fd = 1;
    ProfSubscribeConfig config;
    config.timeInfo = true;
    config.aicoreMetrics = PROF_AICORE_NONE;
    config.fd = static_cast<void *>(&fd);

    EXPECT_EQ(nullptr, aclprofCreateSubscribeConfig(0, (aclprofAicoreMetrics)config.aicoreMetrics, nullptr));
    EXPECT_EQ(nullptr, aclprofCreateSubscribeConfig(2, (aclprofAicoreMetrics)config.aicoreMetrics, config.fd));
    auto profSubconfig = aclprofCreateSubscribeConfig(1, (aclprofAicoreMetrics)config.aicoreMetrics, config.fd);

    EXPECT_NE(nullptr, profSubconfig);
    aclprofDestroySubscribeConfig(profSubconfig);
}

TEST_F(MSPROF_ACL_CORE_UTEST, acl_api_subscribe) {
    GlobalMockObject::verify();

    Analysis::Dvvp::Common::Config::ConfigManager::instance()->Init();

    MOCKER(&MmWrite)
        .stubs()
        .will(invoke(mmWriteStub));
    MOCKER(&MmClose)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&analysis::dvvp::host::ProfManager::IdeCloudProfileProcess)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS))
        .then(returnValue(PROFILING_SUCCESS));

    using namespace Msprofiler::Api;

    int fd = 1;
    ProfSubscribeConfig config;
    config.timeInfo = true;
    config.aicoreMetrics = PROF_AICORE_NONE;
    config.fd = static_cast<void *>(&fd);

    EXPECT_EQ(nullptr, aclprofCreateSubscribeConfig(0, (aclprofAicoreMetrics)config.aicoreMetrics, nullptr));
    EXPECT_EQ(nullptr, aclprofCreateSubscribeConfig(0, (aclprofAicoreMetrics)config.aicoreMetrics, nullptr));
    auto profSubconfig = aclprofCreateSubscribeConfig(1, (aclprofAicoreMetrics)config.aicoreMetrics, config.fd);

    EXPECT_NE(nullptr, profSubconfig);

    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, aclprofModelSubscribe(1, nullptr));

    EXPECT_EQ(ACL_SUCCESS, aclprofModelSubscribe(3, profSubconfig));

    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->Init());

    uint32_t devId = 0;
    EXPECT_EQ(0, ProfAclMgr::instance()->GetDeviceSubscribeCount(1, devId));
    EXPECT_EQ(devId, 0);

    config.timeInfo = false;
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, ProfAclMgr::instance()->ProfAclModelSubscribe(1, 0, &config));
    config.timeInfo = true;
    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->ProfAclModelSubscribe(1, 0, &config));
    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->ProfAclModelSubscribe(2, 0, &config));
    EXPECT_EQ(2, ProfAclMgr::instance()->GetDeviceSubscribeCount(1, devId));
    EXPECT_EQ(devId, 0);

    SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> geIdMap(new analysis::dvvp::proto::FileChunkReq());
    MsprofGeProfIdMapData idMapData;
    geIdMap->set_filename("Framework");
    geIdMap->set_tag("id_map_info");
    std::string geOriData0((char *)&idMapData, sizeof(idMapData));
    geIdMap->set_chunk(geOriData0);
    geIdMap->set_chunksizeinbytes(sizeof(idMapData));
    std::string data0 = analysis::dvvp::message::EncodeMessage(geIdMap);
    analysis::dvvp::transport::UploaderMgr::instance()->UploadData(
        "0", static_cast<const void *>(data0.c_str()), data0.size());

    SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> geTaskDesc(new analysis::dvvp::proto::FileChunkReq());
    struct MsprofGeProfTaskData geTaskDescChunk1;
    struct MsprofGeProfTaskData geTaskDescChunk2;
    geTaskDescChunk1.opName.type = 1;
    geTaskDescChunk2.opName.type = 1;    
    geTaskDescChunk1.opType.type = 0;
    geTaskDescChunk2.opType.type = 0;
    geTaskDesc->set_filename("Framework");
    geTaskDesc->set_tag("task_desc_info");
    std::string geOriData1((char *)&geTaskDescChunk1, sizeof(geTaskDescChunk1));
    geTaskDesc->set_chunk(geOriData1);
    geTaskDesc->set_chunksizeinbytes(sizeof(geTaskDescChunk1));
    std::string data = analysis::dvvp::message::EncodeMessage(geTaskDesc);
    analysis::dvvp::transport::UploaderMgr::instance()->UploadData(
        "0", static_cast<const void *>(data.c_str()), data.size());
    std::string geOriData2((char *)&geTaskDescChunk2, sizeof(geTaskDescChunk2));
    geTaskDesc->set_chunk(geOriData2);
    geTaskDesc->set_chunksizeinbytes(sizeof(geTaskDescChunk2));
    data = analysis::dvvp::message::EncodeMessage(geTaskDesc);
    analysis::dvvp::transport::UploaderMgr::instance()->UploadData(
        "0", static_cast<const void *>(data.c_str()), data.size());

    using namespace Analysis::Dvvp::Analyze;
    TsProfileTimeline tsChunk;
    tsChunk.head.rptType = TS_TIMELINE_RPT_TYPE;
    tsChunk.head.bufSize = sizeof(TsProfileTimeline);
    tsChunk.taskId = 30;
    tsChunk.streamId = 100;
    tsChunk.taskState = TS_TIMELINE_START_TASK_STATE;
    tsChunk.timestamp = 10000;
    SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> tsTimeline(new analysis::dvvp::proto::FileChunkReq());
    tsTimeline->set_filename("ts_track.data");
    tsTimeline->set_chunk(reinterpret_cast<char *>(&tsChunk), tsChunk.head.bufSize);
    tsTimeline->set_chunksizeinbytes(tsChunk.head.bufSize);
    data = analysis::dvvp::message::EncodeMessage(tsTimeline);
    analysis::dvvp::transport::UploaderMgr::instance()->UploadData(
        "0", static_cast<const void *>(data.c_str()), data.size());

    tsChunk.taskState = TS_TIMELINE_AICORE_START_TASK_STATE;
    tsChunk.timestamp = 20000;
    tsTimeline->set_chunk(reinterpret_cast<char *>(&tsChunk), tsChunk.head.bufSize);
    tsTimeline->set_chunksizeinbytes(tsChunk.head.bufSize);
    data = analysis::dvvp::message::EncodeMessage(tsTimeline);
    analysis::dvvp::transport::UploaderMgr::instance()->UploadData(
        "0", static_cast<const void *>(data.c_str()), data.size());

    tsChunk.taskState = TS_TIMELINE_AICORE_END_TASK_STATE;
    tsChunk.timestamp = 30000;
    tsTimeline->set_chunk(reinterpret_cast<char *>(&tsChunk), tsChunk.head.bufSize);
    tsTimeline->set_chunksizeinbytes(tsChunk.head.bufSize);
    data = analysis::dvvp::message::EncodeMessage(tsTimeline);
    analysis::dvvp::transport::UploaderMgr::instance()->UploadData(
        "0", static_cast<const void *>(data.c_str()), data.size());

    tsChunk.taskState = TS_TIMELINE_END_TASK_STATE;
    tsChunk.timestamp = 40000;
    tsTimeline->set_chunk(reinterpret_cast<char *>(&tsChunk), tsChunk.head.bufSize - 10);
    tsTimeline->set_chunksizeinbytes(tsChunk.head.bufSize - 10);
    data = analysis::dvvp::message::EncodeMessage(tsTimeline);
    analysis::dvvp::transport::UploaderMgr::instance()->UploadData(
        "0", static_cast<const void *>(data.c_str()), data.size());
    tsTimeline->set_chunk(reinterpret_cast<char *>(&tsChunk) + tsChunk.head.bufSize - 10, 10);
    tsTimeline->set_chunksizeinbytes(10);
    data = analysis::dvvp::message::EncodeMessage(tsTimeline);
    analysis::dvvp::transport::UploaderMgr::instance()->UploadData(
        "0", static_cast<const void *>(data.c_str()), data.size());

    HwtsProfileType01 hwtsChunk;
    hwtsChunk.taskId = 30;
    hwtsChunk.streamId = 100;
    hwtsChunk.cntRes0Type = HWTS_TASK_START_TYPE;
    hwtsChunk.syscnt = 1000000;
    SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> hwts(new analysis::dvvp::proto::FileChunkReq());
    hwts->set_filename("hwts.data");
    hwts->set_chunk(reinterpret_cast<char *>(&hwtsChunk), sizeof(HwtsProfileType01));
    hwts->set_chunksizeinbytes(sizeof(HwtsProfileType01));
    data = analysis::dvvp::message::EncodeMessage(hwts);
    analysis::dvvp::transport::UploaderMgr::instance()->UploadData(
        "0", static_cast<const void *>(data.c_str()), data.size());

    hwtsChunk.cntRes0Type = HWTS_TASK_END_TYPE;
    hwtsChunk.syscnt = 2000000;
    hwts->set_chunk(reinterpret_cast<char *>(&hwtsChunk), sizeof(HwtsProfileType01) - 10);
    hwts->set_chunksizeinbytes(sizeof(HwtsProfileType01) - 10);
    data = analysis::dvvp::message::EncodeMessage(hwts);
    analysis::dvvp::transport::UploaderMgr::instance()->UploadData(
        "0", static_cast<const void *>(data.c_str()), data.size());
    hwts->set_chunk(reinterpret_cast<char *>(&hwtsChunk) + sizeof(HwtsProfileType01) - 10, 10);
    hwts->set_chunksizeinbytes(10);
    data = analysis::dvvp::message::EncodeMessage(hwts);
    analysis::dvvp::transport::UploaderMgr::instance()->UploadData(
        "0", static_cast<const void *>(data.c_str()), data.size());

    // ffts
    FftsCxtLog fftsCxtLog;
    fftsCxtLog.head.logType = FFTS_SUBTASK_THREAD_START_FUNC_TYPE;
    fftsCxtLog.streamId = 100;
    fftsCxtLog.taskId = 200;
    fftsCxtLog.sysCountLow = 1000;
    fftsCxtLog.sysCountHigh = 0;
    fftsCxtLog.cxtId = 300;
    fftsCxtLog.threadId = 400;
    SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> ffts(new analysis::dvvp::proto::FileChunkReq());
    ffts->set_filename("stars_soc.data");
    ffts->set_chunk(reinterpret_cast<char *>(&fftsCxtLog), sizeof(fftsCxtLog));
    ffts->set_chunksizeinbytes(sizeof(fftsCxtLog));
    data = analysis::dvvp::message::EncodeMessage(ffts);
    analysis::dvvp::transport::UploaderMgr::instance()->UploadData("0", static_cast<const void *>(data.c_str()), data.size());
    fftsCxtLog.head.logType = FFTS_SUBTASK_THREAD_END_FUNC_TYPE;
    fftsCxtLog.sysCountLow = 2000;
    fftsCxtLog.sysCountHigh = 0;
    ffts->set_chunk(reinterpret_cast<char *>(&fftsCxtLog), sizeof(fftsCxtLog));
    data = analysis::dvvp::message::EncodeMessage(ffts);
    analysis::dvvp::transport::UploaderMgr::instance()->UploadData("0", static_cast<const void *>(data.c_str()), data.size());

    FftsAcsqLog fftsAcsqLog;
    fftsAcsqLog.head.logType = ACSQ_TASK_START_FUNC_TYPE;
    fftsAcsqLog.streamId = 1;
    fftsAcsqLog.taskId = 2;
    fftsAcsqLog.sysCountLow = 10000;
    ffts->set_chunk(reinterpret_cast<char *>(&fftsAcsqLog), sizeof(fftsAcsqLog));
    data = analysis::dvvp::message::EncodeMessage(ffts);
    analysis::dvvp::transport::UploaderMgr::instance()->UploadData("0", static_cast<const void *>(data.c_str()), data.size());
    fftsAcsqLog.head.logType = ACSQ_TASK_END_FUNC_TYPE;
    fftsAcsqLog.sysCountLow = 20000;
    ffts->set_chunk(reinterpret_cast<char *>(&fftsAcsqLog), sizeof(fftsAcsqLog));
    data = analysis::dvvp::message::EncodeMessage(ffts);
    analysis::dvvp::transport::UploaderMgr::instance()->UploadData("0", static_cast<const void *>(data.c_str()), data.size());

    ReporterData reportData = {0};
    strcpy(reportData.tag, "task_desc_info");
    reportData.deviceId = 0;
    reportData.dataLen = sizeof(geTaskDescChunk1);
    reportData.data = (unsigned char *)(&geTaskDescChunk1);
    using namespace Analysis::Dvvp::ProfilerCommon;
    using namespace Msprof::Engine;
    MsprofCallbackHandler::reporters_.clear();
    EXPECT_EQ(PROFILING_SUCCESS, RegisterReporterCallback());
    EXPECT_EQ(MSPROF_ERROR_NONE, MsprofReporterCallbackImpl(MSPROF_MODULE_FRAMEWORK, MSPROF_REPORTER_INIT, nullptr, 0));
    EXPECT_EQ(MSPROF_ERROR_NONE, MsprofReporterCallbackImpl(MSPROF_MODULE_FRAMEWORK, MSPROF_REPORTER_REPORT, (void *)&reportData, sizeof(reportData)));
    FlushAllModule("0");
    EXPECT_EQ(MSPROF_ERROR_NONE, MsprofReporterCallbackImpl(MSPROF_MODULE_FRAMEWORK, MSPROF_REPORTER_UNINIT, nullptr, 0));

    ProfSubscribeInfo subscribeInfo = {true, 1, &fd};
    ProfAclMgr::instance()->subscribeInfos_.insert(std::make_pair(3, subscribeInfo));
    ProfAclMgr::instance()->CloseSubscribeFd(1, 3);

    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->ProfAclModelUnSubscribe(1));
    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->ProfAclModelUnSubscribe(2));
    EXPECT_EQ(ACL_SUCCESS, aclprofModelUnSubscribe(3));
    EXPECT_EQ(0, ProfAclMgr::instance()->GetDeviceSubscribeCount(1, devId));
    EXPECT_EQ(devId, 0);
    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->ProfAclModelSubscribe(1, 0, &config));
    ProfAclMgr::instance()->FlushAllData("0");
    ProfAclMgr::instance()->CloseSubscribeFd(0, 1);
    EXPECT_EQ(0, aclApiSubscribeCount);
    aclprofDestroySubscribeConfig(profSubconfig);
}

TEST_F(MSPROF_ACL_CORE_UTEST, ge_api_subscribe) {
    GlobalMockObject::verify();

    Analysis::Dvvp::Common::Config::ConfigManager::instance()->Init();

    MOCKER(&MmWrite)
        .stubs()
        .will(invoke(mmWriteStub));
    MOCKER(&MmClose)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    using namespace Msprofiler::Api;

    int fd = 1;
    ProfSubscribeConfig config;
    config.timeInfo = true;
    config.aicoreMetrics = PROF_AICORE_NONE;
    config.fd = static_cast<void *>(&fd);

    EXPECT_EQ(nullptr, aclprofCreateSubscribeConfig(0, (aclprofAicoreMetrics)config.aicoreMetrics, nullptr));
    EXPECT_EQ(nullptr, aclprofCreateSubscribeConfig(0, (aclprofAicoreMetrics)config.aicoreMetrics, nullptr));
    auto profSubconfig = aclprofCreateSubscribeConfig(1, (aclprofAicoreMetrics)config.aicoreMetrics, config.fd);

    EXPECT_NE(nullptr, profSubconfig);

    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, aclgrphProfGraphSubscribe(1, nullptr));

    EXPECT_EQ(ACL_SUCCESS, aclgrphProfGraphSubscribe(1, profSubconfig));

    EXPECT_EQ(ACL_SUCCESS, aclgrphProfGraphUnSubscribe(1));

    aclprofDestroySubscribeConfig(profSubconfig);
}

TEST_F(MSPROF_ACL_CORE_UTEST, prof_ge_core_helper) {
    GlobalMockObject::verify();

    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::PlatformIsHelperHostSide)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(ACL_ERROR_FEATURE_UNSUPPORTED, aclgrphProfGraphSubscribe(1, nullptr));
    EXPECT_EQ(ACL_ERROR_FEATURE_UNSUPPORTED, aclgrphProfGraphUnSubscribe(1));
    EXPECT_EQ((size_t)ACL_ERROR_FEATURE_UNSUPPORTED, aclprofGetGraphId(nullptr, 0, 0));
    EXPECT_EQ(ACL_ERROR_FEATURE_UNSUPPORTED, ge::aclgrphProfInit(nullptr, 0));
    EXPECT_EQ(ACL_ERROR_FEATURE_UNSUPPORTED, ge::aclgrphProfFinalize());
    ProfConfig config;
    config.devNums = 1;
    config.devIdList[0] = 0;
    config.aicoreMetrics = PROF_AICORE_ARITHMETIC_UTILIZATION;
    config.dataTypeConfig = 0x7d7f001f;
    EXPECT_EQ(nullptr, ge::aclgrphProfCreateConfig(config.devIdList, config.devNums, (ge::ProfilingAicoreMetrics)config.aicoreMetrics, nullptr, config.dataTypeConfig));
    EXPECT_EQ(ACL_ERROR_FEATURE_UNSUPPORTED, ge::aclgrphProfDestroyConfig(nullptr));
    EXPECT_EQ(ACL_ERROR_FEATURE_UNSUPPORTED, ge::aclgrphProfStart(nullptr));
    EXPECT_EQ(ACL_ERROR_FEATURE_UNSUPPORTED, ge::aclgrphProfStop(nullptr));
}

TEST_F(MSPROF_ACL_CORE_UTEST, acl_json) {
    GlobalMockObject::verify();
    Analysis::Dvvp::Common::Config::ConfigManager::instance()->Uninit();

    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::Init)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    std::string aclJson("{\"switch\": \"on\"}");
    auto data = (void *)(const_cast<char *>(aclJson.c_str()));

    EXPECT_EQ(MSPROF_ERROR_NONE, Analysis::Dvvp::ProfilerCommon::MsprofCtrlCallbackImpl(MSPROF_CTRL_INIT_ACL_JSON, data, aclJson.size()));

    //Analysis::Dvvp::ProfilerCommon::MsprofSetDeviceCallbackImpl(0, true);

    EXPECT_EQ(MSPROF_ERROR_NONE, Analysis::Dvvp::ProfilerCommon::MsprofCtrlCallbackImpl(MSPROF_CTRL_FINALIZE, nullptr, 0));
}

TEST_F(MSPROF_ACL_CORE_UTEST, mode_protect) {
    GlobalMockObject::verify();

    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::Init)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    using namespace Msprofiler::Api;
    ProfAclMgr::instance()->mode_ = WORK_MODE_OFF;
    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->CallbackInitPrecheck());
    EXPECT_NE(ACL_SUCCESS, ProfAclMgr::instance()->CallbackFinalizePrecheck());
    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->ProfInitPrecheck());
    EXPECT_NE(ACL_SUCCESS, ProfAclMgr::instance()->ProfStartPrecheck());
    EXPECT_NE(ACL_SUCCESS, ProfAclMgr::instance()->ProfStopPrecheck());
    EXPECT_NE(ACL_SUCCESS, ProfAclMgr::instance()->ProfFinalizePrecheck());
    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->ProfSubscribePrecheck());
    EXPECT_NE(ACL_SUCCESS, ProfAclMgr::instance()->ProfUnSubscribePrecheck());

    ProfAclMgr::instance()->mode_ = WORK_MODE_CMD;
    EXPECT_NE(ACL_SUCCESS, ProfAclMgr::instance()->CallbackInitPrecheck());
    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->CallbackFinalizePrecheck());
    EXPECT_NE(ACL_SUCCESS, ProfAclMgr::instance()->ProfInitPrecheck());
    EXPECT_NE(ACL_SUCCESS, ProfAclMgr::instance()->ProfStartPrecheck());
    EXPECT_NE(ACL_SUCCESS, ProfAclMgr::instance()->ProfStopPrecheck());
    EXPECT_NE(ACL_SUCCESS, ProfAclMgr::instance()->ProfFinalizePrecheck());
    EXPECT_NE(ACL_SUCCESS, ProfAclMgr::instance()->ProfSubscribePrecheck());
    EXPECT_NE(ACL_SUCCESS, ProfAclMgr::instance()->ProfUnSubscribePrecheck());

    ProfAclMgr::instance()->mode_ = WORK_MODE_API_CTRL;
    EXPECT_NE(ACL_SUCCESS, ProfAclMgr::instance()->CallbackInitPrecheck());
    EXPECT_NE(ACL_SUCCESS, ProfAclMgr::instance()->CallbackFinalizePrecheck());
    EXPECT_NE(ACL_SUCCESS, ProfAclMgr::instance()->ProfInitPrecheck());
    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->ProfStartPrecheck());
    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->ProfStopPrecheck());
    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->ProfFinalizePrecheck());
    EXPECT_NE(ACL_SUCCESS, ProfAclMgr::instance()->ProfSubscribePrecheck());
    EXPECT_NE(ACL_SUCCESS, ProfAclMgr::instance()->ProfUnSubscribePrecheck());

    ProfAclMgr::instance()->mode_ = WORK_MODE_SUBSCRIBE;
    EXPECT_NE(ACL_SUCCESS, ProfAclMgr::instance()->CallbackInitPrecheck());
    EXPECT_NE(ACL_SUCCESS, ProfAclMgr::instance()->CallbackFinalizePrecheck());
    EXPECT_NE(ACL_SUCCESS, ProfAclMgr::instance()->ProfInitPrecheck());
    EXPECT_NE(ACL_SUCCESS, ProfAclMgr::instance()->ProfStartPrecheck());
    EXPECT_NE(ACL_SUCCESS, ProfAclMgr::instance()->ProfStopPrecheck());
    EXPECT_NE(ACL_SUCCESS, ProfAclMgr::instance()->ProfFinalizePrecheck());
    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->ProfSubscribePrecheck());
    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->ProfUnSubscribePrecheck());
}

TEST_F(MSPROF_ACL_CORE_UTEST, MsprofTxSwitchPrecheck) {
    GlobalMockObject::verify();
    using namespace Msprofiler::Api;
    ProfAclMgr::instance()->mode_ = WORK_MODE_CMD;

    int32_t ret = ProfAclMgr::instance()->MsprofTxSwitchPrecheck();
    EXPECT_EQ(PROFILING_FAILED, ret);
    ProfAclMgr::instance()->mode_ = WORK_MODE_OFF;

    ret = ProfAclMgr::instance()->MsprofFinalizeHandle();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(MSPROF_ACL_CORE_UTEST, MsprofTxHandle) {
    GlobalMockObject::verify();
    using namespace Msprofiler::Api;
    MOCKER_CPP(&ProfAclMgr::MsprofTxSwitchPrecheck)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    MOCKER_CPP(&ProfAclMgr::DoHostHandle)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
        new analysis::dvvp::message::ProfileParams());
    ProfAclMgr::instance()->params_ = params;

    int ret = ProfAclMgr::instance()->MsprofTxHandle();
    EXPECT_EQ(MSPROF_ERROR_NONE, ret);
    ret = ProfAclMgr::instance()->MsprofTxHandle();
    EXPECT_EQ(PROFILING_FAILED, ret);

    ret = ProfAclMgr::instance()->MsprofTxHandle();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(MSPROF_ACL_CORE_UTEST, DoHostHandle) {
    GlobalMockObject::verify();
    using namespace Msprofiler::Api;
    MOCKER_CPP(&ProfAclMgr::MsprofSetDeviceImpl)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    MOCKER_CPP(&Analysis::Dvvp::ProfilerCommon::RegisterMsprofTxReporterCallback)
        .stubs()
        .will(ignoreReturnValue());

    MOCKER_CPP(&Msprof::MsprofTx::MsprofTxManager::Init)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
        new analysis::dvvp::message::ProfileParams());
    params->msproftx = "on";
    ProfAclMgr::instance()->params_ = params;

    int ret = ProfAclMgr::instance()->DoHostHandle();
    EXPECT_EQ(PROFILING_FAILED, ret);
    ret = ProfAclMgr::instance()->DoHostHandle();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(MSPROF_ACL_CORE_UTEST, MsprofCtrlCallbackImpl)
{
    GlobalMockObject::verify();
    using namespace Msprofiler::Api;
    uint32_t type = MSPROF_CTRL_FINALIZE;
    VOID_PTR data = nullptr;
    uint32_t len = 0;
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::MsprofFinalizeHandle)
        .stubs()
        .will(returnValue(0));
    int ret = Analysis::Dvvp::ProfilerCommon::MsprofCtrlCallbackImpl(type, data, len);
    EXPECT_EQ(MSPROF_ERROR_NONE, ret);
    type = MSPROF_CTRL_INIT_DYNA;
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::IsModeOff)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::MsprofInitForDynamic)
        .stubs()
        .will(returnValue((int)MSPROF_ERROR_NONE));
    ret = Analysis::Dvvp::ProfilerCommon::MsprofCtrlCallbackImpl(type, data, len);
    EXPECT_EQ(MSPROF_ERROR_NONE, ret);
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::PlatformIsHelperHostSide)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&Analysis::Dvvp::ProfilerCommon::RegisterReporterCallback)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    ret = Analysis::Dvvp::ProfilerCommon::MsprofCtrlCallbackImpl(type, data, len);
    EXPECT_EQ(MSPROF_ERROR_NONE, ret);
    MOCKER_CPP(&Analysis::Dvvp::ProfilerCommon::RegisterReporterCallback)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&Analysis::Dvvp::ProfilerCommon::CommandHandleProfStart)
        .stubs()
        .will(returnValue(ACL_SUCCESS))
        .then(returnValue(ACL_ERROR_PROFILING_FAILURE));
    ret = Analysis::Dvvp::ProfilerCommon::MsprofCtrlCallbackImpl(type, data, len);
    EXPECT_EQ(MSPROF_ERROR_NONE, ret);
    EXPECT_EQ(MSPROF_ERROR_NONE, ret);
}

TEST_F(MSPROF_ACL_CORE_UTEST, RegisterMsprofTxReporterCallback)
{
    GlobalMockObject::verify();
    RegisterMsprofTxReporterCallback();
}

TEST_F(MSPROF_ACL_CORE_UTEST, OpDescParserNullptr) {
    GlobalMockObject::verify();

    using namespace Analysis::Dvvp::Analyze;
    EXPECT_NE(ACL_SUCCESS, OpDescParser::GetOpNum(nullptr, 0, 0));
    uint32_t modelId = 0;
    OpDescParser::GetModelId(nullptr, 0, 0, &modelId);
    EXPECT_EQ(0, modelId);

    EXPECT_NE(ACL_SUCCESS, OpDescParser::instance()->GetOpTypeLen(nullptr, 0, nullptr, 0));
    EXPECT_NE(ACL_SUCCESS, OpDescParser::instance()->GetOpType(nullptr, 0, nullptr, 0, 0));
    EXPECT_NE(ACL_SUCCESS, OpDescParser::instance()->GetOpNameLen(nullptr, 0, nullptr, 0));
    EXPECT_NE(ACL_SUCCESS, OpDescParser::instance()->GetOpName(nullptr, 0, nullptr, 0, 0));
    EXPECT_EQ(0, OpDescParser::GetOpStart(nullptr, 0, 0));
    EXPECT_EQ(0, OpDescParser::GetOpEnd(nullptr, 0, 0));
    EXPECT_EQ(0, OpDescParser::GetOpDuration(nullptr, 0, 0));
    EXPECT_EQ(0, OpDescParser::GetOpExecutionTime(nullptr, 0, 0));
    EXPECT_EQ(0, OpDescParser::GetOpVectorFops(nullptr, 0, 0));
    EXPECT_EQ(0, OpDescParser::GetOpCubeFops(nullptr, 0, 0));
    EXPECT_NE(ACL_SUCCESS, OpDescParser::CheckData(nullptr, 100));
    EXPECT_NE(ACL_SUCCESS, OpDescParser::GetOpFlag(nullptr, 100, 0));
    EXPECT_EQ(nullptr, OpDescParser::GetOpAttriValue(nullptr, 100, 0, ACL_SUBSCRIBE_ATTRI_THREADID));
}

TEST_F(MSPROF_ACL_CORE_UTEST, RecordOutPut) {
    GlobalMockObject::verify();

    std::string env;
    setenv("PROFILER_SAMPLECONFIG", env.c_str(), 1);

    using namespace Msprofiler::Api;
    std::string data;
    EXPECT_EQ(PROFILING_SUCCESS, ProfAclMgr::instance()->RecordOutPut(data));
    env = "{\"acl\":\"on\",\"aiCtrlCpuProfiling\":\"off\",\"ai_core_metrics\":\"PipeUtilization\", \
                       \"ai_core_profiling\":\"\",\"ai_core_profiling_events\":\"0x8,0xa,0x9,0xb,0xc,0xd,0x55\", \
                       \"ai_core_profiling_metrics\":\"\",\"ai_core_profiling_mode\":\"\",\"ai_core_status\":\"\", \
                       \"ai_ctrl_cpu_profiling_events\":\"\",\"ai_vector_status\":\"\",\"aicore_sampling_interval\":10, \
                       \"aicpuTrace\":\"\",\"aiv_metrics\":\"\",\"aiv_profiling\":\"\",\"aiv_profiling_events\":\"\", \
                       \"aiv_profiling_metrics\":\"\",\"aiv_profiling_mode\":\"\",\"aiv_sampling_interval\":10,\"app\":\"test\", \
                       \"app_dir\":\".\",\"app_env\":\"\",\"app_location\":\"\",\"app_parameters\":\"\", \
                       \"cpu_profiling\":\"off\",\"cpu_sampling_interval\":10,\"ctrl_mode\":3, \
                       \"dataSaveToLocal\":false,\"ddr_interval\":100,\"ddr_master_id\":0,\"ddr_profiling\":\"\",\"ddr_profiling_events\":\"\", \
                       \"devices\":\"\",\"dvpp_profiling\":\"off\",\"dvpp_sampling_interval\":100,\"hardware_mem\":\"off\", \
                       \"hardware_mem_sampling_interval\":100,\"hbmInterval\":100,\"hbmProfiling\":\"\",\"hbm_profiling_events\":\"\", \
                       \"hcclTrace\":\"\",\"hccsInterval\":100,\"hccsProfiling\":\"off\",\"hwts_log\":\"on\",\"hwts_log1\":\"on\", \
                       \"interconnection_profiling\":\"off\",\"interconnection_sampling_interval\":100,\"io_profiling\":\"off\", \
                       \"io_sampling_interval\":100,\"is_cancel\":false,\"jobInfo\":\"\",\"job_id\":\"\",\"l2CacheTaskProfiling\":\"\", \
                       \"l2CacheTaskProfilingEvents\":\"\",\"llc_interval\":100,\"llc_profiling\":\"capacity\", \
                       \"llc_profiling_events\":\"hisi_l3c0_1/dsid0/,hisi_l3c0_1/dsid1/,hisi_l3c0_1/dsid2/,hisi_l3c0_1/dsid3/,\
                       hisi_l3c0_1/dsid4/,hisi_l3c0_1/dsid5/,hisi_l3c0_1/dsid6/,hisi_l3c0_1/dsid7/\",\"modelExecution\":\"off\", \
                       \"modelLoad\":\"\",\"msprof\":\"off\",\"nicInterval\":100,\"nicProfiling\":\"off\",\"pcieInterval\":100, \
                       \"pcieProfiling\":\"\",\"pid_profiling\":\"off\",\"pid_sampling_interval\":100,\"profiling_mode\":\"def_mode\", \
                       \"profiling_options\":\"\",\"profiling_period\":-1,\"result_dir\":\"/tmp/profiler_st_init_env\", \
                       \"roceInterval\":100,\"roceProfiling\":\"off\", \
                       \"runtimeApi\":\"off\",\"runtimeTrace\":\"\",\"stream_enabled\":\"on\",\"sys_profiling\":\"off\", \
                       \"sys_sampling_interval\":100,\"traceId\":\"\",\"tsCpuProfiling\":\"off\",\"ts_cpu_hot_function\":\"\", \
                       \"ts_cpu_profiling_events\":\"\",\"ts_cpu_usage\":\"\",\"ts_fw_training\":\"\",\"ts_task_track\":\"\", \
                       \"ts_timeline\":\"on\",\"ts_track1\":\"\"}";
    setenv("PROFILER_SAMPLECONFIG", env.c_str(), 1);
    EXPECT_EQ(PROFILING_SUCCESS, ProfAclMgr::instance()->RecordOutPut(data));

    MOCKER(&analysis::dvvp::common::utils::Utils::CreateDir)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    MOCKER_CPP(&ProfAclMgr::RecordOutPut)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    ProfAclMgr::instance()->InitApiCtrlUploader("13");
}

TEST_F(MSPROF_ACL_CORE_UTEST, RecordOutPut2) {
    GlobalMockObject::verify();

    std::string env = "{}";
    setenv("PROFILER_SAMPLECONFIG", env.c_str(), 1);
    std::string data = "PROFXXX";
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
        new analysis::dvvp::message::ProfileParams());

    using namespace Msprofiler::Api;

    ProfAclMgr::instance()->params_ = params;
    ProfAclMgr::instance()->params_->msprofBinPid = 2;

    MOCKER_CPP(WriteFile)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, ProfAclMgr::instance()->RecordOutPut(data));
    EXPECT_EQ(PROFILING_SUCCESS, ProfAclMgr::instance()->RecordOutPut(data));
}

TEST_F(MSPROF_ACL_CORE_UTEST, MsprofInitAclJson) {
    GlobalMockObject::verify();
    using namespace Msprofiler::Api;
    std::string aclJson = "";
    MOCKER_CPP(&ProfAclMgr::CallbackInitPrecheck)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    std::string result = "/tmp/MsprofInitAclJson";
    analysis::dvvp::common::utils::Utils::RemoveDir(result);
    analysis::dvvp::common::utils::Utils::CreateDir(result);
    EXPECT_EQ(0, ProfAclMgr::instance()->MsprofInitAclJson((void *)aclJson.c_str(), aclJson.size()));
    EXPECT_EQ(3, ProfAclMgr::instance()->MsprofInitAclJson((void *)aclJson.c_str(), aclJson.size()));
    aclJson = "{\"switch\": \"on\", \"output\": \"output\",\"aic_metrics\": \"ArithmeticUtilization\",\"aicpu\": \"on\",\"l2\": \"on\"}";
    EXPECT_EQ(0, ProfAclMgr::instance()->MsprofInitAclJson((void *)aclJson.c_str(), aclJson.size()));

    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetL2cacheEvents)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS))
        .then(returnValue(PROFILING_FAILED));
    EXPECT_EQ(0, ProfAclMgr::instance()->MsprofInitAclJson((void *)aclJson.c_str(), aclJson.size()));
    EXPECT_EQ(0, ProfAclMgr::instance()->MsprofInitAclJson((void *)aclJson.c_str(), aclJson.size()));

    aclJson = "{\"switch\": \"on\", \"output\": \"output\",\"aic_metrics\": \"ArithmeticUtilization\",\"aicpu\": \"on\",\"l2\": \"xx\"}";
    EXPECT_EQ(3, ProfAclMgr::instance()->MsprofInitAclJson((void *)aclJson.c_str(), aclJson.size()));
    aclJson = "{\"switch\": \"on\", \"output\": \"output\",\"aic_metrics\": \"ArithmeticUtilization\",\"aicpu\": \"on\",\"l2\": \"off\"}";
    EXPECT_EQ(0, ProfAclMgr::instance()->MsprofInitAclJson((void *)aclJson.c_str(), aclJson.size()));

    analysis::dvvp::common::utils::Utils::RemoveDir(result);
}

TEST_F(MSPROF_ACL_CORE_UTEST, MsprofCheckAndGetChar) {
    GlobalMockObject::verify();
    using namespace Msprofiler::Api;

    std::string test = "test";
    std::string test1;
    EXPECT_EQ(test1, ProfAclMgr::instance()->MsprofCheckAndGetChar(const_cast<char *>(test.c_str()), test.size()));
}

TEST_F(MSPROF_ACL_CORE_UTEST, MsprofInitGeOptionsParamAdaper) {
    GlobalMockObject::verify();
    using namespace Msprofiler::Api;

    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
		new analysis::dvvp::message::ProfileParams());
    std::shared_ptr<analysis::dvvp::proto::ProfGeOptionsConfig> message(new analysis::dvvp::proto::ProfGeOptionsConfig);

    message->set_aicpu("on");
    message->set_training_trace("on");
    message->set_task_trace("on");
    std::string jobInfo = "123";

    ProfAclMgr::instance()->MsprofInitGeOptionsParamAdaper(nullptr, jobInfo, message);
    ProfAclMgr::instance()->MsprofInitGeOptionsParamAdaper(params, jobInfo, message);
}

TEST_F(MSPROF_ACL_CORE_UTEST, MsprofGeOptionsResultPathAdapter) {
    GlobalMockObject::verify();

    MOCKER(analysis::dvvp::common::utils::Utils::IsDirAccessible)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));

    using namespace Msprofiler::Api;
    std::string path;
    EXPECT_EQ(PROFILING_FAILED, ProfAclMgr::instance()->MsprofResultPathAdapter("", path));
    EXPECT_EQ(PROFILING_FAILED, ProfAclMgr::instance()->MsprofResultPathAdapter("./", path));
    EXPECT_EQ(PROFILING_SUCCESS, ProfAclMgr::instance()->MsprofResultPathAdapter("./", path));

    MOCKER(analysis::dvvp::common::utils::Utils::CreateDir)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    ProfAclMgr::instance()->MsprofResultPathAdapter("./", path);
}

TEST_F(MSPROF_ACL_CORE_UTEST, MsprofInitGeOptions) {
    GlobalMockObject::verify();
    using namespace Msprofiler::Api;

    EXPECT_EQ(3, ProfAclMgr::instance()->MsprofInitGeOptions(nullptr, 2));
    MOCKER_CPP(&ProfAclMgr::CallbackInitPrecheck)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    struct MsprofGeOptions options;
    std::string result = "/tmp/MsprofInitGeOptions";
    analysis::dvvp::common::utils::Utils::RemoveDir(result);
    analysis::dvvp::common::utils::Utils::CreateDir(result);
    EXPECT_EQ(MSPROF_ERROR_CONFIG_INVALID, ProfAclMgr::instance()->MsprofInitGeOptions((void *)&options, sizeof(options)));
    std::string ge_json = "{\"output\": \"/tmp/MsprofInitGeOptions\",\"aic_metrics\": \"ArithmeticUtilization\",\"aicpu\": \"on\",\"l2\": \"on\"}";
    strcpy(options.jobId, "123");
    strcpy(options.options, ge_json.c_str());
    EXPECT_EQ(MSPROF_ERROR_NONE, ProfAclMgr::instance()->MsprofInitGeOptions((void *)&options, sizeof(options)));
    ge_json = "{\"output\": \"/tmp/MsprofInitGeOptions\",\"aic_metrics\": \"ArithmeticUtilization\",\"aicpu\": \"on\",\"l2\": \"off\"}";
    strcpy(options.jobId, "123");
    strcpy(options.options, ge_json.c_str());
    EXPECT_EQ(MSPROF_ERROR_NONE, ProfAclMgr::instance()->MsprofInitGeOptions((void *)&options, sizeof(options)));
    ge_json = "{\"output\": \"/tmp/MsprofInitGeOptions\",\"aic_metrics\": \"ArithmeticUtilization\",\"aicpu\": \"on\",\"l2\": \"xx\"}";
    strcpy(options.jobId, "123");
    strcpy(options.options, ge_json.c_str());
    EXPECT_EQ(3, ProfAclMgr::instance()->MsprofInitGeOptions((void *)&options, sizeof(options)));

    analysis::dvvp::common::utils::Utils::RemoveDir(result);
}

TEST_F(MSPROF_ACL_CORE_UTEST, MsprofHostHandle) {
    GlobalMockObject::verify();
    using namespace Msprofiler::Api;
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::PlatformIsHelperHostSide)
        .stubs()
        .will(returnValue(true));
     ProfAclMgr::instance()->MsprofHostHandle();
}

TEST_F(MSPROF_ACL_CORE_UTEST, ReporterData) {
    GlobalMockObject::verify();

    using namespace analysis::dvvp::common::config;

    ReporterData reporterData;
    reporterData.tag[0] = 't';
    reporterData.tag[1] = 'e';
    reporterData.tag[2] = 's';
    reporterData.tag[3] = 't';
    reporterData.tag[4] = 0;
    reporterData.deviceId = 0;
    reporterData.dataLen = 1;
    unsigned char data[100] = {"test"};
    reporterData.data = data;

    Msprof::Engine::ReceiveData recData;
    recData.Init(RING_BUFF_CAPACITY);
    recData.started_ = true;
    int ret = PROFILING_SUCCESS;
    for (int i = 0; i < RING_BUFF_CAPACITY; i++) {
        ret = recData.DoReport(&reporterData);
    }
    EXPECT_EQ(PROFILING_FAILED, ret);
}

TEST_F(MSPROF_ACL_CORE_UTEST, UploaderDumperDumpModelLoadDataTest) {
    GlobalMockObject::verify();
    std::shared_ptr<Msprof::Engine::UploaderDumper> dumper(new Msprof::Engine::UploaderDumper("Framework"));

    MOCKER_CPP(&Msprof::Engine::UploaderDumper::AddToUploader)
        .stubs();

    std::list<SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> > data;
    data.push_back(nullptr);
    dumper->modelLoadDataCached_["0"] = data;
    dumper->DumpModelLoadData("0");
    dumper->modelLoadData_["0"] = data;
    dumper->DumpModelLoadData("0");
}

TEST_F(MSPROF_ACL_CORE_UTEST, AddToUploaderGetUploaderFailed) {
    GlobalMockObject::verify();
    std::shared_ptr<Msprof::Engine::UploaderDumper> dumper(new Msprof::Engine::UploaderDumper("Framework"));

    HwtsProfileType01 hwtsChunk;
    hwtsChunk.taskId = 30;
    hwtsChunk.streamId = 100;
    hwtsChunk.cntRes0Type = HWTS_TASK_START_TYPE;
    hwtsChunk.syscnt = 1000000;
    SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> hwts(new analysis::dvvp::proto::FileChunkReq());
    hwts->set_filename("hwts.data");
    hwts->set_chunk(reinterpret_cast<char *>(&hwtsChunk), sizeof(HwtsProfileType01));
    hwts->set_chunksizeinbytes(sizeof(HwtsProfileType01));

    MOCKER_CPP(&analysis::dvvp::transport::UploaderMgr::GetUploader)
        .stubs()
        .will(returnValue(nullptr));

    dumper->AddToUploader(hwts);
}

TEST_F(MSPROF_ACL_CORE_UTEST, Analyzer_OnNewData) {
    SHARED_PTR_ALIA<PipeTransport> pipeTransport = std::make_shared<PipeTransport>();
    SHARED_PTR_ALIA<Uploader> pipeUploader = std::make_shared<Uploader>(pipeTransport);
    pipeUploader->Init(100000);
    std::shared_ptr<Analyzer> analyzer(new Analyzer(pipeUploader));

    analyzer->inited_ = false;
    analyzer->OnNewData(nullptr, 0);

    analyzer->inited_ = true;
    analyzer->profileMode_ = PROFILE_MODE_INVALID;
    analyzer->flushQueueLen_ = 1;
    analyzer->flushedChannel_ = true;
    analyzer->OnNewData(nullptr, 0);
    EXPECT_EQ(PROFILE_MODE_SINGLE_OP, analyzer->profileMode_);
}

TEST_F(MSPROF_ACL_CORE_UTEST, Analyzer_TsDataPostProc) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Analysis::Dvvp::Analyze::Analyzer::ConstructAndUploadData)
        .stubs();

    std::shared_ptr<Analyzer> analyzer(new Analyzer(nullptr));
    TsProfileKeypoint data;
    data.head.bufSize = sizeof(TsProfileKeypoint);
    data.taskId = 1;
    data.streamId = 1;
    data.timestamp = 1000000;
    data.tagId = TS_KEYPOINT_START_TASK_STATE;
    analyzer->analyzerTs_->ParseTsKeypointData((CONST_CHAR_PTR)(&data), sizeof(data));
    data.timestamp = 2000000;
    data.tagId = TS_KEYPOINT_END_TASK_STATE;
    analyzer->analyzerTs_->ParseTsKeypointData((CONST_CHAR_PTR)(&data), sizeof(data));

    analyzer->profileMode_ = PROFILE_MODE_INVALID;
    analyzer->TsDataPostProc();
    EXPECT_EQ(PROFILE_MODE_STEP_TRACE, analyzer->profileMode_);

    analyzer->profileMode_ = PROFILE_MODE_INVALID;
    analyzer->TsDataPostProc();
    EXPECT_EQ(PROFILE_MODE_STEP_TRACE, analyzer->profileMode_);
}

TEST_F(MSPROF_ACL_CORE_UTEST, Analyzer_HwtsDataPostProc) {
    std::shared_ptr<Analyzer> analyzer(new Analyzer(nullptr));
    HwtsProfileType01 data;
    data.taskId = 0;
    data.streamId = 0;
    data.syscnt = 1000000;
    analyzer->analyzerHwts_->ParseTaskStartEndData((CONST_CHAR_PTR)(&data), sizeof(data), HWTS_TASK_START_TYPE);
    analyzer->analyzerHwts_->ParseTaskStartEndData((CONST_CHAR_PTR)(&data), sizeof(data), HWTS_TASK_END_TYPE);

    analyzer->inited_ = true;
    analyzer->profileMode_ = PROFILE_MODE_INVALID;
    analyzer->flushQueueLen_ = 1;
    analyzer->flushedChannel_ = false;
    analyzer->devIdStr_ = "123";
    analyzer->HwtsDataPostProc();

    EXPECT_EQ(false, analyzer->inited_);
}

// TEST_F(MSPROF_ACL_CORE_UTEST, Analyzer_FftsDataPostProc) {
//     std::shared_ptr<Analyzer> analyzer(new Analyzer(nullptr));
//     FftsCxtLog data;
//     data.taskId = 0;
//     data.streamId = 0;
//     data.sysCountLow = 1000000;
//     analyzer->analyzerFfts_->ParseSubTaskThreadData((Analysis::Dvvp::Analyze::FftsLogHead*)&data,
//                                                     FFTS_SUBTASK_THREAD_START_FUNC_TYPE);
//     analyzer->analyzerFfts_->ParseSubTaskThreadData((Analysis::Dvvp::Analyze::FftsLogHead*)&data,
//                                                     FFTS_SUBTASK_THREAD_END_FUNC_TYPE);

//     analyzer->inited_ = true;
//     analyzer->profileMode_ = PROFILE_MODE_INVALID;
//     analyzer->flushQueueLen_ = 1;
//     analyzer->flushedChannel_ = false;
//     analyzer->devIdStr_ = "123";
//     analyzer->FftsDataPostProc();

//     EXPECT_EQ(false, analyzer->inited_);
// }

TEST_F(MSPROF_ACL_CORE_UTEST, Analyzer_Ffts_PrintStats) {
    std::shared_ptr<Analyzer> analyzer(new Analyzer(nullptr));
    analyzer->analyzerFfts_->PrintStats();
}

TEST_F(MSPROF_ACL_CORE_UTEST, Analyzer_UploadTsOp) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Analysis::Dvvp::Analyze::Analyzer::ConstructAndUploadData)
        .stubs();

    MOCKER_CPP(&Analysis::Dvvp::Analyze::AnalyzerGe::IsOpInfoCompleted)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));

    std::shared_ptr<Analyzer> analyzer(new Analyzer(nullptr));
    TsProfileTimeline data;
    data.taskId = 0;
    data.streamId = 0;
    data.timestamp = 1000000;
    data.taskState = TS_TIMELINE_START_TASK_STATE;
    analyzer->analyzerTs_->ParseTsTimelineData((CONST_CHAR_PTR)(&data), sizeof(data));
    data.taskState = TS_TIMELINE_AICORE_START_TASK_STATE;
    analyzer->analyzerTs_->ParseTsTimelineData((CONST_CHAR_PTR)(&data), sizeof(data));
    data.taskState = TS_TIMELINE_AICORE_END_TASK_STATE;
    analyzer->analyzerTs_->ParseTsTimelineData((CONST_CHAR_PTR)(&data), sizeof(data));
    data.taskState = TS_TIMELINE_END_TASK_STATE;
    analyzer->analyzerTs_->ParseTsTimelineData((CONST_CHAR_PTR)(&data), sizeof(data));

    data.taskId = 1;
    data.taskState = TS_TIMELINE_START_TASK_STATE;
    analyzer->analyzerTs_->ParseTsTimelineData((CONST_CHAR_PTR)(&data), sizeof(data));
    data.taskState = TS_TIMELINE_AICORE_START_TASK_STATE;
    analyzer->analyzerTs_->ParseTsTimelineData((CONST_CHAR_PTR)(&data), sizeof(data));
    data.taskState = TS_TIMELINE_AICORE_END_TASK_STATE;
    analyzer->analyzerTs_->ParseTsTimelineData((CONST_CHAR_PTR)(&data), sizeof(data));
    data.taskState = TS_TIMELINE_END_TASK_STATE;
    analyzer->analyzerTs_->ParseTsTimelineData((CONST_CHAR_PTR)(&data), sizeof(data));

    data.taskId = 2;
    data.taskState = TS_TIMELINE_START_TASK_STATE;
    analyzer->analyzerTs_->ParseTsTimelineData((CONST_CHAR_PTR)(&data), sizeof(data));
    data.taskState = TS_TIMELINE_AICORE_START_TASK_STATE;
    analyzer->analyzerTs_->ParseTsTimelineData((CONST_CHAR_PTR)(&data), sizeof(data));
    data.taskState = TS_TIMELINE_AICORE_END_TASK_STATE;
    analyzer->analyzerTs_->ParseTsTimelineData((CONST_CHAR_PTR)(&data), sizeof(data));
    data.taskState = TS_TIMELINE_END_TASK_STATE;
    analyzer->analyzerTs_->ParseTsTimelineData((CONST_CHAR_PTR)(&data), sizeof(data));

    auto &tsOpTimes = analyzer->analyzerTs_->opTimes_;
    int ind = 0;
    for (auto iter = tsOpTimes.begin(); iter != tsOpTimes.end(); iter++) {
        iter->second.indexId = ind++;
    }

    analyzer->profileMode_ = PROFILE_MODE_STEP_TRACE;
    analyzer->UploadAppOp(tsOpTimes);
    EXPECT_EQ(1, analyzer->analyzerTs_->opTimes_.size());

    analyzer->profileMode_ = PROFILE_MODE_SINGLE_OP;
    analyzer->UploadAppOp(tsOpTimes);
    EXPECT_EQ(0, analyzer->analyzerTs_->opTimes_.size());
}

TEST_F(MSPROF_ACL_CORE_UTEST, Analyzer_UploadHwtsOp) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Analysis::Dvvp::Analyze::Analyzer::ConstructAndUploadData)
        .stubs();

    MOCKER_CPP(&Analysis::Dvvp::Analyze::AnalyzerGe::IsOpInfoCompleted)
        .stubs()
        .will(returnValue(true))
        .then(returnValue(false))
        .then(returnValue(true));

    std::shared_ptr<Analyzer> analyzer(new Analyzer(nullptr));
    HwtsProfileType01 data;
    data.taskId = 0;
    data.streamId = 0;
    data.syscnt = 1000000;
    analyzer->analyzerHwts_->ParseTaskStartEndData((CONST_CHAR_PTR)(&data), sizeof(data), HWTS_TASK_START_TYPE);
    analyzer->analyzerHwts_->ParseTaskStartEndData((CONST_CHAR_PTR)(&data), sizeof(data), HWTS_TASK_END_TYPE);

    data.taskId = 1;
    analyzer->analyzerHwts_->ParseTaskStartEndData((CONST_CHAR_PTR)(&data), sizeof(data), HWTS_TASK_START_TYPE);
    analyzer->analyzerHwts_->ParseTaskStartEndData((CONST_CHAR_PTR)(&data), sizeof(data), HWTS_TASK_END_TYPE);

    data.taskId = 2;
    analyzer->analyzerHwts_->ParseTaskStartEndData((CONST_CHAR_PTR)(&data), sizeof(data), HWTS_TASK_START_TYPE);
    analyzer->analyzerHwts_->ParseTaskStartEndData((CONST_CHAR_PTR)(&data), sizeof(data), HWTS_TASK_END_TYPE);

    auto &hwtsOpTimes = analyzer->analyzerHwts_->opTimes_;
    for (auto iter = hwtsOpTimes.begin(); iter != hwtsOpTimes.end(); iter++) {
        iter->second.indexId = 1;
        break;
    }

    analyzer->profileMode_ = PROFILE_MODE_STEP_TRACE;
    analyzer->UploadAppOp(hwtsOpTimes);
    EXPECT_EQ(2, analyzer->analyzerHwts_->opTimes_.size());
}

TEST_F(MSPROF_ACL_CORE_UTEST, Analyzer_GetOpIndexId) {
    std::shared_ptr<Analyzer> analyzer(new Analyzer(nullptr));
    TsProfileKeypoint data;
    data.head.bufSize = sizeof(TsProfileKeypoint);
    data.indexId = 1234;
    data.taskId = 1;
    data.streamId = 1;
    data.timestamp = 1000000;
    data.tagId = TS_KEYPOINT_START_TASK_STATE;
    analyzer->analyzerTs_->ParseTsKeypointData((CONST_CHAR_PTR)(&data), sizeof(data));
    data.timestamp = 2000000;
    data.tagId = TS_KEYPOINT_END_TASK_STATE;
    analyzer->analyzerTs_->ParseTsKeypointData((CONST_CHAR_PTR)(&data), sizeof(data));
    data.taskId = 2;
    data.streamId = 1;
    data.timestamp = 3000000;
    data.tagId = TS_KEYPOINT_START_TASK_STATE;
    analyzer->analyzerTs_->ParseTsKeypointData((CONST_CHAR_PTR)(&data), sizeof(data));

    uint64_t ret;
    ret = analyzer->GetOpIndexId(1000000 / analyzer->analyzerTs_->frequency_  - 1);
    EXPECT_EQ(0, ret);

    ret = analyzer->GetOpIndexId(2000000 / analyzer->analyzerTs_->frequency_  + 1);
    EXPECT_EQ(0, ret);

    ret = analyzer->GetOpIndexId(1000000 / analyzer->analyzerTs_->frequency_  + 1);
    EXPECT_EQ(1234, ret);
}

TEST_F(MSPROF_ACL_CORE_UTEST, Analyzer_UpdateTsOpIndexId) {
    std::shared_ptr<Analyzer> analyzer(new Analyzer(nullptr));
    TsProfileKeypoint data;
    data.head.bufSize = sizeof(TsProfileKeypoint);
    data.taskId = 1;
    data.streamId = 1;
    data.timestamp = 1000000;
    data.tagId = TS_KEYPOINT_START_TASK_STATE;
    analyzer->analyzerTs_->ParseTsKeypointData((CONST_CHAR_PTR)(&data), sizeof(data));
    data.timestamp = 2000000;
    data.tagId = TS_KEYPOINT_END_TASK_STATE;
    analyzer->analyzerTs_->ParseTsKeypointData((CONST_CHAR_PTR)(&data), sizeof(data));

    auto &tsOpTimes = analyzer->analyzerTs_->opTimes_;
    for (auto iter = tsOpTimes.begin(); iter != tsOpTimes.end(); iter++) {
        iter->second.indexId = 1;
    }
    analyzer->UpdateOpIndexId(tsOpTimes);
}

TEST_F(MSPROF_ACL_CORE_UTEST, Analyzer_DispatchData) {
    std::shared_ptr<Analyzer> analyzer(new Analyzer(nullptr));

    HwtsProfileType01 hwtsChunk;
    hwtsChunk.taskId = 30;
    hwtsChunk.streamId = 100;
    hwtsChunk.cntRes0Type = HWTS_TASK_START_TYPE;
    hwtsChunk.syscnt = 1000000;
    SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> hwts(new analysis::dvvp::proto::FileChunkReq());
    hwts->set_filename("hwts.data");
    hwts->set_chunk(reinterpret_cast<char *>(&hwtsChunk), sizeof(HwtsProfileType01));
    hwts->set_chunksizeinbytes(sizeof(HwtsProfileType01));
    auto data = analysis::dvvp::message::EncodeMessage(hwts);
    auto decoded = analysis::dvvp::message::DecodeMessage2(data.c_str(), data.size());
    auto message = std::dynamic_pointer_cast<analysis::dvvp::proto::FileChunkReq>(decoded);
    analyzer->profileMode_ = PROFILE_MODE_INVALID;
    analyzer->flushedChannel_ = true;
    analyzer->flushQueueLen_  = 1;
    analyzer->DispatchData(message);
    EXPECT_EQ(PROFILE_MODE_SINGLE_OP, analyzer->profileMode_);
}

TEST_F(MSPROF_ACL_CORE_UTEST, AnalyzerTs_UploadKeypointOp) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Analysis::Dvvp::Analyze::Analyzer::ConstructAndUploadData)
        .stubs();

    auto analyzer = std::make_shared<Analysis::Dvvp::Analyze::Analyzer>();

    TsProfileKeypoint data;
    data.head.bufSize = sizeof(TsProfileKeypoint);
    data.taskId = 1;
    data.streamId = 1;
    data.timestamp = 1000000;
    data.tagId = TS_KEYPOINT_START_TASK_STATE;
    analyzer->analyzerTs_->ParseTsKeypointData((CONST_CHAR_PTR)(&data), sizeof(data));
    data.timestamp = 2000000;
    data.tagId = TS_KEYPOINT_END_TASK_STATE;
    analyzer->analyzerTs_->ParseTsKeypointData((CONST_CHAR_PTR)(&data), sizeof(data));

    data.timestamp = 3000000;
    data.tagId = TS_KEYPOINT_START_TASK_STATE;
    analyzer->analyzerTs_->ParseTsKeypointData((CONST_CHAR_PTR)(&data), sizeof(data));
    data.timestamp = 4000000;
    data.tagId = TS_KEYPOINT_END_TASK_STATE;
    analyzer->analyzerTs_->ParseTsKeypointData((CONST_CHAR_PTR)(&data), sizeof(data));

    data.timestamp = 5000000;
    data.tagId = TS_KEYPOINT_START_TASK_STATE;
    analyzer->analyzerTs_->ParseTsKeypointData((CONST_CHAR_PTR)(&data), sizeof(data));

    auto &keypointOpInfo = analyzer->analyzerTs_->keypointOpInfo_;
    keypointOpInfo[0].uploaded = true;
    keypointOpInfo[1].uploaded = false;
    keypointOpInfo[2].uploaded = false;

    analyzer->UploadKeypointOp();
    EXPECT_EQ(true, keypointOpInfo[1].uploaded);
}

TEST_F(MSPROF_ACL_CORE_UTEST, Graph_aclgrphProfGraphSubscribe) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Analysis::Dvvp::Analyze::Analyzer::ConstructAndUploadData)
        .stubs();

    std::shared_ptr<Analyzer> analyzer(new Analyzer(nullptr));
    TsProfileKeypoint data;
    data.head.bufSize = sizeof(TsProfileKeypoint);
    data.taskId = 1;
    data.streamId = 1;
    data.timestamp = 1000000;
    data.tagId = TS_KEYPOINT_START_TASK_STATE;
    analyzer->analyzerTs_->ParseTsKeypointData((CONST_CHAR_PTR)(&data), sizeof(data));
    data.timestamp = 2000000;
    data.tagId = TS_KEYPOINT_END_TASK_STATE;
    analyzer->analyzerTs_->ParseTsKeypointData((CONST_CHAR_PTR)(&data), sizeof(data));

    data.timestamp = 3000000;
    data.tagId = TS_KEYPOINT_START_TASK_STATE;
    analyzer->analyzerTs_->ParseTsKeypointData((CONST_CHAR_PTR)(&data), sizeof(data));
    data.timestamp = 4000000;
    data.tagId = TS_KEYPOINT_END_TASK_STATE;
    analyzer->analyzerTs_->ParseTsKeypointData((CONST_CHAR_PTR)(&data), sizeof(data));

    data.timestamp = 5000000;
    data.tagId = TS_KEYPOINT_START_TASK_STATE;
    analyzer->analyzerTs_->ParseTsKeypointData((CONST_CHAR_PTR)(&data), sizeof(data));

    auto &keypointOpInfo = analyzer->analyzerTs_->keypointOpInfo_;
    keypointOpInfo[0].uploaded = true;
    keypointOpInfo[1].uploaded = false;
    keypointOpInfo[2].uploaded = false;

    analyzer->UploadKeypointOp();
    EXPECT_EQ(true, keypointOpInfo[1].uploaded);
}

TEST_F(MSPROF_ACL_CORE_UTEST, AicoreMetricsEnumToName) {
    std::string metrics;

    Msprofiler::Api::ProfAclMgr::instance()->AicoreMetricsEnumToName(PROF_AICORE_ARITHMETIC_UTILIZATION, metrics);
    EXPECT_EQ("ArithmeticUtilization", metrics);

    Msprofiler::Api::ProfAclMgr::instance()->AicoreMetricsEnumToName(PROF_AICORE_PIPE_UTILIZATION, metrics);
    EXPECT_EQ("PipeUtilization", metrics);

    Msprofiler::Api::ProfAclMgr::instance()->AicoreMetricsEnumToName(PROF_AICORE_MEMORY_BANDWIDTH, metrics);
    EXPECT_EQ("Memory", metrics);

    Msprofiler::Api::ProfAclMgr::instance()->AicoreMetricsEnumToName(PROF_AICORE_L0B_AND_WIDTH, metrics);
    EXPECT_EQ("MemoryL0", metrics);

    Msprofiler::Api::ProfAclMgr::instance()->AicoreMetricsEnumToName(PROF_AICORE_RESOURCE_CONFLICT_RATIO, metrics);
    EXPECT_EQ("ResourceConflictRatio", metrics);

    Msprofiler::Api::ProfAclMgr::instance()->AicoreMetricsEnumToName(PROF_AICORE_MEMORY_UB, metrics);
    EXPECT_EQ("MemoryUB", metrics);

    Msprofiler::Api::ProfAclMgr::instance()->AicoreMetricsEnumToName(PROF_AICORE_NONE, metrics);
    EXPECT_EQ("MemoryUB", metrics);

    Msprofiler::Api::ProfAclMgr::instance()->AicoreMetricsEnumToName(PROF_AICORE_METRICS_COUNT, metrics);
    EXPECT_EQ("MemoryUB", metrics);
}

TEST_F(MSPROF_ACL_CORE_UTEST, ProfStartCfgToMsprofCfg) {
    std::string metrics;
    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetPlatformType)
        .stubs()
        .will(returnValue(Analysis::Dvvp::Common::Config::PlatformType::DC_TYPE));

    SHARED_PTR_ALIA<analysis::dvvp::proto::MsProfStartReq> feature = nullptr;
    feature = std::make_shared<analysis::dvvp::proto::MsProfStartReq>();

    Msprofiler::Api::ProfAclMgr::instance()->ProfStartCfgToMsprofCfg(
        PROF_TASK_TIME_MASK, PROF_AICORE_ARITHMETIC_UTILIZATION, feature);
    EXPECT_EQ(2, feature->hwts_log().size());
}

TEST_F(MSPROF_ACL_CORE_UTEST, acl_api_subscribe_fail) {
    GlobalMockObject::verify();

    Analysis::Dvvp::Common::Config::ConfigManager::instance()->Init();

    MOCKER(&MmWrite)
        .stubs()
        .will(invoke(mmWriteStub));
    MOCKER(&MmClose)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    MOCKER(Analysis::Dvvp::ProfilerCommon::RegisterReporterCallback)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS + 1))
        .then(returnValue(PROFILING_SUCCESS));

    using namespace Msprofiler::Api;

    int fd = 1;
    ProfSubscribeConfig config;
    config.timeInfo = true;
    config.aicoreMetrics = PROF_AICORE_NONE;
    config.fd = static_cast<void *>(&fd);

    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->Init());
    ProfAclMgr::instance()->devTasks_.clear();
    EXPECT_EQ(ACL_ERROR_PROFILING_FAILURE, ProfAclMgr::instance()->ProfAclModelSubscribe(1, 0, &config));
}

TEST_F(MSPROF_ACL_CORE_UTEST, ProfAclStop) {
    GlobalMockObject::verify();

    Analysis::Dvvp::Common::Config::ConfigManager::instance()->Init();

    MOCKER_CPP(&analysis::dvvp::host::ProfManager::IdeCloudProfileProcess)
        .stubs()
        .will(returnValue(PROFILING_FAILED));

    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::CheckDeviceTask)
        .stubs()
        .will(returnValue(ACL_SUCCESS));

    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::ProfStartAiCpuTrace)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::ProfStartCfgToMsprofCfg)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::GenerateSystemTraceConf)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    MOCKER_CPP(&Analysis::Dvvp::Host::Adapter::ProfParamsAdapter::UpdateSampleConfig)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::StartDeviceTask)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::WaitAllDeviceResponse)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));


    using namespace Msprofiler::Api;


    ProfConfig config;
    config.devNums = 1;
    config.devIdList[0] = 0;
    config.aicoreMetrics = PROF_AICORE_ARITHMETIC_UTILIZATION;
    config.dataTypeConfig = 0x7d7f001f;

    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(new analysis::dvvp::message::ProfileParams());
    ProfAclMgr::ProfAclTaskInfo taskInfo = {1, 0, params};
    ProfAclMgr::instance()->devTasks_[0] = taskInfo;

    ProfAclMgr::instance()->mode_ = WORK_MODE_API_CTRL;
    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->Init());
    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->ProfAclStart(&config));

    EXPECT_EQ(ACL_ERROR_PROFILING_FAILURE, ProfAclMgr::instance()->ProfAclStop(&config));
    ProfAclMgr::instance()->devTasks_.clear();
}

TEST_F(MSPROF_ACL_CORE_UTEST, AnalyzerHwts_CheckData) {
    AnalyzerHwts hwts;

    struct OpTime ot1 = {0,123,0,0,345};
    hwts.CheckData(ot1, "key1", HWTS_TASK_START_TYPE, 12345);
    EXPECT_EQ(1, hwts.opRepeatCount_);
    struct OpTime ot2 = {0,0,0,0,345};
    hwts.CheckData(ot1, "key2", HWTS_TASK_END_TYPE, 12345);
    EXPECT_EQ(2, hwts.opRepeatCount_);
}

TEST_F(MSPROF_ACL_CORE_UTEST, FinalizeHandle_CheckFalseMode) {
    using namespace Msprofiler::Api;
    ProfAclMgr::instance()->mode_ = WORK_MODE_OFF;

    int32_t ret = Msprofiler::Api::ProfAclMgr::instance()->MsprofFinalizeHandle();
    EXPECT_EQ(MSPROF_ERROR_NONE, ret);
}

TEST_F(MSPROF_ACL_CORE_UTEST, FinalizeHandle_CheckTrueMode) {
    using namespace Msprofiler::Api;
    ProfAclMgr::instance()->mode_ = WORK_MODE_CMD;

    int32_t ret = Msprofiler::Api::ProfAclMgr::instance()->MsprofFinalizeHandle();
    EXPECT_EQ(MSPROF_ERROR_NONE, ret);
}

TEST_F(MSPROF_ACL_CORE_UTEST, GEFinalizeHandle_CheckRet) {
    using namespace Msprofiler::Api;

    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::GetRunningDevices)
        .stubs();

    MOCKER_CPP(&Msprof::Engine::FlushAllModule)
        .stubs();
    ge::GeFinalizeHandle();
}

class MSPROF_API_SUBSCRIBE_UTEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {

    }
};

TEST_F(MSPROF_API_SUBSCRIBE_UTEST, AnalyzerGe_Parse) {
    std::shared_ptr<Analysis::Dvvp::Analyze::AnalyzerGe> analyzerGe(new Analysis::Dvvp::Analyze::AnalyzerGe());
    analyzerGe->Parse(nullptr);

    struct MsprofGeProfTaskData geTaskDescChunk;
    std::string geOriData((char *)&geTaskDescChunk, sizeof(geTaskDescChunk));
    SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> geTaskDesc(new analysis::dvvp::proto::FileChunkReq());
    geTaskDesc->set_filename("Framework");
    geTaskDesc->set_tag("task_desc_info_xxx");
    geTaskDesc->set_chunk(geOriData);
    geTaskDesc->set_chunksizeinbytes(sizeof(geTaskDescChunk));
    analyzerGe->Parse(geTaskDesc);

    analyzerGe->totalBytes_ = 0;
    geTaskDesc->set_tag("task_desc_info");
    analyzerGe->Parse(geTaskDesc);
    EXPECT_EQ(sizeof(geTaskDescChunk), analyzerGe->totalBytes_);

    struct MsprofGeProfIdMapData geIdMapData;
    std::string geOriData2((char *)&geIdMapData, sizeof(geIdMapData));
    geTaskDesc->set_tag("id_map_info");
    geTaskDesc->set_chunk(geOriData2);
    geTaskDesc->set_chunksizeinbytes(sizeof(geIdMapData));
    analyzerGe->Parse(geTaskDesc);

    geTaskDesc->set_chunksizeinbytes(0);
    analyzerGe->Parse(geTaskDesc);
}

TEST_F(MSPROF_API_SUBSCRIBE_UTEST, AnalyzerGe_ParseOpName) {
    std::shared_ptr<Analysis::Dvvp::Analyze::AnalyzerGe> analyzerGe(new Analysis::Dvvp::Analyze::AnalyzerGe());
    struct MsprofGeProfTaskData geProfTaskData;
    memset(&geProfTaskData, 0, sizeof(MsprofGeProfTaskData));
    geProfTaskData.opName.type = MSPROF_MIX_DATA_STRING;
    struct Analysis::Dvvp::Analyze::AnalyzerGe::GeOpInfo opInfo;
    analyzerGe->ParseOpName(geProfTaskData, opInfo);
}

TEST_F(MSPROF_API_SUBSCRIBE_UTEST, AnalyzerGe_ParseOpType) {
    std::shared_ptr<Analysis::Dvvp::Analyze::AnalyzerGe> analyzerGe(new Analysis::Dvvp::Analyze::AnalyzerGe());
    struct MsprofGeProfTaskData geProfTaskData;
    memset(&geProfTaskData, 0, sizeof(MsprofGeProfTaskData));
    geProfTaskData.opType.type = MSPROF_MIX_DATA_STRING;
    struct Analysis::Dvvp::Analyze::AnalyzerGe::GeOpInfo opInfo;
    analyzerGe->ParseOpType(geProfTaskData, opInfo);
}

// TEST_F(MSPROF_API_SUBSCRIBE_UTEST, AnalyzerGe_ParseTaskDesc) {
//     std::shared_ptr<Analysis::Dvvp::Analyze::AnalyzerGe> analyzerGe(new Analysis::Dvvp::Analyze::AnalyzerGe());

//     struct MsprofGeProfTaskData geTaskDescChunk1;
//     geTaskDescChunk1.opName.type = 2; // invalid type
//     analyzerGe->ParseTaskDesc(reinter_cast<CONST_CHAR_PTR>(&geTaskDescChunk1), sizeof(geTaskDescChunk1));
    
//     EXPECT_EQ(0, analyzerGe->analyzedBytes_);

//     struct MsprofGeProfTaskData geTaskDescChunk2[10];
//     analyzerGe->ParseTaskDesc(static_cast<CONST_CHAR_PTR>(&geTaskDescChunk2[0]), sizeof(geTaskDescChunk2));
//     EXPECT_EQ(sizeof(geTaskDescChunk2), analyzerGe->analyzedBytes_);
// }


class MSPROF_API_MSPROFTX_UTEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {

    }
};

TEST_F(MSPROF_API_MSPROFTX_UTEST, aclprofCreateStamp) {
    GlobalMockObject::verify();
    Msprof::MsprofTx::MsprofTxManager::instance()->Init();

    void * ret = nullptr;
    ret = aclprofCreateStamp();
    EXPECT_EQ((void *)nullptr, ret);
}

TEST_F(MSPROF_API_MSPROFTX_UTEST, aclprofDestroyStamp) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Msprof::MsprofTx::MsprofTxManager::DestroyStamp)
        .stubs()
        .will(ignoreReturnValue());

    void * ret = nullptr;
    aclprofDestroyStamp(nullptr);
    aclprofDestroyStamp((void *)0x12345678);
    EXPECT_EQ((void *)nullptr, ret);
}

TEST_F(MSPROF_API_MSPROFTX_UTEST, aclprofSetCategoryName) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Msprof::MsprofTx::MsprofTxManager::SetCategoryName)
        .stubs()
        .will(returnValue(ACL_SUCCESS));

    aclError ret = aclprofSetCategoryName(0, "nullptr");
    EXPECT_EQ(ACL_SUCCESS, ret);
}

TEST_F(MSPROF_API_MSPROFTX_UTEST, aclprofSetStampCategory) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Msprof::MsprofTx::MsprofTxManager::SetStampCategory)
        .stubs()
        .will(returnValue(ACL_SUCCESS));

    aclError ret = aclprofSetStampCategory(nullptr, 0);
    EXPECT_EQ(ACL_SUCCESS, ret);
}

TEST_F(MSPROF_API_MSPROFTX_UTEST, aclprofSetStampPayload) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Msprof::MsprofTx::MsprofTxManager::SetStampPayload)
        .stubs()
        .will(returnValue(ACL_SUCCESS));

    aclError ret = aclprofSetStampPayload(nullptr, 0, nullptr);
    EXPECT_EQ(ACL_SUCCESS, ret);
}

TEST_F(MSPROF_API_MSPROFTX_UTEST, aclprofSetStampTraceMessage) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Msprof::MsprofTx::MsprofTxManager::SetStampTraceMessage)
        .stubs()
        .will(returnValue(ACL_SUCCESS));

    aclError ret = aclprofSetStampTraceMessage(nullptr, "nullptr", 0);
    EXPECT_EQ(ACL_SUCCESS, ret);
}

TEST_F(MSPROF_API_MSPROFTX_UTEST, aclprofMark) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Msprof::MsprofTx::MsprofTxManager::Mark)
        .stubs()
        .will(returnValue(ACL_SUCCESS));

    aclError ret = aclprofMark(nullptr);
    EXPECT_EQ(ACL_SUCCESS, ret);
}

TEST_F(MSPROF_API_MSPROFTX_UTEST, aclprofPush) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Msprof::MsprofTx::MsprofTxManager::Push)
        .stubs()
        .will(returnValue(ACL_SUCCESS));

    aclError ret = aclprofPush(nullptr);
    EXPECT_EQ(ACL_SUCCESS, ret);
}

TEST_F(MSPROF_API_MSPROFTX_UTEST, aclprofPop) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Msprof::MsprofTx::MsprofTxManager::Pop)
        .stubs()
        .will(returnValue(ACL_SUCCESS));

    aclError ret = aclprofPop();
    EXPECT_EQ(ACL_SUCCESS, ret);
}

TEST_F(MSPROF_API_MSPROFTX_UTEST, aclprofRangeStart) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Msprof::MsprofTx::MsprofTxManager::RangeStart)
        .stubs()
        .will(returnValue(ACL_SUCCESS));

    aclError ret = aclprofRangeStart(nullptr, nullptr);
    EXPECT_EQ(ACL_SUCCESS, ret);
}

TEST_F(MSPROF_API_MSPROFTX_UTEST, aclprofRangeStop) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Msprof::MsprofTx::MsprofTxManager::RangeStop)
        .stubs()
        .will(returnValue(ACL_SUCCESS));

    aclError ret = aclprofRangeStop(0);
    EXPECT_EQ(ACL_SUCCESS, ret);
}

class COMMANDHANDLE_TEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(COMMANDHANDLE_TEST, commandHandle_api) {
    GlobalMockObject::verify();

    EXPECT_EQ(ACL_SUCCESS, CommandHandleProfInit());
    std::string retStr(4099,'a');
    std::string zeroStr = "{}";
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::GetParamJsonStr)
        .stubs()
        .will(returnValue(retStr))
        .then(returnValue(zeroStr));
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::PlatformIsHelperHostSide)
        .stubs()
        .will(returnValue(false));
    EXPECT_EQ(ACL_ERROR_PROFILING_FAILURE, CommandHandleProfInit());
    uint32_t devList[] = {0, 1};
    uint32_t devNums = 2;
    EXPECT_EQ(ACL_SUCCESS, CommandHandleProfStart(devList, devNums, 0));
    EXPECT_EQ(ACL_SUCCESS, CommandHandleProfStart(nullptr, 0, 0));
    EXPECT_EQ(ACL_SUCCESS, CommandHandleProfStop(devList, devNums, 0));
    EXPECT_EQ(ACL_SUCCESS, CommandHandleProfStart(nullptr, 0, 0));
    EXPECT_EQ(ACL_SUCCESS, CommandHandleProfFinalize());
    EXPECT_EQ(ACL_SUCCESS, CommandHandleProfSubscribe(0, 0));
    EXPECT_EQ(ACL_SUCCESS, CommandHandleProfUnSubscribe(0));
}