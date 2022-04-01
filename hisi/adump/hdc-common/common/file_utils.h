/**
 * @file common/file_utils.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef ADX_COMMON_FILE_UTILS_H
#define ADX_COMMON_FILE_UTILS_H
#include <cstdint>
#include <string>
#include <vector>
#include "mmpa_api.h"
#include "ide_process_util.h"
#include "common/extra_config.h"
#include "ide_daemon_api.h"
#define FILE_MMCLOSE_AND_SET_INVALID(fd) do {                   \
    if ((fd) >= 0) {                                              \
        (void)mmClose(fd);                                      \
        fd = -1;                                                \
    }                                                           \
} while (0)

namespace Adx {
#if (OS_TYPE == LINUX)
constexpr char OS_SPLIT_CHAR                 = '/';
const std::string OS_SPLIT_STR        = "/";
#else
constexpr char OS_SPLIT_CHAR                 = '\\';
const std::string OS_SPLIT_STR        = "\\";
static const uint32_t WIN_PATH_MIN_LENGTH    = 2;
constexpr char COLON                         = ':';
#endif
constexpr uint32_t DEFAULT_PATH_MODE         = 0700;
constexpr uint32_t DISK_RESERVED_SPACE       = 1048576;        // disk reserved space 1Mb
using FileFilterFn                           = int (*)(const mmDirent *dir);
class FileUtils {
public:
    static bool IsFileExist(const std::string &path);
    static bool IsDirExist(const std::string &path);
    static IdeErrorT WriteFile(const std::string &fileName, IdeSendBuffT data, uint32_t len, int64_t offset);
    static IdeErrorT CreateDir(const std::string &path);
    static std::string GetFileDir(const std::string &path);
    static bool IsDiskFull(const std::string &path, uint64_t size);
    static bool IsDirectory(const std::string &path);
    static IdeErrorT GetFileName(const std::string &path, std::string &name);
    static bool CheckNonCrossPath(const std::string &path);
    static int32_t FilePathIsReal(const std::string &filePath, std::string &resultPath);
    static int32_t FileNameIsReal(const std::string &file, std::string &resultPath);
    static bool IsValidDirChar(const std::string &path);
    static bool StartsWith(const std::string &s, const std::string &sub);
    static bool EndsWith(const std::string &s, const std::string &sub);
    static bool GetDirFileList(const std::string &path, std::vector<std::string> &list, const FileFilterFn fileter,
        const std::string &fileNamePrefix, bool isRecursive);
    static IdeErrorT CopyFileAndRename(const std::string &src, const std::string &des);
    static std::string ReplaceAll(std::string &base, const std::string &src, const std::string &dst);
    static bool IsAbsolutePath(const std::string &path);
    static IdeErrorT AddMappingFileItem(const std::string &fileName, const std::string &hashValue);
};
}
#endif