/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: UT for analyzer module
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2023-05-08
 */
#include <map>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analyzer.h"
#include "config/config.h"
#include "data_struct.h"
#include "errno/error_code.h"
#include "op_desc_parser.h"
#include "message/codec.h"
#include "prof_channel_manager.h"
#include "proto/profiler.pb.h"
#include "uploader.h"
#include "uploader_mgr.h"
#include "toolchain/prof_acl_api.h"
#include "transport/hash_data.h"
#include "transport/uploader.h"

using namespace Analysis::Dvvp::Analyze;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::transport;

class AnalyzerUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

TEST_F(AnalyzerUtest, PrintDeviceStats)
{
    GlobalMockObject::verify();
    std::shared_ptr<Analyzer> analyzer;
    MSVP_MAKE_SHARED1_BREAK(analyzer, Analyzer, nullptr);

    analyzer->Init();
    analyzer->PrintDeviceStats();
}

TEST_F(AnalyzerUtest, PrintHostStats)
{
    GlobalMockObject::verify();
    std::shared_ptr<Analyzer> analyzer;
    MSVP_MAKE_SHARED1_BREAK(analyzer, Analyzer, nullptr);

    analyzer->Init();
    analyzer->PrintHostStats();
}

TEST_F(AnalyzerUtest, DispatchOptimizeUnagingApiData)
{
    GlobalMockObject::verify();
    std::shared_ptr<Analyzer> analyzer;
    MSVP_MAKE_SHARED1_BREAK(analyzer, Analyzer, nullptr);
    
    analyzer->Init();
    // ge api unaging
    struct MsprofApi geUnAgingTaskDescChunk;
    geUnAgingTaskDescChunk.type = MSPROF_REPORT_NODE_LAUNCH_TYPE;
    geUnAgingTaskDescChunk.beginTime = 40; // 40 begin time
    geUnAgingTaskDescChunk.endTime = 60; // 60 end time
    geUnAgingTaskDescChunk.threadId = 1;
    std::string geUnAgingOriData((CHAR_PTR)&geUnAgingTaskDescChunk, sizeof(geUnAgingTaskDescChunk));
    std::shared_ptr<analysis::dvvp::proto::FileChunkReq> geUnAgingTaskDesc;
    MSVP_MAKE_SHARED0_BREAK(geUnAgingTaskDesc, analysis::dvvp::proto::FileChunkReq);
    geUnAgingTaskDesc->set_filename("unaging.api");
    geUnAgingTaskDesc->set_chunk(geUnAgingOriData);
    geUnAgingTaskDesc->set_chunksizeinbytes(sizeof(geUnAgingTaskDescChunk));
    analyzer->DispatchOptimizeData(geUnAgingTaskDesc);
}

TEST_F(AnalyzerUtest, DispatchOptimizeAgingApiData)
{
    GlobalMockObject::verify();
    std::shared_ptr<Analyzer> analyzer;
    MSVP_MAKE_SHARED1_BREAK(analyzer, Analyzer, nullptr);
    
    analyzer->Init();
    // ge api aging
    struct MsprofApi geAgingTaskDescChunk;
    geAgingTaskDescChunk.type = MSPROF_REPORT_NODE_LAUNCH_TYPE;
    std::string geUnAgingOriData((CHAR_PTR)&geAgingTaskDescChunk, sizeof(geAgingTaskDescChunk));
    std::shared_ptr<analysis::dvvp::proto::FileChunkReq> geAgingTaskDesc;
    MSVP_MAKE_SHARED0_BREAK(geAgingTaskDesc, analysis::dvvp::proto::FileChunkReq);
    geAgingTaskDesc->set_filename("aging.api");
    geAgingTaskDesc->set_chunk(geUnAgingOriData);
    geAgingTaskDesc->set_chunksizeinbytes(sizeof(geAgingTaskDescChunk));
    analyzer->DispatchOptimizeData(geAgingTaskDesc);
}

TEST_F(AnalyzerUtest, DispatchOptimizeNodeBasicInfoData)
{
    GlobalMockObject::verify();
    std::shared_ptr<Analyzer> analyzer;
    MSVP_MAKE_SHARED1_BREAK(analyzer, Analyzer, nullptr);
    
    analyzer->Init();
    // ge node basic info
    struct MsprofNodeBasicInfo nodeData;
    struct MsprofCompactInfo geTaskDescChunk;
    nodeData.opName = 0;
    nodeData.opType = 0;
    geTaskDescChunk.level = MSPROF_REPORT_NODE_LEVEL;
    geTaskDescChunk.data.nodeBasicInfo = nodeData;
    geTaskDescChunk.threadId = 1;
    geTaskDescChunk.timeStamp = 0;
    std::string geOriData((CHAR_PTR)&geTaskDescChunk, sizeof(geTaskDescChunk));
    std::shared_ptr<analysis::dvvp::proto::FileChunkReq> geTaskDesc;
    MSVP_MAKE_SHARED0_BREAK(geTaskDesc, analysis::dvvp::proto::FileChunkReq);
    geTaskDesc->set_filename("unaging.compact");
    geTaskDesc->set_tag("node_basic_info");
    geTaskDesc->set_chunk(geOriData);
    geTaskDesc->set_chunksizeinbytes(sizeof(geTaskDescChunk));
    analyzer->DispatchOptimizeData(geTaskDesc);
}

TEST_F(AnalyzerUtest, DispatchOptimizeGraphIdInfoData)
{
    GlobalMockObject::verify();
    std::shared_ptr<Analyzer> analyzer;
    MSVP_MAKE_SHARED1_BREAK(analyzer, Analyzer, nullptr);

    analyzer->Init();
    // graph id info
    struct MsprofGraphIdInfo graphData;
    struct MsprofAdditionalInfo geAddChunk;
    geAddChunk.level = MSPROF_REPORT_MODEL_LEVEL;
    std::string geAddData((CHAR_PTR)&geAddChunk, sizeof(geAddChunk));
    std::shared_ptr<analysis::dvvp::proto::FileChunkReq> geAddDesc;
    MSVP_MAKE_SHARED0_BREAK(geAddDesc, analysis::dvvp::proto::FileChunkReq);
    geAddDesc->set_filename("Additional");
    geAddDesc->set_tag("graph_id_map");
    geAddDesc->set_chunk(geAddData);
    geAddDesc->set_chunksizeinbytes(sizeof(geAddChunk));
    analyzer->DispatchOptimizeData(geAddDesc);
}

TEST_F(AnalyzerUtest, DispatchOptimizeContextIdInfoData)
{
    GlobalMockObject::verify();
    std::shared_ptr<Analyzer> analyzer;
    MSVP_MAKE_SHARED1_BREAK(analyzer, Analyzer, nullptr);

    analyzer->Init();
    // context id info
    struct MsprofContextIdInfo contxtIdData;
    struct MsprofAdditionalInfo geAddChunk;
    geAddChunk.level = MSPROF_REPORT_NODE_LEVEL;
    geAddChunk.type = MSPROF_REPORT_NODE_CONTEXT_ID_INFO_TYPE;
    std::string geAddData((CHAR_PTR)&geAddChunk, sizeof(geAddChunk));
    std::shared_ptr<analysis::dvvp::proto::FileChunkReq> geAddDesc;
    MSVP_MAKE_SHARED0_BREAK(geAddDesc, analysis::dvvp::proto::FileChunkReq);
    geAddDesc->set_filename("Additional");
    geAddDesc->set_tag("context_id_info");
    geAddDesc->set_chunk(geAddData);
    geAddDesc->set_chunksizeinbytes(sizeof(geAddChunk));
    analyzer->DispatchOptimizeData(geAddDesc);
}

TEST_F(AnalyzerUtest, DispatchOptimizeModelInfoData)
{
    GlobalMockObject::verify();
    std::shared_ptr<Analyzer> analyzer;
    MSVP_MAKE_SHARED1_BREAK(analyzer, Analyzer, nullptr);

    analyzer->Init();
    // ge model info
    struct MsprofEvent geUnAgingEventChunk;
    geUnAgingEventChunk.level = MSPROF_REPORT_MODEL_LEVEL;
    geUnAgingEventChunk.type = MSPROF_REPORT_MODEL_LOAD_TYPE;
    geUnAgingEventChunk.threadId = 1;
    geUnAgingEventChunk.itemId = 1;
    geUnAgingEventChunk.timeStamp = 0;
    std::string geUnAgingStartOriData((CHAR_PTR)&geUnAgingEventChunk, sizeof(geUnAgingEventChunk));
    std::shared_ptr<analysis::dvvp::proto::FileChunkReq> geModelLoad;
    MSVP_MAKE_SHARED0_BREAK(geModelLoad, analysis::dvvp::proto::FileChunkReq);
    geModelLoad->set_filename("unaging.event");
    geModelLoad->set_chunk(geUnAgingStartOriData);
    geModelLoad->set_chunksizeinbytes(sizeof(geUnAgingEventChunk));
    analyzer->DispatchOptimizeData(geModelLoad);

    geUnAgingEventChunk.timeStamp = 1;
    std::string geUnAgingEndOriData((CHAR_PTR)&geUnAgingEventChunk, sizeof(geUnAgingEventChunk));
    geModelLoad->set_filename("unaging.event");
    geModelLoad->set_chunk(geUnAgingEndOriData);
    geModelLoad->set_chunksizeinbytes(sizeof(geUnAgingEventChunk));
    analyzer->DispatchOptimizeData(geModelLoad);
}

TEST_F(AnalyzerUtest, UploadProfOpDescProc_nullUploader)
{
    GlobalMockObject::verify();
    std::shared_ptr<Analyzer> analyzer;
    MSVP_MAKE_SHARED1_BREAK(analyzer, Analyzer, nullptr);

    std::shared_ptr<AnalyzerBase> analyzerBase;
    MSVP_MAKE_SHARED0_BREAK(analyzerBase, AnalyzerBase);

    analyzer->Init();
    
    ProfOpDesc opDescData;
    analyzerBase->opDescInfos_.push_back(opDescData);
    analyzer->UploadProfOpDescProc();
}

TEST_F(AnalyzerUtest, UploadProfOpDescProc_validUploader)
{
    SHARED_PTR_ALIA<Uploader> pipeUploader = nullptr;
    MSVP_MAKE_SHARED1_BREAK(pipeUploader, Uploader, nullptr);
    pipeUploader->Init(100000); // 100000 uploader buffer size

    std::shared_ptr<Analyzer> analyzer;
    MSVP_MAKE_SHARED1_BREAK(analyzer, Analyzer, pipeUploader);

    analyzer->Init();
    
    std::shared_ptr<AnalyzerBase> analyzerBase;
    MSVP_MAKE_SHARED0_BREAK(analyzerBase, AnalyzerBase);
    ProfOpDesc opDescData;
    analyzerBase->opDescInfos_.push_back(opDescData);
    analyzer->UploadProfOpDescProc();
}

class AnalyzerBaseUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};
TEST_F(AnalyzerBaseUtest, HandleDeviceData)
{
    GlobalMockObject::verify();
    std::shared_ptr<AnalyzerBase> analyzerBase;
    MSVP_MAKE_SHARED0_BREAK(analyzerBase, AnalyzerBase);

    uint32_t threadId = 1;
    // rtOpInfo_ cannot find key
    std::string key1 = "0-0";
    struct RtOpInfo devData;
    devData.start = 0;
    devData.end = 0;
    uint32_t taskId = 0;
    uint16_t streamId = 0;
    uint32_t time;
    analyzerBase->HandleDeviceData(key1, devData, time);

    // rtOpInfo_ find key with invalid devData
    struct RtOpInfo opInfo = {1, 0, 0, threadId, 0, 0, 0, ACL_SUBSCRIBE_OP};
    analyzerBase->rtOpInfo_.insert(std::make_pair(key1, opInfo));
    analyzerBase->HandleDeviceData(key1, devData, time);

    // rtOpInfo_ find key with valid devData
    devData.start = 1; // 1 data start
    devData.end = 2; // 2 data end
    analyzerBase->tsOpInfo_.insert(std::make_pair(key1, opInfo));
    std::string key2 = "1-1";
    analyzerBase->tsOpInfo_.insert(std::make_pair(key1, opInfo));
    analyzerBase->HandleDeviceData(key1, devData, time);

    // rtOpInfo_ find key with valid devData and valid geOpFlagInfo
    uint64_t opNameHash = HashData::instance()->GenHashId("opName");
    uint64_t opTypeHash = HashData::instance()->GenHashId("opType");
    analyzerBase->tsOpInfo_.insert(std::make_pair(key1, opInfo));
    uint64_t modelId = 0;
    struct GeOpFlagInfo geInfo1 = {opNameHash, opTypeHash, modelId, 0, 2, 0, 0, ACL_SUBSCRIBE_OP, UINT16_MAX};
    struct GeOpFlagInfo geInfo2 = {opNameHash, opTypeHash, modelId, 3, 4, 0, 0,
                                   ACL_SUBSCRIBE_OP, UINT16_MAX}; // 3 4 invalid ge info
    analyzerBase->geOpInfo_.insert(std::make_pair(threadId, geInfo1));
    analyzerBase->geOpInfo_.insert(std::make_pair(threadId, geInfo2));
    analyzerBase->HandleDeviceData(key1, devData, time);

    analyzerBase->tsOpInfo_.insert(std::make_pair(key1, opInfo));
    uint32_t graphId = 0;
    analyzerBase->SetGraphModelId(modelId, graphId);
    analyzerBase->HandleDeviceData(key1, devData, time);
}

class AnalyzerFftsUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

TEST_F(AnalyzerFftsUtest, FftsParse)
{
    GlobalMockObject::verify();
    std::shared_ptr<AnalyzerFfts> analyzerFfts;
    MSVP_MAKE_SHARED0_BREAK(analyzerFfts, AnalyzerFfts);
    FftsCxtLog fftsCxtLog;
    fftsCxtLog.head.logType = FFTS_SUBTASK_THREAD_START_FUNC_TYPE;
    fftsCxtLog.streamId = 0;
    fftsCxtLog.taskId = 0;
    fftsCxtLog.sysCountLow = 1000; // 1000 count low
    fftsCxtLog.sysCountHigh = 0;
    fftsCxtLog.cxtId = 0;
    fftsCxtLog.threadId = 0;
    std::shared_ptr<analysis::dvvp::proto::FileChunkReq> ffts;
    MSVP_MAKE_SHARED0_BREAK(ffts, analysis::dvvp::proto::FileChunkReq);
    ffts->set_filename("stars_soc.data");
    ffts->set_chunk(reinterpret_cast<CHAR_PTR>(&fftsCxtLog), sizeof(fftsCxtLog));
    ffts->set_chunksizeinbytes(sizeof(fftsCxtLog));
    analyzerFfts->FftsParse(ffts);
    fftsCxtLog.head.logType = FFTS_SUBTASK_THREAD_END_FUNC_TYPE;
    fftsCxtLog.sysCountLow = 2000; // 2000 count high
    fftsCxtLog.sysCountHigh = 0;
    ffts->set_chunk(reinterpret_cast<CHAR_PTR>(&fftsCxtLog), sizeof(fftsCxtLog));
    analyzerFfts->FftsParse(ffts);

    FftsAcsqLog fftsAcsqLog;
    fftsAcsqLog.head.logType = ACSQ_TASK_START_FUNC_TYPE;
    fftsAcsqLog.streamId = 1;
    fftsAcsqLog.taskId = 1;
    fftsAcsqLog.sysCountLow = 10000; // 10000 count low
    ffts->set_chunk(reinterpret_cast<CHAR_PTR>(&fftsAcsqLog), sizeof(fftsAcsqLog));
    analyzerFfts->FftsParse(ffts);
    fftsAcsqLog.head.logType = ACSQ_TASK_END_FUNC_TYPE;
    fftsAcsqLog.sysCountLow = 20000; // 20000 count high
    ffts->set_chunk(reinterpret_cast<CHAR_PTR>(&fftsAcsqLog), sizeof(fftsAcsqLog));
    analyzerFfts->FftsParse(ffts);
}

TEST_F(AnalyzerFftsUtest, ParseOptimizeAcsqTaskData)
{
    GlobalMockObject::verify();
    std::shared_ptr<AnalyzerFfts> analyzerFfts;
    MSVP_MAKE_SHARED0_BREAK(analyzerFfts, AnalyzerFfts);

    analyzerFfts->ParseOptimizeAcsqTaskData(nullptr, 0);
    std::string key = "0-0";
    uint32_t threadId = 1;
    struct RtOpInfo opInfo = {1, 0, 0, threadId, 0, 0, 0, ACL_SUBSCRIBE_OP};
    std::shared_ptr<AnalyzerBase> analyzerBase;
    MSVP_MAKE_SHARED0_BREAK(analyzerBase, AnalyzerBase);
    
    struct FftsAcsqLog data1;
    data1.head.logType = ACSQ_TASK_START_FUNC_TYPE;
    data1.streamId = 0;
    data1.taskId = 0;
    analyzerFfts->ParseOptimizeAcsqTaskData(reinterpret_cast<const FftsLogHead *>(
        (reinterpret_cast<const VOID_PTR>(&data1))), data1.head.logType);

    struct FftsAcsqLog data2;
    data2.head.logType = ACSQ_TASK_END_FUNC_TYPE;
    data2.streamId = 0;
    data2.taskId = 0;
    analyzerFfts->ParseOptimizeAcsqTaskData(reinterpret_cast<const FftsLogHead *>(
        (reinterpret_cast<const VOID_PTR>(&data2))), data2.head.logType);
}

TEST_F(AnalyzerFftsUtest, ParseOptimizeSubTaskThreadData)
{
    GlobalMockObject::verify();
    std::shared_ptr<AnalyzerFfts> analyzerFfts;
    MSVP_MAKE_SHARED0_BREAK(analyzerFfts, AnalyzerFfts);

    analyzerFfts->ParseOptimizeSubTaskThreadData(nullptr, 0);
    std::string key = "0-0";
    uint32_t threadId = 1;
    struct RtOpInfo opInfo = {1, 0, 0, threadId, 0, 0, 0, ACL_SUBSCRIBE_OP};
    std::shared_ptr<AnalyzerBase> analyzerBase;
    MSVP_MAKE_SHARED0_BREAK(analyzerBase, AnalyzerBase);
    
    struct FftsCxtLog data1;
    data1.head.logType = FFTS_SUBTASK_THREAD_START_FUNC_TYPE;
    data1.streamId = 0;
    data1.taskId = 0;
    analyzerFfts->ParseOptimizeSubTaskThreadData(reinterpret_cast<const FftsLogHead *>(
        (reinterpret_cast<const VOID_PTR>(&data1))), data1.head.logType);

    struct FftsCxtLog data2;
    data2.head.logType = FFTS_SUBTASK_THREAD_END_FUNC_TYPE;
    data2.streamId = 0;
    data2.taskId = 0;
    analyzerFfts->ParseOptimizeSubTaskThreadData(reinterpret_cast<const FftsLogHead *>(
        (reinterpret_cast<const VOID_PTR>(&data2))), data2.head.logType);
}

class AnalyzerGeUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

TEST_F(AnalyzerGeUtest, IsGeApiData)
{
    GlobalMockObject::verify();
    std::shared_ptr<AnalyzerGe> analyzerGe;
    MSVP_MAKE_SHARED0_BREAK(analyzerGe, AnalyzerGe);

    EXPECT_EQ(true, analyzerGe->IsGeApiData("aging.api"));
    EXPECT_EQ(false, analyzerGe->IsGeApiData("aging.event"));
}

TEST_F(AnalyzerGeUtest, IsGeEventData)
{
    GlobalMockObject::verify();
    std::shared_ptr<AnalyzerGe> analyzerGe;
    MSVP_MAKE_SHARED0_BREAK(analyzerGe, AnalyzerGe);

    EXPECT_EQ(false, analyzerGe->IsGeEventData("aging.api"));
    EXPECT_EQ(true, analyzerGe->IsGeEventData("aging.event"));
}

TEST_F(AnalyzerGeUtest, IsGeCompactData)
{
    GlobalMockObject::verify();
    std::shared_ptr<AnalyzerGe> analyzerGe;
    MSVP_MAKE_SHARED0_BREAK(analyzerGe, AnalyzerGe);

    EXPECT_EQ(true, analyzerGe->IsGeCompactData("aging.compact.node_basic_info"));
    EXPECT_EQ(false, analyzerGe->IsGeCompactData("aging.compact.graph_id_map"));
}

TEST_F(AnalyzerGeUtest, IsGeGraphIdMapData)
{
    GlobalMockObject::verify();
    std::shared_ptr<AnalyzerGe> analyzerGe;
    MSVP_MAKE_SHARED0_BREAK(analyzerGe, AnalyzerGe);

    EXPECT_EQ(false, analyzerGe->IsGeGraphIdMapData("aging.compact.node_basic_info"));
    EXPECT_EQ(true, analyzerGe->IsGeGraphIdMapData("aging.compact.graph_id_map"));
}

TEST_F(AnalyzerGeUtest, IsGeContextData)
{
    GlobalMockObject::verify();
    std::shared_ptr<AnalyzerGe> analyzerGe;
    MSVP_MAKE_SHARED0_BREAK(analyzerGe, AnalyzerGe);

    EXPECT_EQ(true, analyzerGe->IsGeContextData("aging.additional.context_id_info"));
    EXPECT_EQ(false, analyzerGe->IsGeContextData("aging.additional.context_info"));
}

class AnalyzerHwtsUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

TEST_F(AnalyzerHwtsUtest, ParseOptimizeHwtsData)
{
    GlobalMockObject::verify();
    std::shared_ptr<AnalyzerHwts> analyzerHwts;
    MSVP_MAKE_SHARED0_BREAK(analyzerHwts, AnalyzerHwts);
    struct HwtsProfileType01 hwtsChunk;
    hwtsChunk.taskId = 1;
    hwtsChunk.streamId = 1;
    hwtsChunk.cntRes0Type = HWTS_TASK_START_TYPE;
    hwtsChunk.syscnt = 10000; // 10000 start
    std::shared_ptr<analysis::dvvp::proto::FileChunkReq> hwts;
    MSVP_MAKE_SHARED0_BREAK(hwts, analysis::dvvp::proto::FileChunkReq);
    hwts->set_filename("hwts.data");
    hwts->set_chunk(reinterpret_cast<CHAR_PTR>(&hwtsChunk), sizeof(HwtsProfileType01));
    hwts->set_chunksizeinbytes(sizeof(HwtsProfileType01));
    std::string data = analysis::dvvp::message::EncodeMessage(hwts);
    analyzerHwts->ParseOptimizeHwtsData(data.c_str(), static_cast<uint32_t>(data.size()));

    hwtsChunk.cntRes0Type = HWTS_TASK_END_TYPE;
    hwtsChunk.syscnt = 20000; // 20000 end
    hwts->set_chunk(reinterpret_cast<CHAR_PTR>(&hwtsChunk), sizeof(HwtsProfileType01));
    hwts->set_chunksizeinbytes(sizeof(HwtsProfileType01));
    data = analysis::dvvp::message::EncodeMessage(hwts);
    analyzerHwts->ParseOptimizeHwtsData(data.c_str(), static_cast<uint32_t>(data.size()));
}

class AnalyzerRtUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

TEST_F(AnalyzerRtUtest, IsRtCompactData)
{
    GlobalMockObject::verify();
    std::shared_ptr<AnalyzerRt> analyzerRt;
    MSVP_MAKE_SHARED0_BREAK(analyzerRt, AnalyzerRt);

    EXPECT_EQ(true, analyzerRt->IsRtCompactData("aging.compact.task_track"));
    EXPECT_EQ(false, analyzerRt->IsRtCompactData("aging.compact"));
}

TEST_F(AnalyzerRtUtest, RtCompactParse)
{
    GlobalMockObject::verify();
    std::shared_ptr<AnalyzerRt> analyzerRt;
    MSVP_MAKE_SHARED0_BREAK(analyzerRt, AnalyzerRt);

    // rt unaging
    struct MsprofCompactInfo rtDataChunk;
    struct MsprofRuntimeTrack rtInnerData;
    rtInnerData.taskId = 1;
    rtInnerData.streamId = 1;
    rtDataChunk.data.runtimeTrack = rtInnerData;
    rtDataChunk.level = MSPROF_REPORT_RUNTIME_LEVEL;
    rtDataChunk.timeStamp = 1;
    rtDataChunk.threadId = 1;
    std::string rtData((CHAR_PTR)&rtDataChunk, sizeof(rtDataChunk));
    std::shared_ptr<analysis::dvvp::proto::FileChunkReq> rtTaskDesc;
    MSVP_MAKE_SHARED0_BREAK(rtTaskDesc, analysis::dvvp::proto::FileChunkReq);
    rtTaskDesc->set_filename("unaging.compact");
    rtTaskDesc->set_tag("task_track");
    rtTaskDesc->set_chunk(rtData);
    rtTaskDesc->set_chunksizeinbytes(sizeof(rtDataChunk));
    analyzerRt->RtCompactParse(rtTaskDesc);

    // rt aging
    struct MsprofCompactInfo rtDataAgingChunk;
    struct MsprofRuntimeTrack rtInnerAgingData;
    rtInnerAgingData.taskId = 1;
    rtInnerAgingData.streamId = 1;
    rtDataAgingChunk.data.runtimeTrack = rtInnerAgingData;
    rtDataAgingChunk.level = MSPROF_REPORT_RUNTIME_LEVEL;
    rtDataAgingChunk.timeStamp = 1;
    rtDataAgingChunk.threadId = 1;
    std::string rtAgingData((CHAR_PTR)&rtDataAgingChunk, sizeof(rtDataChunk));
    std::shared_ptr<analysis::dvvp::proto::FileChunkReq> rtAgingTaskDesc;
    MSVP_MAKE_SHARED0_BREAK(rtAgingTaskDesc, analysis::dvvp::proto::FileChunkReq);
    rtAgingTaskDesc->set_filename("aging.compact");
    rtAgingTaskDesc->set_tag("task_track");
    rtAgingTaskDesc->set_chunk(rtAgingData);
    rtAgingTaskDesc->set_chunksizeinbytes(sizeof(rtDataAgingChunk));
    analyzerRt->RtCompactParse(rtAgingTaskDesc);
}

