/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: yutianqi
 * Create: 2018-06-13
 */
#ifndef COLLECT_IO_SENDER_H_INCLUDED
#define COLLECT_IO_SENDER_H_INCLUDED

#include <condition_variable>
#include <chrono>
#include <mutex>
#include <google/protobuf/message.h>
#include "common.h"
#include "file.h"
#include "mmpa_api.h"
#include "queue/bound_queue.h"
#include "singleton/singleton.h"
#include "thread/thread_pool.h"
#include "utils/utils.h"
#include "transport/transport.h"

namespace analysis {
namespace dvvp {
namespace streamio {
namespace client {
using namespace analysis::dvvp::transport;
typedef analysis::dvvp::common::queue::BoundQueue<SHARED_PTR_ALIA<File>> FileQueue;

struct data_chunk {
    char*  relativeFileName;// from subpath begin; For example: subA/subB/example.txt; Note: the begin don't has '/';
    unsigned char*  dataBuf;// the pointer to the data
    unsigned int    bufLen;// the len of dataBuf
    unsigned int    isLastChunk;// = 1, the last chunk of the file; != 1, not the last chunk of the file
    long long       offset;// the begin location of the file to write; if the offset is -1, directly append data.
};

class Sender : public analysis::dvvp::common::thread::Task, public std::enable_shared_from_this<Sender> {
public:
    Sender(SHARED_PTR_ALIA<ITransport> transport, const std::string &engineName,
           SHARED_PTR_ALIA<analysis::dvvp::common::memory::ChunkPool> chunkPool);
    virtual ~Sender();

public:
    int Init();
    void Uninit();

    int Send(const std::string &jobCtxJson, const struct data_chunk &data);
    void Flush();

public:
    virtual int Execute();
    virtual size_t HashId();

private:
    void FlushFileCache();
    SHARED_PTR_ALIA<File> GetFirstFileFromCache();
    SHARED_PTR_ALIA<File> GetFileFromCache(const std::string &fileName);
    void AddFileIntoCache(const std::string &fileName, SHARED_PTR_ALIA<File> file);
    SHARED_PTR_ALIA<std::mutex> GetMutexByFileName(const std::string &fileName);
    void DispatchFile(SHARED_PTR_ALIA<File> file);
    void WaitEmpty();
    SHARED_PTR_ALIA<File> InitNewFile(const std::string &jobCtxJson,
                                      const std::string &fileName);
    SHARED_PTR_ALIA<File> InitNewCustomFile(const std::string &jobCtxJson,
                                            const std::string &fileName,
                                            const size_t chunkSize);
    int SaveFileData(const std::string &jobCtxJson, const struct data_chunk &data);
    void CloseFileFds();
    int32_t OpenWriteFile(const std::string &fileName);
    int32_t GetFileFd(const std::string &fileName);
    void SetFileFd(const std::string &fileName, int32_t fd);
    int SendData(CONST_CHAR_PTR buffer, int size);
    void ExecuteStreamMode(SHARED_PTR_ALIA<File> file);
    void ExecuteFileMode(SHARED_PTR_ALIA<File> file);
    SHARED_PTR_ALIA<std::string> EncodeData(SHARED_PTR_ALIA<File> file);
    int DoSaveFileData(const std::string &jobCtxJson, const std::string &fileName,
                       SHARED_PTR_ALIA<File> &file, const struct data_chunk &data);

private:
    Sender &operator=(const Sender &sender);
    Sender(const Sender &sender);

private:
    SHARED_PTR_ALIA<ITransport> transport_;
    std::string engineName_;
    SHARED_PTR_ALIA<analysis::dvvp::common::memory::ChunkPool> chunkPool_;
    volatile bool isInited_;
    volatile bool isFinished_;
    size_t hashId_;
    std::mutex fileFdLock_;
    std::map<std::string, int32_t> fileFdMap_;
    SHARED_PTR_ALIA<FileQueue> fileQueue_;
    std::mutex fileMapLock_;
    std::map<std::string, SHARED_PTR_ALIA<File>> fileMap_;
    std::mutex fileMutexMapLock_;
    std::map<std::string, SHARED_PTR_ALIA<std::mutex>> fileMutexMap_;
    std::condition_variable cv_;
    std::mutex cvLock_;
};

class SenderPool : public analysis::dvvp::common::singleton::Singleton<SenderPool> {
public:
    SenderPool();
    virtual ~SenderPool();

public:
    int Init();
    void Uninit();

public:
    int Dispatch(SHARED_PTR_ALIA<Sender> sender);

private:
    volatile bool inited_;
    SHARED_PTR_ALIA<analysis::dvvp::common::thread::ThreadPool> senderPool_;
    std::mutex mtx_;
};
}  // namespace client
}  // namespace streamio
}  // namespace dvvp
}  // namespace analysis

#endif