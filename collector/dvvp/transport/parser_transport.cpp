/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: transport for parser
 * Author: zcj
 * Create: 2020-10-24
 */
#include "parser_transport.h"

#include <cstring>

#include "analyzer.h"
#include "errno/error_code.h"
#include "op_desc_parser.h"
#include "msprof_dlog.h"
#include "prof_acl_mgr.h"

namespace analysis {
namespace dvvp {
namespace transport {
using namespace analysis::dvvp::common::error;
using namespace Msprofiler::Api;
using namespace Collector::Dvvp::Plugin;
using namespace Collector::Dvvp::Mmpa;

ParserTransport::ParserTransport(SHARED_PTR_ALIA<Uploader> uploader)
{
    MSVP_MAKE_SHARED1_VOID(analyzer_, Analysis::Dvvp::Analyze::Analyzer, uploader);
}

ParserTransport::~ParserTransport()
{
    CloseSession();
}

int ParserTransport::SendBuffer(CONST_VOID_PTR buffer, int length)
{
    if (analyzer_ != nullptr) {
        analyzer_->OnNewData(buffer, length);
        return length;
    }
    MSPROF_LOGE("Analyzer is already closed");
    return 0;
}

int ParserTransport::CloseSession()
{
    MSPROF_LOGI("ParserTransport Close");
    if (analyzer_ != nullptr) {
        analyzer_->Flush();
        analyzer_->PrintStats();
        analyzer_.reset();
    }
    return PROFILING_SUCCESS;
}

void ParserTransport::WriteDone()
{
    MSPROF_LOGI("ParserTransport WriteDone");
    if (analyzer_ != nullptr) {
        analyzer_->Flush();
    }
}

void ParserTransport::SetDevIdToAnalyzer(const std::string &devIdStr)
{
    MSPROF_LOGI("ParserTransport SetDeviceId");
    if (analyzer_ != nullptr) {
        analyzer_->SetDevId(devIdStr);
    }
}

int PipeTransport::SendBuffer(CONST_VOID_PTR buffer, int length)
{
    int sent = 0;
    static const unsigned long pipeFullSleepUs = 1000;  // sleep 1ms and retry
    uint32_t modelId = 0;
    int32_t ret = Analysis::Dvvp::Analyze::OpDescParser::GetModelId(buffer, length, 0, &modelId);
    if (ret != ACL_SUCCESS) {
        MSPROF_LOGE("Failed to parse model id from data");
        return sent;
    }

    int fd = Msprofiler::Api::ProfAclMgr::instance()->GetSubscribeFdForModel(modelId);
    if (fd < 0) {
        MSPROF_LOGW("Model %u not subscribed, drop buffer, size %d", modelId, length);
        return length;
    }
    MSPROF_LOGD("Write %d bytes to fd %d", length, fd);
    int count = 0;
    while (true) {
        sent = MmWrite(fd, const_cast<VOID_PTR>(buffer), length);
        if (sent >= 0) {
            break;
        }
        if (errno == EAGAIN) {
            if (count++ % 1000 == 0) {  // record log every 1000 times
                MSPROF_LOGW("Pipe is full, count: %d", count);
            }
            analysis::dvvp::common::utils::Utils::UsleepInterupt(pipeFullSleepUs);
        } else {
            analysis::dvvp::common::utils::Utils::PrintSysErrorMsg();
            break;
        }
    }
    return sent;
}
int PipeTransport::CloseSession()
{
    return PROFILING_SUCCESS;
}
}  // namespace transport
}  // namespace dvvp
}  // namespace analysis
