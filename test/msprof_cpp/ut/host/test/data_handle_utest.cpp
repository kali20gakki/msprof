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
#include "hdc-common/log/hdc_log.h"
#include "data_handle.h"
#include "errno/error_code.h"
#include "prof_manager.h"
#include "uploader_mgr.h"
#include "platform/platform.h"

using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::Transport;

class DATA_HANDLE_TEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(DATA_HANDLE_TEST, Msprof_ProcessStreamFileChunk) {
    GlobalMockObject::verify();

    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::PlatformIsSocSide)
        .stubs()
        .will(returnValue(true));

    MOCKER_CPP(&analysis::dvvp::transport::UploaderMgr::UploadData)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    std::shared_ptr<analysis::dvvp::ProfileFileChunk> req(new analysis::dvvp::ProfileFileChunk);
    analysis::dvvp::message::JobContext jobCtx;
    jobCtx.dev_id = "0";
    jobCtx.job_id = "jobId";
    req->extraInfo = Utils::PackDotInfo("jobId", "0");
    req->fileName = Utils::PackDotInfo("Framework", "");
    // UPLOAD FAILED
    EXPECT_EQ(PROFILING_FAILED, MsprofDataHandle::ProcessStreamFileChunk(req));
    // OK
    EXPECT_EQ(PROFILING_SUCCESS, MsprofDataHandle::ProcessStreamFileChunk(req));
}

TEST_F(DATA_HANDLE_TEST, Msprof_ProcessFileChunkFlush) {
    GlobalMockObject::verify();

    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::PlatformIsSocSide)
        .stubs()
        .will(returnValue(true));

    MOCKER_CPP(&analysis::dvvp::transport::UploaderMgr::UploadData)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    std::shared_ptr<analysis::dvvp::proto::FileChunkFlushReq> req(new analysis::dvvp::proto::FileChunkFlushReq);
    analysis::dvvp::message::JobContext jobCtx;
    jobCtx.dev_id = "0";
    jobCtx.job_id = "jobId";
    req->mutable_hdr()->set_job_ctx(jobCtx.ToString());
    req->set_filename("DATA_PREPROCESS");
    // UPLOAD FAILED
    EXPECT_EQ(PROFILING_SUCCESS, MsprofDataHandle::ProcessFileChunkFlush(req));
    // OK
    EXPECT_EQ(PROFILING_SUCCESS, MsprofDataHandle::ProcessFileChunkFlush(req));

    EXPECT_EQ(PROFILING_FAILED, MsprofDataHandle::SendData("jobId", 0, "DATA_PREPROCESS", jobCtx));
}

TEST_F(DATA_HANDLE_TEST, PlatformTest) {
    GlobalMockObject::verify();

    auto platform = Analysis::Dvvp::Common::Platform::Platform::instance();

    EXPECT_EQ(PROFILING_SUCCESS, platform->Init());
    EXPECT_EQ(PROFILING_SUCCESS, platform->Uninit());
    platform->SetPlatformSoc();
    EXPECT_EQ(Analysis::Dvvp::Common::Platform::PlatformType::DEVICE, platform->GetPlatform());
}
