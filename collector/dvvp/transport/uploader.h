/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_TRANSPORT_UPLOADER_H
#define ANALYSIS_DVVP_TRANSPORT_UPLOADER_H

#include <cstddef>
#include "config/config.h"
#include "message/codec.h"
#include "message/dispatcher.h"
#include "queue/bound_queue.h"
#include "thread/thread.h"
#include "transport/transport.h"

namespace analysis {
namespace dvvp {
namespace transport {
using UploaderQueue = analysis::dvvp::common::queue::BoundQueue<SHARED_PTR_ALIA<std::string> >;
class Uploader : public analysis::dvvp::common::thread::Thread {
using analysis::dvvp::common::thread::Thread::Stop;
public:
    explicit Uploader(SHARED_PTR_ALIA<ITransport> transport);

    virtual ~Uploader();

public:
    int Init(size_t size = analysis::dvvp::common::config::UPLOADER_QUEUE_CAPACITY);
    int Uinit();
    int UploadData(CONST_VOID_PTR data, int len);
    int Stop(bool force);
    void Flush() const;
    void CloseTransport();
    SHARED_PTR_ALIA<ITransport> GetTransport();
    int GetQueueSize() const;

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
