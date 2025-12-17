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
#ifndef ANALYSIS_DVVP_ANALYZE_PARSER_TRANSPORT_H
#define ANALYSIS_DVVP_ANALYZE_PARSER_TRANSPORT_H

#include "prof_acl_mgr.h"
#include "transport.h"
#include "utils/utils.h"
#include "analyzer.h"

namespace analysis {
namespace dvvp {
namespace transport {
// ParserTransport
class ParserTransport : public ITransport {
public:
    ParserTransport() {};
    ~ParserTransport() override;

public:
    int Init(SHARED_PTR_ALIA<Uploader> uploader);
    int SendBuffer(CONST_VOID_PTR buffer, int length) override;
    int SendBuffer(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunkReq) override;
    int CloseSession() override;
    void WriteDone() override;
    void SetDevIdToAnalyzer(const std::string &devIdStr) const;

private:
    SHARED_PTR_ALIA<Analysis::Dvvp::Analyze::Analyzer> analyzer_;
};

// PipeTransport
class PipeTransport : public ITransport {
public:
    int SendBuffer(CONST_VOID_PTR buffer, int length) override;
    int SendBuffer(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunkReq) override;
    int CloseSession() override;
    void WriteDone() override {}
};
}  // namespace transport
}  // namespace dvvp
}  // namespace analysis

#endif
