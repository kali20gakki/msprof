/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Create: 2018-06-13
 */
#include "transport.h"
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <sys/stat.h>
#include "config/config.h"
#include "errno/error_code.h"

#include "message/prof_params.h"
#include "mmpa_api.h"
#include "msprof_dlog.h"
#include "securec.h"
#include "param_validation.h"

namespace analysis {
namespace dvvp {
namespace transport {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::common::validation;
using namespace analysis::dvvp::common::utils;
using namespace Collector::Dvvp::Plugin;
using namespace Collector::Dvvp::Mmpa;

int ITransport::SendFile(const std::string &jobCtx,
                         const std::string &file,
                         const std::string &relativePath,
                         const std::string &destDir /* = "" */)
{
    UNUSED(destDir);
    static const int FILE_BUF_LEN = 1 * 1024 * 1024; // 1 * 1024 * 1024 means 1mb

    SHARED_PTR_ALIA<char> buffer;
    MSVP_MAKE_SHARED_ARRAY_RET(buffer, char, FILE_BUF_LEN, PROFILING_FAILED);

    long long len = analysis::dvvp::common::utils::Utils::GetFileSize(file);
    if (len < 0 || len > MSVP_LARGE_FILE_MAX_LEN) {
        MSPROF_LOGE("Failed to get size of file: %s, err=%d", Utils::BaseName(file).c_str(), static_cast<int>(errno));
        return PROFILING_FAILED;
    }
    MSPROF_LOGI("size of file: %s is %lld", Utils::BaseName(file).c_str(), len);

    int32_t fd = MmOpen2(file, M_RDONLY, M_IRUSR | M_IWUSR);
    if (fd == PROFILING_FAILED || fd == PROFILING_INVALID_PARAM) {
        MSPROF_LOGE("Failed to open: %s, err=%d", Utils::BaseName(file).c_str(), errno);
        return PROFILING_FAILED;
    }

    long long sizeRead = 0;
    long long offset = 0;
    int ret = PROFILING_SUCCESS;
    do {
        sizeRead = (long long)MmRead(fd, buffer.get(), FILE_BUF_LEN);
        if (sizeRead > 0) {
            FileChunk chunk;
            chunk.relativeFileName = relativePath;
            chunk.dataBuf = (UNSIGNED_CHAR_PTR)buffer.get();
            chunk.bufLen = static_cast<unsigned int>(sizeRead);
            chunk.offset = offset;
            chunk.isLastChunk = ((offset + sizeRead < len) ? false : true);
            ret = SendFileChunk(jobCtx, chunk);
            if (ret != PROFILING_SUCCESS) {
                MSPROF_LOGE("Failed to send file: %s, chunk offset: %lld",
                    Utils::BaseName(file).c_str(), offset);
                break;
            }
            offset += sizeRead;
        }
    } while (sizeRead > 0);
    MmClose(fd);

    return ret;
}

int ITransport::SendFiles(const std::string &jobId,
                          const std::string &rootDir,
                          const std::string &destDir /* = "" */)
{
    std::vector<std::string> files;
    analysis::dvvp::common::utils::Utils::GetFiles(rootDir, true, files);
    for (size_t ii = 0; ii < files.size(); ++ii) {
        auto file = files[ii];
        std::string relative_path;
        int ret = analysis::dvvp::common::utils::Utils::RelativePath(file,
                                                                     rootDir,
                                                                     relative_path);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to get relative path for: %s", Utils::BaseName(file).c_str());
            continue;
        }

        analysis::dvvp::message::JobContext jobCtx;
        jobCtx.job_id = jobId;
        ret = SendFile(jobCtx.ToString(), file, relative_path, destDir);
        MSPROF_LOGI("file:%s, relative_path:%s, destDir:%s, ret: %d", Utils::BaseName(file).c_str(),
            Utils::BaseName(relative_path).c_str(), Utils::BaseName(destDir).c_str(), ret);
    }

    return PROFILING_SUCCESS;
}

int ITransport::SendFileChunk(const std::string &jobCtx, FileChunk &chunk)
{
    return PROFILING_SUCCESS;
}

}  // namespace transport
}  // namespace dvvp
}  // namespace analysis
