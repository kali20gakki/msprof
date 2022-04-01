#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include <string.h>
#include <google/protobuf/util/json_util.h>
#include "config/config_manager.h"
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
#include "uploader_mgr.h"
#include "utils/utils.h"
#include "prof_acl_core.h"
#include "prof_ge_core.h"
#include "uploader_dumper.h"
#include "prof_common.h"
#include "analyzer_ge.h"
#include "platform/platform.h"
#include "command_handle.h"
#include "data_struct.h"
using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::ProfilerCommon;
using namespace Analysis::Dvvp::Common::Platform;
using namespace Analysis::Dvvp::Analyze;
using namespace analysis::dvvp::transport;

const int RECEIVE_CHUNK_SIZE = 320; // chunk size:320

class MSPROF_ACL_CORE_STEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(MSPROF_ACL_CORE_STEST, TestAclCtrlApi) {
    GlobalMockObject::verify();
}


TEST_F(MSPROF_ACL_CORE_STEST, acl_api) {
    GlobalMockObject::verify();

    std::string result = "/tmp/acl_api_stest_new";
    analysis::dvvp::common::utils::Utils::RemoveDir(result);
    analysis::dvvp::common::utils::Utils::CreateDir(result);
    EXPECT_EQ(ge::SUCCESS, ge::aclgrphProfInit(result.c_str(), result.size()));

    ProfConfig config;
    config.devNums = 1;
    config.devIdList[0] = 0;
    config.aicoreMetrics = PROF_AICORE_MEMORY_UB;
    config.dataTypeConfig = 0x7d7f001f;

    ge::aclgrphProfConfig *aclConfig = ge::aclgrphProfCreateConfig(
        config.devIdList, config.devNums, (ge::ProfilingAicoreMetrics)config.aicoreMetrics, nullptr, config.dataTypeConfig);

    EXPECT_NE(nullptr, aclConfig);

    EXPECT_EQ(ge::SUCCESS, ge::aclgrphProfStart(aclConfig));

    Analysis::Dvvp::ProfilerCommon::MsprofReporterCallbackImpl(MSPROF_MODULE_DATA_PREPROCESS, MSPROF_REPORTER_INIT, nullptr, 0);
    MsprofHashData hData = {0};
    std::string hashData = "profiling_st_data";
    hData.dataLen = hashData.size();
    hData.data = const_cast<unsigned char *>((const unsigned char *)hashData.c_str());
    hData.hashId = 0;
    Analysis::Dvvp::ProfilerCommon::MsprofReporterCallbackImpl(MSPROF_MODULE_DATA_PREPROCESS, MSPROF_REPORTER_HASH, (void *)&hData, sizeof(hData));
    EXPECT_NE(0, hData.hashId);
    hData.hashId = 0;
    Analysis::Dvvp::ProfilerCommon::MsprofReporterCallbackImpl(MSPROF_MODULE_DATA_PREPROCESS, MSPROF_REPORTER_HASH, (void *)&hData, 0);
    EXPECT_EQ(0, hData.hashId);
    hData.dataLen = 0;

    ReporterData data = {0};
    data.tag[0] = 't';
    data.deviceId = 0;
    std::string reportData = "profiling_st_data";
    data.dataLen = reportData.size();
    data.data = const_cast<unsigned char *>((const unsigned char *)reportData.c_str());
    Analysis::Dvvp::ProfilerCommon::MsprofReporterCallbackImpl(MSPROF_MODULE_DATA_PREPROCESS, MSPROF_REPORTER_REPORT, (void *)&data, sizeof(data));
    Analysis::Dvvp::ProfilerCommon::MsprofReporterCallbackImpl(MSPROF_MODULE_DATA_PREPROCESS, MSPROF_REPORTER_UNINIT, nullptr, 0);
    Msprof::Engine::MsprofCallbackHandler::reporters_[MSPROF_MODULE_DATA_PREPROCESS].ForceFlush("0");

    EXPECT_EQ(ge::SUCCESS, ge::aclgrphProfStop(aclConfig));

    EXPECT_EQ(ge::SUCCESS, ge::aclgrphProfFinalize());

    EXPECT_EQ(ge::SUCCESS, ge::aclgrphProfDestroyConfig(aclConfig));

    analysis::dvvp::common::utils::Utils::RemoveDir(result);
}

TEST_F(MSPROF_ACL_CORE_STEST, aclgrphProfInit_failed) {
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
}

TEST_F(MSPROF_ACL_CORE_STEST, AddToUploaderGetUploaderFailed) {
    GlobalMockObject::verify();
    std::shared_ptr<Msprof::Engine::UploaderDumper> dumper(new Msprof::Engine::UploaderDumper("Framework"));

    SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> hwts(new analysis::dvvp::proto::FileChunkReq());
    hwts->set_filename("hwts.data");

    MOCKER_CPP(&analysis::dvvp::transport::UploaderMgr::GetUploader)
        .stubs()
        .will(returnValue(nullptr));

    dumper->AddToUploader(hwts);
}

TEST_F(MSPROF_ACL_CORE_STEST, acl_prof_api) {
    GlobalMockObject::verify();

    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::Init)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    std::string result = "/tmp/acl_prof_api_stest_new";
    analysis::dvvp::common::utils::Utils::RemoveDir(result);
    analysis::dvvp::common::utils::Utils::CreateDir(result);
    EXPECT_EQ(ACL_SUCCESS, aclprofInit(result.c_str(), result.size()));

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
    using namespace Msprofiler::Api;
    ProfAclMgr::instance()->mode_ = WORK_MODE_OFF;
    analysis::dvvp::common::utils::Utils::RemoveDir(result);
}

TEST_F(MSPROF_ACL_CORE_STEST, prof_acl_api_helper) {
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
    EXPECT_EQ(0, ProfGetOpExecutionTime(nullptr, 0, 0));
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

TEST_F(MSPROF_ACL_CORE_STEST, MsprofInitHelper) {
    GlobalMockObject::verify();

    using namespace Msprofiler::Api;
    EXPECT_EQ(3, ProfAclMgr::instance()->MsprofInitHelper(nullptr, 0));
    MOCKER_CPP(&ProfAclMgr::CallbackInitPrecheck)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    struct MsprofCommandHandleParams commandHandleParams;
    memset(&commandHandleParams, 0, sizeof(MsprofCommandHandleParams));
    std::string result = "/tmp/MsprofInitHelper";
    analysis::dvvp::common::utils::Utils::RemoveDir(result);
    analysis::dvvp::common::utils::Utils::CreateDir(result);
    EXPECT_EQ(0, ProfAclMgr::instance()->MsprofInitHelper((void *)&commandHandleParams, sizeof(MsprofCommandHandleParams)));
    commandHandleParams.pathLen = result.size();
    strncpy(commandHandleParams.path, result.c_str(), 1024);
    commandHandleParams.storageLimit = 250;
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(new analysis::dvvp::message::ProfileParams());
    params->FromString("{\"result_dir\":\"/tmp/\", \"devices\":\"1\", \"job_id\":\"1\"}");
    std::string jsonParams = params->ToString();
    commandHandleParams.profDataLen = jsonParams.size();
    strncpy(commandHandleParams.profData, jsonParams.c_str(), 4096);
    EXPECT_EQ(0, ProfAclMgr::instance()->MsprofInitHelper((void *)&commandHandleParams, sizeof(MsprofCommandHandleParams)));

    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::PlatformIsHelperHostSide)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(MSPROF_ERROR_NONE, MsprofInit(MSPROF_CTRL_INIT_HELPER, (void *)&commandHandleParams, sizeof(MsprofCommandHandleParams)));
    analysis::dvvp::common::utils::Utils::RemoveDir(result);
}

TEST_F(MSPROF_ACL_CORE_STEST, MsprofHostHandle) {
    GlobalMockObject::verify();
    using namespace Msprofiler::Api;
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::PlatformIsHelperHostSide)
        .stubs()
        .will(returnValue(true));
     ProfAclMgr::instance()->MsprofHostHandle();
}

TEST_F(MSPROF_ACL_CORE_STEST, prof_ge_core_helper) {
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

TEST_F(MSPROF_ACL_CORE_STEST, acl_prof_init_failed) {
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

TEST_F(MSPROF_ACL_CORE_STEST, MsprofResultPathAdapter) {
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

    analysis::dvvp::common::utils::Utils::IsDirAccessible("/etc/passwd");
}

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
    EXPECT_EQ(true, OpDescParser::GetOpExecutionTime(buf, bufLen, 0) >= 0);
    EXPECT_EQ(true, OpDescParser::GetOpCubeFops(buf, bufLen, 0) >= 0);
    EXPECT_EQ(true, OpDescParser::GetOpVectorFops(buf, bufLen, 0) >= 0);
    EXPECT_EQ(111, OpDescParser::GetOpFlag(buf, bufLen, 0));
    EXPECT_NE(nullptr, OpDescParser::GetOpAttriValue(buf, bufLen, 0, ACL_SUBSCRIBE_ATTRI_THREADID));
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
    return bufLen;
}

TEST_F(MSPROF_ACL_CORE_STEST, get_op_xxx) {
    ProfOpDesc data;
    data.modelId = 0;
    data.flag = ACL_SUBSCRIBE_OP_THREAD;
    data.threadId = 0;
    data.modelId = 0;
    EXPECT_EQ(ACL_SUBSCRIBE_OP_THREAD, OpDescParser::GetOpFlag(&data, sizeof(data), 0));
    EXPECT_NE(nullptr, OpDescParser::GetOpAttriValue(&data, sizeof(data), 0, ACL_SUBSCRIBE_ATTRI_THREADID));
    EXPECT_EQ(nullptr, OpDescParser::GetOpAttriValue(&data, sizeof(data), 0, ACL_SUBSCRIBE_ATTRI_NONE));

    EXPECT_EQ(ACL_SUBSCRIBE_NONE, OpDescParser::GetOpFlag(nullptr, sizeof(data), 0));
    EXPECT_EQ(nullptr, OpDescParser::GetOpAttriValue(nullptr, sizeof(data), 0, ACL_SUBSCRIBE_ATTRI_THREADID));
}

TEST_F(MSPROF_ACL_CORE_STEST, acl_api_subscribe) {
    GlobalMockObject::verify();

    Analysis::Dvvp::Common::Config::ConfigManager::instance()->Init();

    MOCKER(mmWrite)
        .stubs()
        .will(invoke(mmWriteStub));
    MOCKER(mmClose)
        .stubs()
        .will(returnValue(EN_OK));

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


    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->Init());
    config.timeInfo = false;
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, ProfAclMgr::instance()->ProfAclModelSubscribe(1, 0, &config));
    config.timeInfo = true;
    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->ProfAclModelSubscribe(1, 0, &config));
    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->ProfAclModelSubscribe(2, 0, &config));

    SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> geIdMap(new analysis::dvvp::proto::FileChunkReq());
    struct MsprofGeProfIdMapData idMapData;
    geIdMap->set_filename("Framework");
    geIdMap->set_tag("id_map_info");
    std::string geOriData0((char *)&idMapData, sizeof(idMapData));
    geIdMap->set_chunk(geOriData0);
    geIdMap->set_chunksizeinbytes(sizeof(idMapData));
    std::string data0 = analysis::dvvp::message::EncodeMessage(geIdMap);
    analysis::dvvp::transport::UploaderMgr::instance()->UploadData(
        "0", static_cast<const void *>(data0.c_str()), data0.size());

    std::string data;

    std::string geTaskDescChunk = "{\"block_dims\": 1,   \"cur_iter_num\": 0,   \"input\": [     {       \"data_type\": \"int32\",       \"format\": \"ND\",       \"idx\": 0,       \"shape\": []     },     {       \"data_type\": \"int32\",       \"format\": \"ND\",       \"idx\": 1,       \"shape\": []     }   ],   \"model_id\": 1,   \"model_name\": \"ge_default_20210220001434_1\",   \"op_name\": \"OpName\",   \"op_type\": \"OpType\",   \"output\": [     {       \"data_type\": \"int32\",       \"format\": \"ND\",       \"idx\": 0,       \"shape\": []     }   ],   \"shape_type\": \"static\",   \"stream_id\": 100,   \"task_id\": 30,   \"task_type\": \"AI_CORE\" },";
    SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> geTaskDesc(new analysis::dvvp::proto::FileChunkReq());
    geTaskDesc->set_filename("Framework");
    geTaskDesc->set_chunk(geTaskDescChunk);
    geTaskDesc->set_chunksizeinbytes(geTaskDescChunk.size());
    geTaskDesc->set_tag("task_desc_info");
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
    tsTimeline->set_chunk(reinterpret_cast<char *>(&tsChunk), tsChunk.head.bufSize);
    tsTimeline->set_chunksizeinbytes(tsChunk.head.bufSize);
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
    hwts->set_chunk(reinterpret_cast<char *>(&hwtsChunk), sizeof(HwtsProfileType01));
    hwts->set_chunksizeinbytes(sizeof(HwtsProfileType01));
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

    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->ProfAclModelUnSubscribe(1));
    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->ProfAclModelUnSubscribe(2));
    EXPECT_EQ(0, aclApiSubscribeCount);
    aclprofDestroySubscribeConfig(profSubconfig);
}

TEST_F(MSPROF_ACL_CORE_STEST, acl_api_subscribe_step_parse) {
    GlobalMockObject::verify();

    Analysis::Dvvp::Common::Config::ConfigManager::instance()->Init();

    MOCKER(mmWrite)
        .stubs()
        .will(invoke(mmWriteStub));
    MOCKER(mmClose)
        .stubs()
        .will(returnValue(EN_OK));

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


    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->Init());
    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->ProfAclModelSubscribe(1, 0, &config));
    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->ProfAclModelSubscribe(2, 0, &config));

    std::string data;

    std::string geTaskDescChunk = "{\"block_dims\": 1,   \"cur_iter_num\": 0,   \"input\": [     {       \"data_type\": \"int32\",       \"format\": \"ND\",       \"idx\": 0,       \"shape\": []     },     {       \"data_type\": \"int32\",       \"format\": \"ND\",       \"idx\": 1,       \"shape\": []     }   ],   \"model_id\": 1,   \"model_name\": \"ge_default_20210220001434_1\",   \"op_name\": \"OpName\",   \"op_type\": \"OpType\",   \"output\": [     {       \"data_type\": \"int32\",       \"format\": \"ND\",       \"idx\": 0,       \"shape\": []     }   ],   \"shape_type\": \"static\",   \"stream_id\": 100,   \"task_id\": 30,   \"task_type\": \"AI_CORE\" },";
    SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> geTaskDesc(new analysis::dvvp::proto::FileChunkReq());
    geTaskDesc->set_filename("Framework");
    geTaskDesc->set_chunk(geTaskDescChunk);
    geTaskDesc->set_chunksizeinbytes(geTaskDescChunk.size());
    geTaskDesc->set_tag("task_desc_info");
    data = analysis::dvvp::message::EncodeMessage(geTaskDesc);
    analysis::dvvp::transport::UploaderMgr::instance()->UploadData(
        "0", static_cast<const void *>(data.c_str()), data.size());

    using namespace Analysis::Dvvp::Analyze;
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

    using namespace Analysis::Dvvp::Analyze;
    TsProfileKeypoint tsKpChunk;
    tsKpChunk.head.rptType = TS_KEYPOINT_RPT_TYPE;
    tsKpChunk.head.bufSize = sizeof(TsProfileKeypoint);
    tsKpChunk.taskId = 30;
    tsKpChunk.streamId = 100;
    tsKpChunk.tagId = TS_KEYPOINT_START_TASK_STATE;
    tsKpChunk.timestamp = 1000000;
    SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> tsKpTimeline(new analysis::dvvp::proto::FileChunkReq());
    tsKpTimeline->set_filename("ts_track.data");
    tsKpTimeline->set_chunk(reinterpret_cast<char *>(&tsKpChunk), tsKpChunk.head.bufSize);
    tsKpTimeline->set_chunksizeinbytes(tsKpChunk.head.bufSize);
    data = analysis::dvvp::message::EncodeMessage(tsKpTimeline);
    analysis::dvvp::transport::UploaderMgr::instance()->UploadData(
        "0", static_cast<const void *>(data.c_str()), data.size());

    using namespace Analysis::Dvvp::Analyze;
    TsProfileTimeline tsChunk;
    tsChunk.head.rptType = TS_TIMELINE_RPT_TYPE;
    tsChunk.head.bufSize = sizeof(TsProfileTimeline);
    tsChunk.taskId = 30;
    tsChunk.streamId = 100;
    tsChunk.taskState = TS_TIMELINE_START_TASK_STATE;
    tsChunk.timestamp = 1000001;
    SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> tsTimeline(new analysis::dvvp::proto::FileChunkReq());
    tsTimeline->set_filename("ts_track.data");
    tsTimeline->set_chunk(reinterpret_cast<char *>(&tsChunk), tsChunk.head.bufSize);
    tsTimeline->set_chunksizeinbytes(tsChunk.head.bufSize);
    data = analysis::dvvp::message::EncodeMessage(tsTimeline);
    analysis::dvvp::transport::UploaderMgr::instance()->UploadData(
        "0", static_cast<const void *>(data.c_str()), data.size());

    tsChunk.taskState = TS_TIMELINE_AICORE_START_TASK_STATE;
    tsChunk.timestamp = 1000002;
    tsTimeline->set_chunk(reinterpret_cast<char *>(&tsChunk), tsChunk.head.bufSize);
    tsTimeline->set_chunksizeinbytes(tsChunk.head.bufSize);
    data = analysis::dvvp::message::EncodeMessage(tsTimeline);
    analysis::dvvp::transport::UploaderMgr::instance()->UploadData(
        "0", static_cast<const void *>(data.c_str()), data.size());

    tsChunk.taskState = TS_TIMELINE_AICORE_END_TASK_STATE;
    tsChunk.timestamp = 1000003;
    tsTimeline->set_chunk(reinterpret_cast<char *>(&tsChunk), tsChunk.head.bufSize);
    tsTimeline->set_chunksizeinbytes(tsChunk.head.bufSize);
    data = analysis::dvvp::message::EncodeMessage(tsTimeline);
    analysis::dvvp::transport::UploaderMgr::instance()->UploadData(
        "0", static_cast<const void *>(data.c_str()), data.size());

    tsChunk.taskState = TS_TIMELINE_END_TASK_STATE;
    tsChunk.timestamp = 1000004;
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

    tsKpChunk.tagId = TS_KEYPOINT_END_TASK_STATE;
    tsKpChunk.timestamp = 5000000;
    tsKpTimeline->set_chunk(reinterpret_cast<char *>(&tsKpChunk), tsKpChunk.head.bufSize);
    tsKpTimeline->set_chunksizeinbytes(tsKpChunk.head.bufSize);
    data = analysis::dvvp::message::EncodeMessage(tsKpTimeline);
    analysis::dvvp::transport::UploaderMgr::instance()->UploadData(
        "0", static_cast<const void *>(data.c_str()), data.size());

    tsKpTimeline->set_chunk(reinterpret_cast<char *>(&tsKpChunk) + tsKpChunk.head.bufSize - 10, 10);
    tsKpTimeline->set_chunksizeinbytes(10);
    data = analysis::dvvp::message::EncodeMessage(tsKpTimeline);
    analysis::dvvp::transport::UploaderMgr::instance()->UploadData(
        "0", static_cast<const void *>(data.c_str()), data.size());

    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->ProfAclModelUnSubscribe(1));
    EXPECT_EQ(ACL_SUCCESS, ProfAclMgr::instance()->ProfAclModelUnSubscribe(2));
    aclprofDestroySubscribeConfig(profSubconfig);
}


TEST_F(MSPROF_ACL_CORE_STEST, ge_api_subscribe) {
    GlobalMockObject::verify();

    Analysis::Dvvp::Common::Config::ConfigManager::instance()->Init();

    MOCKER(mmWrite)
        .stubs()
        .will(invoke(mmWriteStub));
    MOCKER(mmClose)
        .stubs()
        .will(returnValue(EN_OK));

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

    EXPECT_EQ(ge::SUCCESS, aclgrphProfGraphSubscribe(1, profSubconfig));

    EXPECT_EQ(ge::SUCCESS, aclgrphProfGraphUnSubscribe(1));

    aclprofDestroySubscribeConfig(profSubconfig);
}

TEST_F(MSPROF_ACL_CORE_STEST, acl_json) {
    GlobalMockObject::verify();

    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::Init)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER(analysis::dvvp::common::utils::Utils::GetFileSize)
        .stubs()
        .will(returnValue((long long)100000000)); // 100MB
    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetPlatformType)
        .stubs()
        .will(returnValue(Analysis::Dvvp::Common::Config::PlatformType::DC_TYPE));

    std::string aclJson("{\"switch\": \"on\", \"storage_limit\": \"500MB\"}");
    auto data = (void *)(const_cast<char *>(aclJson.c_str()));
    EXPECT_EQ(MSPROF_ERROR_NONE, Analysis::Dvvp::ProfilerCommon::MsprofCtrlCallbackImpl(MSPROF_CTRL_INIT_ACL_JSON, data, aclJson.size()));

    aclJson = "{\"switch\": \"on\", \"l2\": \"on\"}";
    data = (void *)(const_cast<char *>(aclJson.c_str()));

    EXPECT_EQ(MSPROF_ERROR_NONE, Analysis::Dvvp::ProfilerCommon::MsprofCtrlCallbackImpl(MSPROF_CTRL_INIT_ACL_JSON, data, aclJson.size()));

    Analysis::Dvvp::ProfilerCommon::MsprofSetDeviceCallbackImpl(0, true);

    EXPECT_EQ(MSPROF_ERROR_NONE, Analysis::Dvvp::ProfilerCommon::MsprofCtrlCallbackImpl(MSPROF_CTRL_FINALIZE, nullptr, 0));
}

TEST_F(MSPROF_ACL_CORE_STEST, ge_option) {
    GlobalMockObject::verify();

    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::Init)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    std::string result = "/tmp/profiler_st_ge_option";
    analysis::dvvp::common::utils::Utils::RemoveDir(result);
    analysis::dvvp::common::utils::Utils::CreateDir(result);
    std::shared_ptr<analysis::dvvp::proto::ProfGeOptionsConfig> message(new analysis::dvvp::proto::ProfGeOptionsConfig);
    message->set_output(result);
    message->set_storage_limit("500MB");
    message->set_training_trace("on");
    message->set_task_trace("on");
    message->set_aicpu("on");
    message->set_aic_metrics("ArithmeticUtilization");
    message->set_l2("on");

    std::string json = analysis::dvvp::message::EncodeJson(message, false, false);

    MsprofGeOptions options = {0};
    strcpy(options.jobId, "jobId");
    for (size_t i = 0; i < json.size(); i++) {
        options.options[i] = json.at(i);
    }
    auto data = (void *)&options;

    EXPECT_EQ(MSPROF_ERROR_NONE, Analysis::Dvvp::ProfilerCommon::MsprofCtrlCallbackImpl(MSPROF_CTRL_INIT_GE_OPTIONS, data, sizeof(options)));

    Analysis::Dvvp::ProfilerCommon::MsprofSetDeviceCallbackImpl(0, true);

    EXPECT_EQ(MSPROF_ERROR_NONE, Analysis::Dvvp::ProfilerCommon::MsprofCtrlCallbackImpl(MSPROF_CTRL_FINALIZE, nullptr, 0));
    analysis::dvvp::common::utils::Utils::RemoveDir(result);
}

TEST_F(MSPROF_ACL_CORE_STEST, init_env) {
    GlobalMockObject::verify();

    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::Init)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER(analysis::dvvp::common::utils::Utils::GetFileSize)
        .stubs()
        .will(returnValue((long long)100000000)); // 100MB
    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetPlatformType)
        .stubs()
        .will(returnValue(Analysis::Dvvp::Common::Config::PlatformType::DC_TYPE));

    std::string result = "/tmp/profiler_st_init_env";
    analysis::dvvp::common::utils::Utils::RemoveDir(result);
    analysis::dvvp::common::utils::Utils::CreateDir(result);
    std::string env = "{\"acl\":\"on\",\"aiCtrlCpuProfiling\":\"off\",\"ai_core_metrics\":\"PipeUtilization\", \
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
                       \"ts_timeline\":\"on\",\"ts_track1\":\"\",\"storage_limit\": \"1000MB\"}";
    setenv("PROFILER_SAMPLECONFIG", env.c_str(), 1);

    EXPECT_EQ(MSPROF_ERROR_NONE, Analysis::Dvvp::ProfilerCommon::MsprofCtrlCallbackImpl(MSPROF_CTRL_INIT_ACL_ENV, nullptr, 0));

    Analysis::Dvvp::ProfilerCommon::MsprofSetDeviceCallbackImpl(0, true);

    EXPECT_EQ(MSPROF_ERROR_NONE, Analysis::Dvvp::ProfilerCommon::MsprofCtrlCallbackImpl(MSPROF_CTRL_FINALIZE, nullptr, 0));
    analysis::dvvp::common::utils::Utils::RemoveDir(result);
}

TEST_F(MSPROF_ACL_CORE_STEST, mode_protect) {
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

TEST_F(MSPROF_ACL_CORE_STEST, FinalizeHandle_CheckFalseMode) {
    using namespace Msprofiler::Api;
    ProfAclMgr::instance()->mode_ = WORK_MODE_OFF;

    int32_t ret = ProfAclMgr::instance()->MsprofFinalizeHandle();
    EXPECT_EQ(MSPROF_ERROR_NONE, ret);
}

TEST_F(MSPROF_ACL_CORE_STEST, FinalizeHandle_CheckTrueMode) {
    using namespace Msprofiler::Api;
    ProfAclMgr::instance()->mode_ = WORK_MODE_CMD;

    int32_t ret = ProfAclMgr::instance()->MsprofFinalizeHandle();
    EXPECT_EQ(MSPROF_ERROR_NONE, ret);
}

TEST_F(MSPROF_ACL_CORE_STEST, GEFinalizeHandle_CheckRet) {
    using namespace Msprofiler::Api;
    MOCKER_CPP(&ProfAclMgr::GetRunningDevices)
        .stubs();

    MOCKER_CPP(&Msprof::Engine::FlushAllModule)
        .stubs();
    ge::GeFinalizeHandle();
}

class MSPROF_API_SUBSCRIBE_STEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {

    }
};

TEST_F(MSPROF_API_SUBSCRIBE_STEST, AnalyzerGe_Parse) {
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

TEST_F(MSPROF_API_SUBSCRIBE_STEST, AnalyzerGe_ParseOpName) {
    std::shared_ptr<Analysis::Dvvp::Analyze::AnalyzerGe> analyzerGe(new Analysis::Dvvp::Analyze::AnalyzerGe());
    struct MsprofGeProfTaskData geProfTaskData;
    memset(&geProfTaskData, 0, sizeof(MsprofGeProfTaskData));
    geProfTaskData.opName.type = MSPROF_MIX_DATA_STRING;
    struct Analysis::Dvvp::Analyze::AnalyzerGe::GeOpInfo opInfo;
    analyzerGe->ParseOpName(geProfTaskData, opInfo);
}

TEST_F(MSPROF_API_SUBSCRIBE_STEST, AnalyzerGe_ParseOpType) {
    std::shared_ptr<Analysis::Dvvp::Analyze::AnalyzerGe> analyzerGe(new Analysis::Dvvp::Analyze::AnalyzerGe());
    struct MsprofGeProfTaskData geProfTaskData;
    memset(&geProfTaskData, 0, sizeof(MsprofGeProfTaskData));
    geProfTaskData.opType.type = MSPROF_MIX_DATA_STRING;
    struct Analysis::Dvvp::Analyze::AnalyzerGe::GeOpInfo opInfo;
    analyzerGe->ParseOpType(geProfTaskData, opInfo);
}

class COMMANDHANDLE_STEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(COMMANDHANDLE_STEST, commandHandle_api) {
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
    EXPECT_EQ(ACL_ERROR, CommandHandleProfInit());
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
