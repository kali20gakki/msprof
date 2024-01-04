/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : file.cpp
 * Description        : 文件处理
 * Author             : msprof team
 * Creation Date      : 2023/11/13
 * *****************************************************************************
 */

#include "file.h"

#include <dirent.h>
#include <sys/stat.h>
#include "log.h"
#include "error_code.h"

namespace Analysis {
namespace Utils {

namespace {
const uint16_t MAX_PATH_SIZE = 1024;
const uint32_t MAX_SUB_FILES_SIZE = 100000;
constexpr int DIR_CHECK_MODE = R_OK | W_OK | X_OK; // rwx
const int MAX_DEPTH = 20;
}

bool File::Chmod(const std::string &path, const mode_t &mode)
{
    if (path.empty()) {
        ERROR("The file path is empty.");
        return false;
    }
    return chmod(path.c_str(), mode) == 0;
}

bool File::Access(const std::string &path, const int &mode)
{
    if (path.empty()) {
        ERROR("The file path is empty.");
        return false;
    }
    return access(path.c_str(), mode) == 0;
}

bool File::DeleteFile(const std::string &filePath)
{
    if (filePath.empty()) {
        ERROR("The file path is empty.");
        return false;
    }
    return unlink(filePath.c_str()) == 0;
}

bool File::Exist(const std::string &path)
{
    return Access(path, F_OK);
}

bool File::IsSoftLink(const std::string &path)
{
    if (path.empty()) {
        ERROR("The file path is empty.");
        return false;
    }
    struct stat fileStat;
    if (lstat(path.c_str(), &fileStat) != 0) {
        ERROR("The file lstat failed.");
        return false;
    }
    return S_ISLNK(fileStat.st_mode);
}

bool File::IsFile(const std::string &path)
{
    if (path.empty()) {
        ERROR("The file path is empty.");
        return false;
    }
    struct stat fileStat;
    if (stat(path.c_str(), &fileStat) != 0) {
        ERROR("The file stat failed.");
        return false;
    }
    return fileStat.st_mode & S_IFREG;
}

uint64_t File::Size(const std::string &filePath)
{
    if (filePath.empty()) {
        ERROR("The file path is empty.");
        return 0;
    }
    struct stat fileStat;
    if (lstat(filePath.c_str(), &fileStat) != 0) {
        ERROR("The file lstat failed.");
        return 0;
    }
    return fileStat.st_size;
}

bool File::CreateDir(const std::string& path, const mode_t &mode)
{
    if (path.empty()) {
        ERROR("Create directory path is empty.");
        return false;
    }
    if (File::Exist(path)) {
        if (IsSoftLink(path)) {
            ERROR("The path '%' is soft link.", path);
            return false;
        }
        if (!File::Access(path, DIR_CHECK_MODE)) {
            ERROR("The path '%' have no rwx access.", path);
            return false;
        }
        return true;
    }
    if (mkdir(path.c_str(), mode) != 0) {
        ERROR("Create directory '%' failed.", path);
        return false;
    }
    return true;
}

bool File::RemoveDir(const std::string &path, int depth)
{
    if (depth >= MAX_DEPTH) {
        ERROR("The maximum recursion depth is exceeded");
        return false;
    }
    if (!File::Check(path)) {
        ERROR("Remove dir failed, check path error.");
        return false;
    }
    DIR *dir = opendir(path.c_str()); // 打开目录
    if (dir == nullptr) { // 打开目录失败
        ERROR("Remove dir failed, open dir error.");
        return false;
    }
    const struct dirent *entry = nullptr;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) { // 跳过当前目录和父目录
            continue;
        }
        auto absPath = path + "/" + entry->d_name;
        struct stat st;
        if (lstat(absPath.c_str(), &st) == -1) { // 获取文件/文件夹的属性
            closedir(dir);
            ERROR("Remove dir failed, lstat error.");
            return false;
        }
        if (S_ISDIR(st.st_mode)) { // 如果是文件夹，递归删除
            if (!RemoveDir(absPath, depth + 1)) {
                closedir(dir);
                ERROR("Remove dir failed, rm child dir error.");
                return false;
            }
            rmdir(absPath.c_str()); // 删除文件夹
        } else { // 如果是文件，直接删除
            remove(absPath.c_str());
        }
    }
    // 关闭目录流
    closedir(dir);
    // 删除当前目录
    if (rmdir(path.c_str()) != 0) {
        ERROR("Remove dir failed.");
        return false;
    }
    return true;
}

std::string File::PathJoin(const std::vector<std::string>& paths)
{
    if (paths.empty()) {
        ERROR("The file stat failed.");
        return "";
    }
    std::stringstream ss;
    for (size_t i = 0; i < paths.size(); ++i) {
        if (i == 0) {
            ss << paths[i];
            continue;
        }
        ss << "/" << paths[i];
    }
    return ss.str();
}

std::vector<std::string> File::GetFilesWithPrefix(const std::string &path, const std::string &prefix)
{
    std::vector<std::string> res;
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        ERROR("Failed open directory: '%'", path);
        return res;
    }
    dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string fileName(entry->d_name);
        if (fileName.size() < prefix.size()) {
            continue;
        }
        if (fileName.substr(0, prefix.size()) == prefix) {  // 如果fileName是以prefix为前缀
            if (res.size() > MAX_SUB_FILES_SIZE) {
                closedir(dir);
                WARN("Find more than % sub files in '%' dir", MAX_SUB_FILES_SIZE, path);
                return res;
            }
            std::vector<std::string> paths;
            paths.emplace_back(path);
            paths.emplace_back(fileName);
            res.emplace_back(PathJoin(paths));
        }
    }
    closedir(dir);
    return res;
}

std::vector<std::string> File::FilterFileWithSuffix(const std::vector<std::string> &files, const std::string &suffix)
{
    if (suffix.empty()) {
        return files;
    }
    std::vector<std::string> res;
    for (const auto &file : files) {
        // 过滤掉以suffix为后缀的file
        if (file.size() >= suffix.size() && file.substr(file.size() - suffix.size(), file.size()) == suffix) {
            continue;
        }
        res.emplace_back(file);
    }
    return res;
}

bool File::Check(const std::string &path, uint64_t maxReadFileBytes)
{
    if (path.empty()) {
        ERROR("The file path is empty.");
        return false;
    }
    if (path.size() > MAX_PATH_SIZE) {
        ERROR("The length of file path '%' > %.", path, MAX_PATH_SIZE);
        return false;
    }
    if (IsSoftLink(path)) {
        ERROR("The path '%' is soft link.", path);
        return false;
    }
    if (File::Size(path) > maxReadFileBytes) {
        ERROR("The file '%' size > %.", path, maxReadFileBytes);
        return false;
    }
    return true;
}

void FileReader::Open(const std::string &path, const std::ios_base::openmode &mode)
{
    if (!Check(path)) {
        ERROR("The FileReader open check failed: '%'.", path);
        return;
    }
    if (IsOpen()) {
        ERROR("The FileReader is opened: '%'.", path);
        return;
    }
    inStream_.open(path, mode);
    if (inStream_.fail()) {
        ERROR("Open file '%' failed", path);
        return;
    }
    path_ = path;
}

int FileReader::ReadBinary(std::stringstream &ss)
{
    if (!IsOpen()) {
        ERROR("File '%' not open.", path_);
        return ANALYSIS_ERROR;
    }
    inStream_.seekg(0, std::ios::beg);
    ss << inStream_.rdbuf();
    return ANALYSIS_OK;
}

int FileReader::ReadText(std::vector<std::string> &text)
{
    if (!IsOpen()) {
        ERROR("File '%' not open.", path_);
        return ANALYSIS_ERROR;
    }
    std::string line;
    while (getline(inStream_, line)) {
        text.emplace_back(line);
    }
    return ANALYSIS_OK;
}

int FileReader::ReadJson(nlohmann::json &content)
{
    if (!IsOpen()) {
        ERROR("File '%' not open.", path_);
        return ANALYSIS_ERROR;
    }
    try {
        content = nlohmann::json::parse(inStream_);
    } catch (const nlohmann::json::parse_error &e) {
        ERROR("Read json failed: '%'", path_);
        ERROR(e.what());
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

bool FileReader::Check(const std::string &path, uint64_t maxReadFileBytes)
{
    if (!File::Check(path, maxReadFileBytes)) {
        ERROR("The FileReader check failed: '%'.", path);
        return false;
    }
    if (!File::IsFile(path)) {
        ERROR("The read path '%' is not a file.", path);
        return false;
    }
    if (!File::Access(path, R_OK)) {
        ERROR("The file '%' has no read permission.", path);
        return false;
    }
    return true;
}

bool FileReader::IsOpen() const
{
    return inStream_.is_open();
}

void FileReader::Close()
{
    if (IsOpen()) {
        inStream_.close();
    }
}

bool FileWriter::Check(const std::string &path, uint64_t maxReadFileBytes)
{
    if (!File::Check(path, maxReadFileBytes)) {
        ERROR("The FileWriter check failed: '%'.", path);
        return false;
    }
    if (!File::Exist(path)) {
        INFO("The file exists: %.", path);
        return true;
    }
    if (!File::IsFile(path)) {
        ERROR("The write path '%' is not a file.", path);
        return false;
    }
    if (!File::Access(path, W_OK)) {
        ERROR("The file '%' has no write permission.", path);
        return false;
    }
    return true;
}

void FileWriter::Open(const std::string &path, const std::ios_base::openmode &mode)
{
    if (!Check(path)) {
        ERROR("The FileWriter open check failed: '%'.", path);
        return;
    }
    if (IsOpen()) {
        ERROR("The FileReader is opened: '%'.", path);
        return;
    }
    outStream_.open(path, mode);
    if (outStream_.fail()) {
        ERROR("Open '%' failed", path);
        return;
    }
    File::Chmod(path, permission_);
    path_ = path;
}

void FileWriter::WriteText(const std::string& content)
{
    if (IsOpen()) {
        outStream_ << content;
        outStream_.flush();
    }
}

bool FileWriter::IsOpen() const
{
    return outStream_.is_open();
}

void FileWriter::Close()
{
    if (IsOpen()) {
        outStream_.close();
    }
}

std::vector<std::string> File::GetOriginData(const std::string &path,
                                             const std::vector<std::string> &prePatterns,
                                             const std::vector<std::string> &sufFilters)
{
    std::vector<std::string> files;
    for (const auto &prefix : prePatterns) {
        auto matchedFiles = File::GetFilesWithPrefix(path, prefix);
        for (const auto &filter : sufFilters) {
            matchedFiles = File::FilterFileWithSuffix(matchedFiles, filter);
        }

        std::sort(matchedFiles.begin(), matchedFiles.end());
        std::move(
            matchedFiles.begin(),
            matchedFiles.end(),
            back_inserter(files)
        );
    }
    return files;
}

}  // namespace Utils
}  // namespace Analysis
