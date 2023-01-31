/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: transport for parser
 * Author: zcj
 * Create: 2020-10-24
 */
#ifndef ANALYSIS_DVVP_ANALYZE_PARSER_TRANSPORT_H
#define ANALYSIS_DVVP_ANALYZE_PARSER_TRANSPORT_H

#include "prof_acl_mgr.h"
#include "transport.h"
#include "utils/utils.h"

namespace Analysis {
namespace Dvvp {
namespace Analyze {
class Analyzer;
}}}

namespace analysis {
namespace dvvp {
namespace transport {
class Uploader;
// ParserTransport
class ParserTransport : public ITransport {
public:
    ParserTransport() {};
    ~ParserTransport() override;

public:
    int Init(SHARED_PTR_ALIA<Uploader> uploader);
    int SendBuffer(CONST_VOID_PTR buffer, int length) override;
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
    int CloseSession() override;
    void WriteDone() override {}
};
}  // namespace transport
}  // namespace dvvp
}  // namespace analysis

#endif
