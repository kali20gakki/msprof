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
#ifndef ANALYSIS_DVVP_TRANSPORT_UPLOADER_H
#define ANALYSIS_DVVP_TRANSPORT_UPLOADER_H

#include <cstddef>
#include "config/config.h"

#include "queue/bound_queue.h"
#include "thread/thread.h"
#include "transport/transport.h"

namespace analysis {
namespace dvvp {
namespace transport {
using UploaderQueue = analysis::dvvp::common::queue::BoundQueue<SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> >;
class Uploader : public analysis::dvvp::common::thread::Thread {
using analysis::dvvp::common::thread::Thread::Stop;
public:
    explicit Uploader(SHARED_PTR_ALIA<ITransport> transport);

    virtual ~Uploader();

public:
    int Init(size_t size = analysis::dvvp::common::config::UPLOADER_QUEUE_CAPACITY);
    int Uinit();
    int UploadData(CONST_VOID_PTR data, int len);
    int UploadData(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunkReq);
    int Stop(bool force);
    void Flush() const;
    void CloseTransport();
    SHARED_PTR_ALIA<ITransport> GetTransport();

protected:
    void Run(const struct error_message::Context &errorContext) override;

private:
    SHARED_PTR_ALIA<ITransport> transport_;
    SHARED_PTR_ALIA<UploaderQueue> queue_;
    bool isInited_;
    volatile bool forceQuit_;
    volatile bool isStopped_;
};
}  // namespace transport
}  // namespace dvvp
}  // namespace analysis

#endif
