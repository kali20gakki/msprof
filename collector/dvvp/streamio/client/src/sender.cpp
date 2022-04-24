/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: yutianqi
 * Create: 2018-06-13
 */
#include "sender.h"
#include "config/config.h"
#include "errno/error_code.h"
#include "message/codec.h"
#include "message/prof_params.h"
#include "securec.h"
#include "mmpa_plugin.h"
#include "proto/profiler.pb.h"

using namespace analysis::dvvp::proto;

namespace analysis {
namespace dvvp {
namespace streamio {
namespace client {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::common::config;
using namespace Analysis::Dvvp::Plugin;

Sender::Sender(SHARED_PTR_ALIA<ITransport> transport, const std::string &engineName,
               SHARED_PTR_ALIA<analysis::dvvp::common::memory::ChunkPool> chunkPool)
    : transport_(transport),
      engineName_(engineName), chunkPool_(chunkPool),
      isInited_(false), isFinished_(true)
{
    hashId_ = std::hash<std::string>()(engineName_);
}

Sender::~Sender()
{
    if (isInited_) {
        fileMap_.clear();
        fileMutexMap_.clear();
        WaitEmpty();
        CloseFileFds();

        isInited_ = false;
    }
}

int Sender::Init()
{
    int ret = PROFILING_FAILED;

    MSPROF_LOGI("sender Init.");
    do {
        MSVP_MAKE_SHARED1_BREAK(fileQueue_, FileQueue, MSVP_CLN_SENDER_QUEUE_CAPCITY);
        fileQueue_->SetQueueName("Sender");
        fileFdMap_.clear();
        fileMap_.clear();
        fileMutexMap_.clear();

        ret = PROFILING_SUCCESS;
        isInited_ = true;
    } while (0);

    MSPROF_LOGI("sender Init success.");

    return ret;
}

void Sender::Uninit()
{
    if (isInited_) {
        MSPROF_LOGI("[%s]sender Uninit begin.", engineName_.c_str());
        FlushFileCache();
        WaitEmpty();
        CloseFileFds();

        isInited_ = false;
        MSPROF_LOGI("[%s]sender Uninit end.", engineName_.c_str());
    }
}

void Sender::FlushFileCache()
{
    std::lock_guard<std::mutex> lk(fileMapLock_);

    for (auto iter = fileMap_.begin(); iter != fileMap_.end(); ++iter) {
        MSPROF_LOGD("FlushFileCache file:%s size:%u", iter->first.c_str(), iter->second->GetChunk()->GetUsedSize());
        DispatchFile(iter->second);
    }
    fileMap_.clear();
    fileMutexMap_.clear();
}

SHARED_PTR_ALIA<File> Sender::GetFirstFileFromCache()
{
    std::lock_guard<std::mutex> lk(fileMapLock_);

    auto iter = fileMap_.begin();
    if (iter != fileMap_.end()) {
        auto file = iter->second;
        fileMap_.erase(iter);

        return file;
    }

    return nullptr;
}

SHARED_PTR_ALIA<File> Sender::GetFileFromCache(const std::string &fileName)
{
    std::lock_guard<std::mutex> lk(fileMapLock_);

    auto iter = fileMap_.find(fileName);
    if (iter != fileMap_.end()) {
        auto file = iter->second;
        fileMap_.erase(iter);

        return file;
    }

    return nullptr;
}

void Sender::AddFileIntoCache(const std::string &fileName, SHARED_PTR_ALIA<File> file)
{
    if (file != nullptr) {
        std::lock_guard<std::mutex> lk(fileMapLock_);
        fileMap_[fileName] = file;
    }
}

SHARED_PTR_ALIA<std::mutex> Sender::GetMutexByFileName(const std::string &fileName)
{
    std::lock_guard<std::mutex> lk(fileMutexMapLock_);

    auto iter = fileMutexMap_.find(fileName);
    if (iter != fileMutexMap_.end()) {
        return iter->second;
    }

    SHARED_PTR_ALIA<std::mutex> fileNameLock;
    MSVP_MAKE_SHARED0_RET(fileNameLock, std::mutex, nullptr);
    fileMutexMap_[fileName] = fileNameLock;

    return fileNameLock;
}

void Sender::DispatchFile(SHARED_PTR_ALIA<File> file)
{
    if (file != nullptr) {
        fileQueue_->Push(file);
        isFinished_ = false;
        try {
            SenderPool::instance()->Dispatch(shared_from_this());
        } catch (std::bad_weak_ptr &e) {
            MSPROF_LOGE("DispatchFile error, because shared_from_this return nullptr.");
        }
    }
}

void Sender::WaitEmpty()
{
    static const int waitBuffEmptyTimeoutUs = 1000;
    std::unique_lock<std::mutex> lk(cvLock_);
    cv_.wait_for(lk, std::chrono::microseconds(waitBuffEmptyTimeoutUs), [=] { return isFinished_; });
}

SHARED_PTR_ALIA<File> Sender::InitNewFile(const std::string &jobCtxJson,
                                          const std::string &fileName)
{
    SHARED_PTR_ALIA<File> file = nullptr;
    SHARED_PTR_ALIA<analysis::dvvp::common::memory::Chunk> chunk = nullptr;
    int ret = PROFILING_FAILED;

    do {
        chunk = chunkPool_->TryAlloc();
        if (chunk == nullptr) {
            MSPROF_LOGE("[Sender::InitNewFile] TryAlloc failed, fileName:%s.", fileName.c_str());
            auto anyFile = GetFirstFileFromCache();
            if (anyFile != nullptr) {
                DispatchFile(anyFile);
            }

            chunk = chunkPool_->Alloc();
        }

        // chunk will be owned by file
        MSVP_MAKE_SHARED4_BREAK(file, File, chunkPool_, chunk, jobCtxJson, fileName);
        ret = file->Init();
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to Init new fileName:%s.", fileName.c_str());
            break;
        }
    } while (0);

    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("release chunk %s.", fileName.c_str());
        chunkPool_->Release(chunk);
    }

    return (ret == PROFILING_SUCCESS) ? file : nullptr;
}

SHARED_PTR_ALIA<File> Sender::InitNewCustomFile(const std::string &jobCtxJson,
                                                const std::string &fileName,
                                                const size_t chunkSize)
{
    SHARED_PTR_ALIA<analysis::dvvp::common::memory::Chunk> chunk = nullptr;
    MSVP_MAKE_SHARED1_RET(chunk, analysis::dvvp::common::memory::Chunk, chunkSize, nullptr);
    if (!chunk->Init()) {
        MSPROF_LOGE("Failed to Init new custom chunk, fileName:%s", fileName.c_str());
        return nullptr;
    }

    SHARED_PTR_ALIA<File> file = nullptr;
    MSVP_MAKE_SHARED4_RET(file, File, nullptr, chunk, jobCtxJson, fileName, nullptr);
    int ret = file->Init();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to Init new custom fileName:%s.", fileName.c_str());
        return nullptr;
    }

    return file;
}

int Sender::DoSaveFileData(const std::string &jobCtxJson, const std::string &fileName,
                           SHARED_PTR_ALIA<File> &file, const struct data_chunk &data)
{
    unsigned int remainingLen = data.bufLen;
    errno_t err = 0;
    if (file == nullptr) {
        return PROFILING_SUCCESS;
    }
    auto chunk = file->GetChunk();
    if (chunk == nullptr) {
        MSPROF_LOGE("DoSaveFileData Failed getchunk is null");
        return PROFILING_FAILED;
    }
    unsigned int chunkFreeSize = chunk->GetFreeSize();
    unsigned int chunkUsedSize = chunk->GetUsedSize();
    MSPROF_LOGD("chunk[%s] chunkUsedSize:%u chunkFreeSize:%u remainingLen:%u",
        fileName.c_str(),  chunkUsedSize, chunkFreeSize, remainingLen);
    if (data.isLastChunk == 1 || remainingLen > chunkFreeSize) {
        // send buffered data if used
        if (chunkUsedSize > 0) {
            DispatchFile(file);
        }
        // send this data
        file = InitNewCustomFile(jobCtxJson, fileName, data.bufLen);
        chunk = file->GetChunk();
        if (data.bufLen != 0) {
            err = memcpy_s(static_cast<void *>(const_cast<UNSIGNED_CHAR_PTR>(chunk->GetBuffer())),
                chunk->GetFreeSize(), data.dataBuf, data.bufLen);
            if (err != EOK) {
                MSPROF_LOGE("chunk[%s] memcpy_s failed, chunkFreeSize:%u, bufLen:%u, err:%u",
                            fileName.c_str(), chunk->GetFreeSize(), data.bufLen, err);
                return PROFILING_FAILED;
            }
        }
        chunk->SetUsedSize(data.bufLen);
        DispatchFile(file);
    } else {
        err = memcpy_s(static_cast<void *>(const_cast<UNSIGNED_CHAR_PTR>(chunk->GetBuffer() + chunkUsedSize)),
            chunkFreeSize, data.dataBuf, remainingLen);
        if (err != EOK) {
            MSPROF_LOGE("chunk[%s] memcpy_s failed, chunkUsedSize:%u remainingLen:%u",
                        fileName.c_str(), chunkUsedSize, remainingLen);
            return PROFILING_FAILED;
        }
        chunk->SetUsedSize(chunkUsedSize + remainingLen);
        AddFileIntoCache(fileName, file);
    }

    return PROFILING_SUCCESS;
}

int Sender::SaveFileData(const std::string &jobCtxJson, const struct data_chunk &data)
{
    // get file from cache, otherwise create a new file
    std::string fileName = data.relativeFileName;

    SHARED_PTR_ALIA<std::mutex> fileNameLock = GetMutexByFileName(fileName);
    std::lock_guard<std::mutex> lk(*fileNameLock);

    auto file = GetFileFromCache(fileName);
    if (file == nullptr) {
        file = InitNewFile(jobCtxJson, fileName);
    }

    if (DoSaveFileData(jobCtxJson, fileName, file, data) == PROFILING_FAILED) {
        return PROFILING_FAILED;
    }
    if (file == nullptr) {
        return PROFILING_FAILED;
    } else {
        return PROFILING_SUCCESS;
    }
}

void Sender::CloseFileFds()
{
    std::lock_guard<std::mutex> lk(fileFdLock_);

    int ret = EN_OK;

    for (auto iter = fileFdMap_.begin(); iter != fileFdMap_.end(); iter++) {
        ret = MmpaPlugin::instance()->MsprofMmClose(iter->second);
        if (ret != EN_OK) {
            MSPROF_LOGE("Failed to mmClose ret:%d fd:%d", ret, iter->second);
        }
        fileFdMap_[iter->first] = -1;
    }

    fileFdMap_.clear();
}

INT32 Sender::OpenWriteFile(const std::string &fileName)
{
    size_t index = fileName.find_last_of(MSVP_SLASH);
    std::string dir = fileName.substr(0, index == std::string::npos ? 0 : index);
    int ret = analysis::dvvp::common::utils::Utils::CreateDir(dir);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to create dir %s", Utils::BaseName(dir).c_str());
        analysis::dvvp::common::utils::Utils::PrintSysErrorMsg();
        return -1;
    }

    INT32 fd = MmpaPlugin::instance()->MsprofMmOpen2(fileName.c_str(), O_WRONLY | O_CREAT, M_IRUSR | M_IWUSR);
    if (fd < 0) {
        MSPROF_LOGE("open file %s failed.", fileName.c_str());
        return fd;
    }

    SetFileFd(fileName, fd);
    MSPROF_LOGI("open file %s success(fd=%d)", fileName.c_str(), fd);

    return fd;
}

INT32 Sender::GetFileFd(const std::string &fileName)
{
    std::lock_guard<std::mutex> lk(fileFdLock_);

    auto iter = fileFdMap_.find(fileName);
    if (iter != fileFdMap_.end()) {
        return iter->second;
    }

    return -1;
}

void Sender::SetFileFd(const std::string &fileName, INT32 fd)
{
    std::lock_guard<std::mutex> lk(fileFdLock_);

    fileFdMap_[fileName] = fd;
}

int Sender::Send(const std::string &jobCtxJson, const struct data_chunk &data)
{
    if (!isInited_) {
        return PROFILING_FAILED;
    }

    return SaveFileData(jobCtxJson, data);
}

void Sender::Flush()
{
    if (isInited_) {
        MSPROF_LOGD("[%s]sender Flush begin.", engineName_.c_str());
        FlushFileCache();
        WaitEmpty();
        CloseFileFds();
        MSPROF_LOGD("[%s]sender Flush end.", engineName_.c_str());
    }
}

int Sender::SendData(CONST_CHAR_PTR buffer, int size)
{
    int sentLen = 0;

    if (buffer != nullptr && size > 0 && transport_ != nullptr) {
        sentLen = transport_->SendBuffer(reinterpret_cast<const void *>(buffer), size);
        if (sentLen != size) {
            MSPROF_LOGE("Failed to SendData, data_len=%d, sent len=%d", size, sentLen);
        } else {
            MSPROF_LOGD("[%s]sended data %d", engineName_.c_str(), size);
        }
    }

    return sentLen;
}

SHARED_PTR_ALIA<std::string> Sender::EncodeData(SHARED_PTR_ALIA<File> file)
{
    SHARED_PTR_ALIA<FileChunkReq> fileChunk;
    MSVP_MAKE_SHARED0_RET(fileChunk, FileChunkReq, nullptr);

    if (file != nullptr) {
        auto chunk = file->GetChunk();

        // fill the jobCtxJson
        fileChunk->mutable_hdr()->set_job_ctx(file->GetJobCtx());
        fileChunk->set_filename(file->GetModule());
        fileChunk->set_offset(-1);
        fileChunk->set_chunksizeinbytes(chunk->GetUsedSize());
        fileChunk->set_datamodule(file->GetChunkDataModule());
        fileChunk->set_needack(false);
        if (chunk->GetUsedSize() == 0) {
            fileChunk->set_islastchunk(true);
        } else {
            fileChunk->set_islastchunk(false);
            fileChunk->set_chunk((UNSIGNED_CHAR_PTR)chunk->GetBuffer(), chunk->GetUsedSize());
            fileChunk->set_tag(file->GetTag());
            fileChunk->set_chunkstarttime(file->GetChunkStartTime());
            fileChunk->set_chunkendtime(file->GetChunkEndTime());
        }
    }

    return analysis::dvvp::message::EncodeMessageShared(fileChunk);
}

void Sender::ExecuteStreamMode(SHARED_PTR_ALIA<File> file)
{
    auto encode = EncodeData(file);
    int currentSize = encode->size();
    MSPROF_LOGD("[Sender::ExecuteStreamMode] fileName = %s", file->GetFileName().c_str());
    int ret = SendData(encode->c_str(), currentSize);
    if (ret < currentSize) {
        MSPROF_LOGE("[%s] send data size = %u", engineName_.c_str(), ret);
        return;
    }
}

void Sender::ExecuteFileMode(SHARED_PTR_ALIA<File> file)
{
    auto chunk = file->GetChunk();
    auto fileName = file->GetWriteFileName();

    INT32 fd = GetFileFd(fileName);
    if (fd < 0) {
        fd = OpenWriteFile(fileName);
        MSPROF_LOGI("%s is not exist, create file:%d", fileName.c_str(), static_cast<int>(fd));
        if (fd < 0) {
            MSPROF_LOGE("create %s failed fd[%d].", fileName.c_str(), static_cast<int>(fd));
            return;
        }
    }

    size_t size = chunk->GetUsedSize();
    INT32 ret = MmpaPlugin::instance()->MsprofMmWrite(fd, static_cast<void *>(const_cast<UNSIGNED_CHAR_PTR>(chunk->GetBuffer())), size);
    if (static_cast<size_t>(ret) != size) {
        MSPROF_LOGE("write[%d] fileName = %s data size = %u, written size = %d",
                    static_cast<int>(fd), fileName.c_str(),
                    size, ret);
    }
}

int Sender::Execute()
{
    static const int maxPopCount = 64;
    std::vector<SHARED_PTR_ALIA<File> > file(maxPopCount);

    file.clear();
    if (!fileQueue_->TryBatchPop(maxPopCount, file)) {
        return PROFILING_FAILED;
    }

    size_t count = file.size();

    for (unsigned int i = 0; i < count; ++i) {
        if (file[i]->IsStreamMode()) {
            ExecuteStreamMode(file[i]);
        } else {
            ExecuteFileMode(file[i]);
        }
        file[i]->Uinit();
    }
    MSPROF_LOGD("[Sender::Execute] Execute success, count:%lu", count);

    std::lock_guard<std::mutex> lk(cvLock_);
    if (fileQueue_->size() == 0) {
        isFinished_ = true;
        cv_.notify_all();
    }

    return PROFILING_SUCCESS;
}

size_t Sender::HashId()
{
    return hashId_;
}

SenderPool::SenderPool()
    : inited_(false)
{
}

SenderPool::~SenderPool()
{
    Uninit();
}

int SenderPool::Init()
{
    int ret = PROFILING_FAILED;

    do {
        std::lock_guard<std::mutex> lk(mtx_);
        if (inited_) {
            ret = PROFILING_SUCCESS;
            break;
        }

        MSPROF_LOGI("createing sender pool");
        MSVP_MAKE_SHARED2_BREAK(senderPool_, analysis::dvvp::common::thread::ThreadPool,
            analysis::dvvp::common::thread::LOAD_BALANCE_METHOD::ID_MOD, MSVP_CLN_SENDER_POOL_THREAD_NUM);

        senderPool_->SetThreadPoolNamePrefix(MSVP_SENDER_POOL_NAME_PREFIX);
        senderPool_->SetThreadPoolQueueSize(SENDERPOOL_THREAD_QUEUE_SIZE);
        ret = senderPool_->Start();
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to start sender pool, ret=%d", ret);
            break;
        }
        inited_ = true;
    } while (0);

    return ret;
}

void SenderPool::Uninit()
{
    std::lock_guard<std::mutex> lk(mtx_);
    if (inited_) {
        (void)senderPool_->Stop();
        inited_ = false;
    }
}

int SenderPool::Dispatch(SHARED_PTR_ALIA<Sender> sender)
{
    std::lock_guard<std::mutex> lk(mtx_);
    if (inited_) {
        senderPool_->Dispatch(sender);
        return PROFILING_SUCCESS;
    }

    return PROFILING_FAILED;
}
}  // namespace client
}  // namespace streamio
}  // namespace dvvp
}  // namespace analysis
