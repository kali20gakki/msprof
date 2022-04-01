/**
 * @file file_utils.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "file_utils.h"
#include <cctype>
#include "log/adx_log.h"
#include "memory_utils.h"
namespace Adx {
static const std::string MAPPING_FILE_NAME   = "mapping.csv";

/**
 * @brief Write data to file
 * @param [in] fileName: File path
 * @param [in] data    : Data to be written
 * @param [in] len     : The length of data that needs to be written
 * @param [in] offset  : File offset
 * @return
 *        IDE_DAEMON_NONE_ERROR:   Write data to file success
 *        Other:                   failed, check for IdeErrorCode
 */
IdeErrorT FileUtils::WriteFile(const std::string &fileName, IdeSendBuffT data, uint32_t len, int64_t offset)
{
    IDE_CTRL_VALUE_FAILED(!fileName.empty(), return IDE_DAEMON_INVALID_PARAM_ERROR, "fileName is nullptr");
    IDE_CTRL_VALUE_FAILED(data != nullptr, return IDE_DAEMON_INVALID_PARAM_ERROR, "data is nullptr");
    std::string wrFile = fileName;
    int fd = mmOpen2(wrFile.c_str(), O_APPEND | M_RDWR | M_CREAT, M_IREAD | M_IWRITE);
    if (fd < 0 && mmGetErrorCode() != ENAMETOOLONG) {
        char errBuf[MAX_ERRSTR_LEN + 1] = {0};
        IDE_LOGE("Open file %s failed , exception %s", fileName.c_str(),
                 mmGetErrorFormatMessage(mmGetErrorCode(), errBuf, MAX_ERRSTR_LEN));
        return IDE_DAEMON_INVALID_PATH_ERROR;
    } else if (fd < 0) {
        std::string wrPath = GetFileDir(wrFile);
        IDE_CTRL_VALUE_FAILED(!wrPath.empty(), return IDE_DAEMON_INVALID_PATH_ERROR, "file path error");

        std::string hashValue = std::to_string(std::hash<std::string>{}(fileName));
        if (AddMappingFileItem(fileName, hashValue) != IDE_DAEMON_NONE_ERROR) {
            IDE_LOGE("add mapping item [ %s ] failed", hashValue.c_str());
        }
        wrFile = wrPath + OS_SPLIT_STR + hashValue;
        IDE_LOGW("file name [ %s ] too long rename as [ %s ] and the mapping record in mapping.csv",
            fileName.c_str(), wrFile.c_str());
        fd = mmOpen2(wrFile.c_str(), O_APPEND | M_RDWR | M_CREAT, M_IREAD | M_IWRITE);
        IDE_CTRL_VALUE_FAILED(fd >= 0, return IDE_DAEMON_INVALID_PATH_ERROR, "file path error");
    }

    if (offset != -1) {
        long err = mmLseek(fd, offset, SEEK_SET);
        if (err < 0) {
            IDE_LOGE("invalid param, fd %d, offset %ld", fd, offset);
            FILE_MMCLOSE_AND_SET_INVALID(fd);
            return IDE_DAEMON_UNKNOW_ERROR;
        }
    }

    IdeU8Pt wrData = static_cast<IdeU8Pt>(const_cast<IdeBuffT>(data));
    unsigned int reserve = len;
    do {
        mmSsize_t writeLen = mmWrite(fd, wrData + (len - reserve), reserve);
        if (writeLen < 0) {
            char errBuf[MAX_ERRSTR_LEN + 1] = {0};
            IDE_LOGE("Write failed, info : %s, writeLen: len: %u/%u",
                     mmGetErrorFormatMessage(mmGetErrorCode(), errBuf, MAX_ERRSTR_LEN), len - reserve, len);
            FILE_MMCLOSE_AND_SET_INVALID(fd);
            return IDE_DAEMON_NO_SPACE_ERROR;
        }
        reserve -= writeLen;
    } while (reserve != 0);

    FILE_MMCLOSE_AND_SET_INVALID(fd);
    return IDE_DAEMON_NONE_ERROR;
}

/**
 * @brief Add map item into mapping.csv
 * @param [in] filePath: file path
 * @param [in] hashValue: hash value of filePath
 * @return
 *        IDE_DAEMON_NONE_ERROR:    add mapping item success
 *        Other:                    failed, check for IdeErrorCode
 */
IdeErrorT FileUtils::AddMappingFileItem(const std::string &filePath, const std::string &hashValue)
{
    IDE_CTRL_VALUE_FAILED(!filePath.empty(), return IDE_DAEMON_INVALID_PARAM_ERROR, "filePath is nullptr");
    IDE_CTRL_VALUE_FAILED(!hashValue.empty(), return IDE_DAEMON_INVALID_PARAM_ERROR, "hashValue is null");

    std::string dir = GetFileDir(filePath);
    IDE_CTRL_VALUE_FAILED(!dir.empty(), return IDE_DAEMON_INVALID_PATH_ERROR, "file path error");
    std::string mappingFile = dir + OS_SPLIT_STR + MAPPING_FILE_NAME;
    int fd = mmOpen2(mappingFile.c_str(), O_APPEND | M_RDWR | M_CREAT, M_IRUSR | M_IWRITE);
    IDE_CTRL_VALUE_FAILED(fd >= 0, return IDE_DAEMON_INVALID_PATH_ERROR, "the path of mapping.csv error");

    long err = mmLseek(fd, 0, SEEK_END);
    if (err < 0) {
        IDE_LOGE("fd %d, set the current pos at the end of mapping.csv failed", fd);
        FILE_MMCLOSE_AND_SET_INVALID(fd);
        return IDE_DAEMON_UNKNOW_ERROR;
    }

    std::string filename;
    if (GetFileName(filePath, filename) != IDE_DAEMON_NONE_ERROR) {
        IDE_LOGE("invalid filepath [ %s ]", filePath.c_str());
        FILE_MMCLOSE_AND_SET_INVALID(fd);
        return IDE_DAEMON_INVALID_PATH_ERROR;
    }

    std::string mapItem = hashValue + ',' + filename + '\n';
    unsigned int mapItemDataLen = mapItem.length();
    unsigned int residLen = mapItemDataLen;
    do {
        mmSsize_t writeLen = mmWrite(fd, const_cast<IdeStringBuffer>(mapItem.c_str()) +
                                     (mapItemDataLen - residLen), residLen);
        if (writeLen < 0) {
            char errBuf[MAX_ERRSTR_LEN + 1] = {0};
            IDE_LOGE("Write failed, info : %s, writeLen: len: %u/%u",
                     mmGetErrorFormatMessage(mmGetErrorCode(), errBuf, MAX_ERRSTR_LEN),
                     mapItemDataLen - residLen, mapItemDataLen);
            FILE_MMCLOSE_AND_SET_INVALID(fd);
            return IDE_DAEMON_NO_SPACE_ERROR;
        }
        if (residLen >= writeLen) {
            residLen -= writeLen;
        } else {
            char errBuf[MAX_ERRSTR_LEN + 1] = {0};
            IDE_LOGE("Write failed, info : %s, writeLen larger than residLen",
                     mmGetErrorFormatMessage(mmGetErrorCode(), errBuf, MAX_ERRSTR_LEN));
            FILE_MMCLOSE_AND_SET_INVALID(fd);
            return IDE_DAEMON_UNKNOW_ERROR;
        }
    } while (residLen != 0);

    FILE_MMCLOSE_AND_SET_INVALID(fd);
    return IDE_DAEMON_NONE_ERROR;
}

/**
 * @brief Check file is exist
 * @param [in] path: file path
 * @return
 *        true:           file exists
 *        false:          file not exist
 */
bool FileUtils::IsFileExist(const std::string &path)
{
    if (path.empty()) {
        return false;
    }

    if (::mmAccess(path.c_str()) == EN_OK) {
        return true;
    }

    return false;
}

/**
 * @brief Check directory is exist
 * @param [in] path: file path
 * @return
 *        true: directory exists
 *        false: directory not exist
 */
bool FileUtils::IsDirExist(const std::string &path)
{
    if (path.empty()) {
        return false;
    }
#if(OS_TYPE != WIN)
    if (mmAccess2(path.c_str(), R_OK | W_OK | X_OK) != EN_OK) {
#else
    if (mmAccess2(path.c_str(), M_R_OK | M_W_OK) != EN_OK) {
#endif
        IDE_LOGW("check path: %s rwx permission failed!", path.c_str());
        return false;
    }

    return IsDirectory(path);
}

/**
 * @brief Create dir
 * @param [in] path: file path
 * @return
 *        IDE_DAEMON_NONE_ERROR:           Dump start Success
 *        IDE_DAEMON_INVALID_PATH_ERROR:   Invalid path
 *        IDE_DAEMON_MKDIR_ERROR:          Mkdir failed
 */
IdeErrorT FileUtils::CreateDir(const std::string &path)
{
    std::string curr = path;
    IdeErrorT ret = IDE_DAEMON_UNKNOW_ERROR;

    if (curr.empty()) {
        IDE_LOGE("create dir input empty");
        return IDE_DAEMON_INVALID_PATH_ERROR;
    }

    if (IsFileExist(curr)) {
        return IDE_DAEMON_NONE_ERROR;
    } else {
        std::string dir = GetFileDir(curr);
        IDE_CTRL_VALUE_FAILED(!dir.empty(), return ret, "Get file dir failed, ret: %d", ret);

        ret = CreateDir(dir);
        IDE_CTRL_VALUE_FAILED(ret == IDE_DAEMON_NONE_ERROR, return ret, "Create dir failed, ret: %d", ret);
    }

    if (!IsFileExist(path)) {
        if (mmMkdir(path.c_str(), (mmMode_t)DEFAULT_PATH_MODE) != EN_OK && errno != EEXIST) {
            char errBuf[MAX_ERRSTR_LEN + 1] = {0};
            IDE_LOGE("mkdir %s failed, errorstr: %s", path.c_str(),
                     mmGetErrorFormatMessage(mmGetErrorCode(), errBuf, MAX_ERRSTR_LEN));
            return IDE_DAEMON_MKDIR_ERROR;
        }
    }

    return IDE_DAEMON_NONE_ERROR;
}

/**
 * @brief Get the path part of the path
 * @param [in] path: file path
 * @param [out] dir:  the path part of path
 * @return
 *        IDE_DAEMON_NONE_ERROR:           Dump start Success
 *        IDE_DAEMON_INVALID_PATH_ERROR:   Invalid path
 */
std::string FileUtils::GetFileDir(const std::string &path)
{
    std::string dir;
    size_t pos = path.find_last_of(OS_SPLIT_CHAR);
    if (pos != std::string::npos) {
        dir = path.substr(0, pos);
        if (dir.empty()) {
            dir += OS_SPLIT_STR;
        }
        return dir;
    }

    return dir;
}

/**
 * @brief Check if the path has sufficient disk space
 * @param [in] path: file path
 * @param [in] size: The amount of disk space required
 * @return
 *        true:   the path has sufficient disk space
 *        false:  invalid path or the path does not have sufficient disk space
 */
bool FileUtils::IsDiskFull(const std::string &path, uint64_t size)
{
    IDE_CTRL_VALUE_FAILED(!path.empty(), return false, "path is empty");

    mmDiskSize diskSize;
    (void)memset_s(&diskSize, sizeof(diskSize), 0, sizeof(diskSize));
    int ret = mmGetDiskFreeSpace(path.c_str(), &diskSize);
    IDE_CTRL_VALUE_FAILED(ret == EN_OK, return true, "get disk free space fail");
    IDE_CTRL_VALUE_FAILED(diskSize.freeSize > DISK_RESERVED_SPACE, return true,
        "the %s more than disk reserved space(1Mb)", path.c_str());
    IDE_CTRL_VALUE_FAILED(size < diskSize.freeSize, return true, "the %s is full", path.c_str());
    return false;
}

/**
 * @brief Check if the path is a directory
 * @param [in] path: file path
 * @return
 *        true:   the path is a directory
 *        false:  the path is not a directory
 */
bool FileUtils::IsDirectory(const std::string &path)
{
    if (path.empty()) {
        IDE_LOGE("invalid parameter");
        return false;
    }

    int ret = mmIsDir(path.c_str());
    if (ret != EN_OK) {
        IDE_LOGI("%s is file", path.c_str());
        return false;
    }

    return true;
}

/**
 * @brief Get the name part of the path
 * @param [in] path: file path
 * @param [out] dir:  the name part of path
 * @return
 *        IDE_DAEMON_NONE_ERROR:           Dump start Success
 *        IDE_DAEMON_INVALID_PATH_ERROR:   Invalid path
 */
IdeErrorT FileUtils::GetFileName(const std::string &path, std::string &name)
{
    size_t pos = path.find_last_of(OS_SPLIT_CHAR);
    if (pos != std::string::npos) {
        name = path.substr(pos + 1);
        if (name.empty()) {
            return IDE_DAEMON_INVALID_PATH_ERROR;
        }
        return IDE_DAEMON_NONE_ERROR;
    }
    name = path;
    return IDE_DAEMON_NONE_ERROR;
}

/**
 * @brief Check file path exit cross path
 * @param [in] path: file path
 * @return
 *      true : file not exists cross path
 *     false : file exists cross path
 */
bool FileUtils::CheckNonCrossPath(const std::string &path)
{
    if (path.empty() || path.length() > MMPA_MAX_PATH) {
        return false;
    }

    size_t pos = path.find("..");
    if (pos == 0) {
        return false;
    }

    if (path.find("/..") != std::string::npos ||
        path.find("/\\.\\.") != std::string::npos) {
        return false;
    }

    return true;
}

/**
 * @brief jugde the file_path is realpath
 * @param [in] file_path: file path
 * @param [out] resolved_path: return path
 *
 * @return
 *        IDE_DAEMON_OK: succ
 *        IDE_DAEMON_ERROR: failed
 */
int32_t FileUtils::FilePathIsReal(const std::string &filePath, std::string &resultPath)
{
    if (filePath.empty()) {
        return IDE_DAEMON_ERROR;
    }

    IdeStringBuffer resolvedPath = reinterpret_cast<IdeStringBuffer>(IdeXmalloc(MMPA_MAX_PATH));
    IDE_CTRL_VALUE_FAILED(resolvedPath != nullptr, return IDE_DAEMON_ERROR, "malloc memory failed");

    int ret = mmRealPath(filePath.c_str(), resolvedPath, MMPA_MAX_PATH);
    if (ret != EN_OK) {
        IDE_XFREE_AND_SET_NULL(resolvedPath);
        return IDE_DAEMON_ERROR;
    }
    resultPath = resolvedPath;
    IDE_XFREE_AND_SET_NULL(resolvedPath);
    return IDE_DAEMON_OK;
}

/**
 * @brief jugde the file path with filename is realpath
 * @param file_path: file path
 * @param resolved_path: return path
 * @param path_len: the max length of resolved_path buffer
 *
 * @return
 *        IDE_DAEMON_OK: succ
 *        IDE_DAEMON_ERROR: failed
 */
int32_t FileUtils::FileNameIsReal(const std::string &file, std::string &resultPath)
{
    int ret = 0;
    if (file.empty()) {
        return IDE_DAEMON_ERROR;
    }

    size_t pos = file.find_last_of(OS_SPLIT_CHAR);
    if (pos != std::string::npos) {
        std::string path = file.substr(0, pos + 1);
        ret = FilePathIsReal(path, resultPath);
        if (ret != IDE_DAEMON_OK) {
            IDE_LOGE("path %s is not exist", file.c_str());
            return IDE_DAEMON_ERROR;
        }
    }

    IdeStringBuffer resolvedPath = reinterpret_cast<IdeStringBuffer>(IdeXmalloc(MMPA_MAX_PATH));
    IDE_CTRL_VALUE_FAILED(resolvedPath != nullptr, return IDE_DAEMON_ERROR, "malloc memory failed");

    ret = mmRealPath(file.c_str(), resolvedPath, MMPA_MAX_PATH);
    if (ret != EN_OK) {
        IDE_LOGD("path %s is not exist, use path: %s", file.c_str(), resolvedPath);
    }

    resultPath = resolvedPath;
    IDE_XFREE_AND_SET_NULL(resolvedPath);
    return IDE_DAEMON_OK;
}

/**
 * @brief Check if characters of the dir are all valid
 * @param [in] path: file path
 * @return
 *        true:   characters of the dir are all valid
 *        false:  characters of the dir are not all valid
 */
bool FileUtils::IsValidDirChar(const std::string &path)
{
    if (path.empty()) {
        IDE_LOGE("invalid parameter");
        return false;
    }

    const std::string pathWhiteList = "-=[];\\,./!@#$%^&*()_+{}:?";
    size_t len = path.length();
    for (size_t i = 0; i < len; i++) {
        if (!islower(path[i]) && !isupper(path[i]) && !isdigit(path[i]) &&
            pathWhiteList.find(path[i]) == std::string::npos) {
            IDE_LOGW("invalid path [%s] in char : [%c] at location %lu", path.c_str(), path[i], i);
            return false;
        }
    }

    return true;
}

/**
 * @brief sort dirent by alphabet order
 * @param [in]a : dirent a
 * @param [in]b : dirent b
 * @return
 *        0  : a=b
 *        >0 : a>b
 *        <0 : a<b
 */
static int32_t AlphaSort(const mmDirent **a, const mmDirent **b)
{
    // if first arg is null, return 'a' is greater than 'b'
    IDE_CTRL_VALUE_FAILED(a != nullptr, return 1, "Invalid ptr parameter");
    IDE_CTRL_VALUE_FAILED(*a != nullptr, return 1, "Invalid ptr parameter");
    // if second arg is null, return 'a' is less than 'b'
    IDE_CTRL_VALUE_FAILED(b != nullptr, return -1, "Invalid ptr parameter");
    IDE_CTRL_VALUE_FAILED(*b != nullptr, return -1, "Invalid ptr parameter");
    // sort by char
    return strcmp((*a)->d_name, (*b)->d_name);
}

bool FileUtils::StartsWith(const std::string &s, const std::string &sub)
{
    return s.find(sub) == 0;
}

bool FileUtils::EndsWith(const std::string &s, const std::string &sub)
{
    return s.rfind(sub) == (s.length()-sub.length());
}

/**
 * @brief get dir file list, if is subdir, recursivly get sub dir file list
 * @param [in]path : file path
 * @param [in]list : dir file list
 * @param [in]fileter : file filter method
 * @return
 *        EOK : SUCCESS
 *        other : FAILED
 */
bool FileUtils::GetDirFileList(const std::string &path, std::vector<std::string> &list, const FileFilterFn fileter,
    const std::string &fileNamePrefix, bool isRecursive)
{
    mmDirent **fileList = NULL;
    int32_t foundNum = mmScandir(path.c_str(), &fileList, fileter, AlphaSort);
    char errBuf[MAX_ERRSTR_LEN + 1] = {0};
    IDE_CTRL_VALUE_FAILED(foundNum >= 0, return false,
        "Scandir %s failed, errno=%s", path.c_str(), mmGetErrorFormatMessage(mmGetErrorCode(), errBuf, MAX_ERRSTR_LEN));
    IDE_CTRL_VALUE_FAILED(fileList != nullptr, return false, "Scandir failed, get null file list");

    for (int32_t i = 0; i < foundNum; i++) {
        if (fileList[i] == nullptr) {
            continue;
        }
        std::string filename = fileList[i]->d_name;
        if (filename == "." || filename == "..") {
            continue;
        }
        IDE_LOGD("fileList : %s", fileList[i]->d_name);
        unsigned char isFolder = 0x4;
        unsigned char isFile = 0x8;
        // if is sub dir, recursivly get sub dir file list
        if (fileList[i]->d_type == isFolder) {
            if (isRecursive) {
                IDE_LOGD("found sub dir: %s, recall get list", fileList[i]->d_name);
                std::string subFilePath = path + OS_SPLIT_STR + fileList[i]->d_name + OS_SPLIT_STR;
                GetDirFileList(subFilePath, list, nullptr, fileNamePrefix, isRecursive);
            }
        } else if (fileList[i]->d_type == isFile &&
            (fileNamePrefix.empty() || Adx::FileUtils::StartsWith(filename, fileNamePrefix)) &&
            !Adx::FileUtils::EndsWith(filename, "tmp")) {
            list.push_back(path + fileList[i]->d_name); // input base path ends with slash
        } else {
            IDE_LOGD("found neither folder nor file, skip : %s", fileList[i]->d_name);
        }
    }
    mmScandirFree(fileList, foundNum);
    return true;
}

/**
 * @brief : copy file and rename
 * @param [in] src : source file
 * @param [in] des : new file
 * @return
 *        zero : success
 *      others : failed
 */
IdeErrorT FileUtils::CopyFileAndRename(const std::string &src, const std::string &des)
{
    if (src.empty() || des.empty()) {
        return IDE_DAEMON_INVALID_PATH_ERROR;
    }

    std::string cmdCopyStr = "cp " + src + " " + des; // creare mv file
    IDE_LOGI("cp command : %s", cmdCopyStr.c_str());
    FILE *fstream = Popen(cmdCopyStr.c_str(), "r");
    IDE_CTRL_VALUE_FAILED(fstream != nullptr, return IDE_DAEMON_UNKNOW_ERROR, "Popen failed");
    int ret = Pclose(fstream);
    if (ret != 0) {
        IDE_LOGW("pclose exit failed");
    }
    return IDE_DAEMON_NONE_ERROR;
}

std::string FileUtils::ReplaceAll(std::string &base, const std::string &src, const std::string &dst)
{
    size_t pos = 0;
    std::string targetStr = dst;
    while ((pos = base.find(src, pos)) != std::string::npos)
    {
        base.replace(pos, src.size(), targetStr);
        pos += targetStr.size();
    }
    return base;
}

bool FileUtils::IsAbsolutePath(const std::string &path)
{
#if (OS_TYPE == LINUX)
    return path.front() == OS_SPLIT_CHAR;
#else
    if (path.length() < WIN_PATH_MIN_LENGTH) {
        return false;
    }
    return (path[1] == COLON) &&
        ((path[0] >= 'a' && path[0] <= 'z') ||
        (path[0] >= 'A' && path[0] <= 'Z'));
#endif
}
}