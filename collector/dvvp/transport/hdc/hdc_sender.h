/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: handle profiling request
 * Author: xp
 * Create: 2020-12-24
 */
#ifndef ANALYSIS_DVVP_COMMON_HDC_SENDER_H
#define ANALYSIS_DVVP_COMMON_HDC_SENDER_H

#include "sender.h"
#include "transport/transport.h"

namespace analysis {
namespace dvvp {
namespace transport {
using namespace analysis::dvvp::streamio::client;
class HdcSender {
public:
    HdcSender();
    virtual ~HdcSender();

public:
    int Init(SHARED_PTR_ALIA<ITransport> transport, const std::string &engineName);
    void Uninit() const;
    int SendData(const std::string &jobCtx, const struct data_chunk &data);
    void Flush() const;

private:
    std::string engineName_;
    SHARED_PTR_ALIA<analysis::dvvp::common::memory::ChunkPool> chunkPool_;
    SHARED_PTR_ALIA<Sender> sender_;
    SHARED_PTR_ALIA<ITransport> transport_;
};
}  // namespace transport
}  // namespace dvvp
}  // namespace analysis

#endif  // ANALYSIS_DVVP_COMMON_HDC_SENDER_H


