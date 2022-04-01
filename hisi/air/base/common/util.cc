/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "framework/common/util.h"

#include <sys/stat.h>
#ifdef __GNUC__
#include <regex.h>
#else
#include <regex>
#endif
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <ctime>
#include <fstream>

#include "common/util/error_manager/error_manager.h"
#include "external/ge/ge_api_error_codes.h"
#include "framework/common/debug/ge_log.h"
#include "framework/common/fmk_types.h"
#include "framework/common/ge_inner_error_codes.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "mmpa/mmpa_api.h"
#include "external/graph/types.h"
#include "graph/def_types.h"

namespace ge {
namespace {
using google::protobuf::io::CodedInputStream;
using google::protobuf::io::FileInputStream;
using google::protobuf::io::ZeroCopyInputStream;

/*
 * kProtoReadBytesLimit and kWarningThreshold are real arguments of CodedInputStream::SetTotalBytesLimit.
 * In order to prevent integer overflow and excessive memory allocation during protobuf processing,
 * it is necessary to limit the length of proto message (call SetTotalBytesLimit function).
 * In theory, the minimum message length that causes an integer overflow is 512MB, and the default is 64MB.
 * If the limit of warning_threshold is exceeded, the exception information will be printed in stderr.
 * If such an exception is encountered during operation,
 * the proto file can be divided into several small files or the limit value can be increased.
 */
const int32_t kFileSizeOutLimitedOrOpenFailed = -1;
const int32_t kProtoReadBytesLimit = INT_MAX;  // Max size of 2 GB minus 1 byte.

/// The maximum length of the file.
const uint32_t kMaxFileSizeLimit = UINT32_MAX;  // 4G for now
const int32_t kMaxBuffSize = 256;
const size_t kMaxErrorStrLength = 128U;
const char_t *const kPathValidReason = "The path can only contain 'a-z' 'A-Z' '0-9' '-' '.' '_' and chinese character";

void PathValidErrReport(const std::string &file_path, const std::string &atc_param, const std::string &reason) {
  if (!atc_param.empty()) {
    REPORT_INPUT_ERROR("E10001", std::vector<std::string>({"parameter", "value", "reason"}),
                       std::vector<std::string>({atc_param, file_path, reason}));
  } else {
    REPORT_INNER_ERROR("E19999", "Path[%s] invalid, reason:%s", file_path.c_str(), reason.c_str());
  }
}
}  // namespace

static bool ReadProtoFromCodedInputStream(CodedInputStream &coded_stream, google::protobuf::Message *const proto) {
  GE_CHK_BOOL_TRUE_EXEC_WITH_LOG(proto == nullptr, return false, "incorrect parameter. nullptr == proto");

  coded_stream.SetTotalBytesLimit(kProtoReadBytesLimit);
  return proto->ParseFromCodedStream(&coded_stream);
}

bool ReadProtoFromArray(const void *const data, const int32_t size, google::protobuf::Message *const proto) {
  GE_CHK_BOOL_TRUE_EXEC_WITH_LOG(((proto == nullptr) || (data == nullptr) || (size == 0)), return false,
                                 "incorrect parameter. proto is nullptr || data is nullptr || size is 0");

  google::protobuf::io::CodedInputStream coded_stream(PtrToPtr<void, uint8_t>(const_cast<void *const>(data)), size);
  return ReadProtoFromCodedInputStream(coded_stream, proto);
}

// Get file length
int64_t GetFileLength(const std::string &input_file) {
  GE_CHK_BOOL_TRUE_EXEC_WITH_LOG(input_file.empty(), return -1, "input_file path is null.");

  const std::string real_path = RealPath(input_file.c_str());

  GE_CHK_BOOL_TRUE_EXEC_WITH_LOG(real_path.empty(), return -1, "input_file path '%s' not valid", input_file.c_str());
  ULONGLONG file_length = 0U;
  if (mmGetFileSize(input_file.c_str(), &file_length) != EN_OK) {
    char_t err_buf[kMaxErrorStrLength + 1U] = {};
    const auto err_msg = mmGetErrorFormatMessage(mmGetErrorCode(), &err_buf[0], kMaxErrorStrLength);
    ErrorManager::GetInstance().ATCReportErrMessage("E19001", {"file", "errmsg"}, {input_file, err_msg});
    GELOGE(static_cast<uint32_t>(kFileSizeOutLimitedOrOpenFailed),
           "Open file[%s] failed. errmsg:%s", input_file.c_str(), err_msg);
  }
  GE_CHK_BOOL_TRUE_EXEC_WITH_LOG((file_length == 0U),
                                 REPORT_INNER_ERROR("E19999", "file:%s size is 0, not valid", input_file.c_str());
                                 return -1, "File[%s] size is 0, not valid.", input_file.c_str());

  GE_CHK_BOOL_TRUE_EXEC_WITH_LOG(
      file_length > kMaxFileSizeLimit,
      REPORT_INNER_ERROR("E19999", "file:%s size:%lld is out of limit: %d.", input_file.c_str(), file_length,
                         kMaxFileSizeLimit);
    return kFileSizeOutLimitedOrOpenFailed, "File[%s] size %lld is out of limit: %d.", input_file.c_str(), file_length,
           kMaxFileSizeLimit);
  return static_cast<int64_t>(file_length);
}

/** @ingroup domi_common
 *  @brief Read all data from binary file
 *  @param [in] file_name  File path
 *  @param [out] buffer  The address of the output memory, which needs to be released by the caller
 *  @param [out] length  Output memory size
 *  @return false fail
 *  @return true success
 */
bool ReadBytesFromBinaryFile(const char_t *const file_name, char_t **const buffer, int32_t &length) {
  GE_CHK_BOOL_TRUE_EXEC_WITH_LOG((file_name == nullptr), return false, "incorrect parameter. file is nullptr");
  GE_CHK_BOOL_TRUE_EXEC_WITH_LOG((buffer == nullptr), return false, "incorrect parameter. buffer is nullptr");

  const std::string real_path = RealPath(file_name);
  GE_CHK_BOOL_TRUE_EXEC_WITH_LOG(real_path.empty(), return false, "file path '%s' not valid", file_name);

  std::ifstream file(real_path.c_str(), std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    GELOGE(ge::FAILED, "[Read][File]Failed, file %s", file_name);
    REPORT_CALL_ERROR("E19999", "Read file %s failed", file_name);
    return false;
  }

  length = static_cast<int32_t>(file.tellg());

  GE_CHK_BOOL_TRUE_EXEC_WITH_LOG((length <= 0), file.close(); return false, "file length <= 0");

  (void)file.seekg(0, std::ios::beg);

  *buffer = new (std::nothrow) char[length]();
  GE_CHK_BOOL_TRUE_EXEC_RET_STATUS(*buffer == nullptr, false, file.close(), "new an object failed.");

  (void)file.read(*buffer, static_cast<int64_t>(length));
  file.close();
  return true;
}

std::string CurrentTimeInStr() {
  const std::time_t now = std::time(nullptr);
  const std::tm *const ptm = std::localtime(&now);
  if (ptm == nullptr) {
    char_t err_buf[kMaxErrorStrLength + 1U] = {};
    const auto err_msg = mmGetErrorFormatMessage(mmGetErrorCode(), &err_buf[0], kMaxErrorStrLength);
    GELOGE(ge::FAILED, "[Check][Param]Localtime incorrect, errmsg %s", err_msg);
    REPORT_CALL_ERROR("E19999", "Localtime incorrect, errmsg %s", err_msg);
    return "";
  }

  const int32_t kTimeBufferLen = 32;
  char_t buffer[kTimeBufferLen + 1] = {};
  // format: 20171122042550
  (void)std::strftime(&buffer[0], static_cast<size_t>(kTimeBufferLen), "%Y%m%d%H%M%S", ptm);
  return std::string(buffer);
}

bool ReadProtoFromText(const char_t *const file, google::protobuf::Message *const message) {
  GE_CHK_BOOL_TRUE_EXEC_WITH_LOG(((file == nullptr) || (message == nullptr)), return false,
                                 "incorrect parameter. nullptr == file || nullptr == message");

  const std::string real_path = RealPath(file);
  if (real_path.empty()) {
    char_t err_buf[kMaxErrorStrLength + 1U] = {};
    const auto err_msg = mmGetErrorFormatMessage(mmGetErrorCode(), &err_buf[0], kMaxErrorStrLength);
    ErrorManager::GetInstance().ATCReportErrMessage("E19000", {"path", "errmsg"}, {file, err_msg});
    GELOGE(FAILED, "[Do][RealPath]Path[%s]'s realpath is empty, errmsg[%s]", file, err_msg);
    return false;
  }

  GE_CHK_BOOL_TRUE_EXEC_WITH_LOG(GetFileLength(real_path) == -1, return false, "file size not valid.");

  std::ifstream fs(real_path.c_str(), std::ifstream::in);

  if (!fs.is_open()) {
    REPORT_INNER_ERROR("E19999", "open file:%s failed", real_path.c_str());
    GELOGE(ge::FAILED, "[Open][ProtoFile]Failed, real path %s, orginal file path %s",
           real_path.c_str(), file);
    return false;
  }

  google::protobuf::io::IstreamInputStream input(&fs);
  const bool ret = google::protobuf::TextFormat::Parse(&input, message);
  GE_IF_BOOL_EXEC(!ret, ErrorManager::GetInstance().ATCReportErrMessage("E19018", {"protofile"}, {file});
                  GELOGE(static_cast<uint32_t>(ret), "[Parse][File]Through"
                        "[google::protobuf::TextFormat::Parse] failed, file %s", file));
  fs.close();

  return ret;
}

uint64_t GetCurrentTimestamp() {
  mmTimeval tv{};
  const int32_t ret = mmGetTimeOfDay(&tv, nullptr);
  if (ret != EN_OK) {
    char_t err_buf[kMaxErrorStrLength + 1U] = {};
    const auto err_msg = mmGetErrorFormatMessage(mmGetErrorCode(), &err_buf[0], kMaxErrorStrLength);
    GELOGE(ge::FAILED, "Func gettimeofday may failed, ret:%d, errmsg:%s", ret, err_msg);
  }
  const auto total_use_time = tv.tv_usec + (tv.tv_sec * 1000000);  // 1000000: seconds to microseconds
  return static_cast<uint64_t>(total_use_time);
}

uint32_t GetCurrentSecondTimestap() {
  mmTimeval tv{};
  const int32_t ret = mmGetTimeOfDay(&tv, nullptr);
  if ((ret != 0)) {
    char_t err_buf[kMaxErrorStrLength + 1U] = {};
    const auto err_msg = mmGetErrorFormatMessage(mmGetErrorCode(), &err_buf[0], kMaxErrorStrLength);
    GELOGE(ge::FAILED, "Func gettimeofday may failed, ret:%d, errmsg:%s", ret, err_msg);
  }
  const auto total_use_time = tv.tv_sec;  // seconds
  return static_cast<uint32_t>(total_use_time);
}

bool CheckInt64MulOverflow(const int64_t a, const int64_t b) {
  if (a > 0) {
    if (b > 0) {
      if (a > (std::numeric_limits<int64_t>::max() / b)) {
        return false;
      }
    } else {
      if (b < (std::numeric_limits<int64_t>::min() / a)) {
        return false;
      }
    }
  } else {
    if (b > 0) {
      if (a < (std::numeric_limits<int64_t>::min() / b)) {
        return false;
      }
    } else {
      if ((a != 0) && (b < (std::numeric_limits<int64_t>::max() / a))) {
        return false;
      }
    }
  }
  return true;
}

bool CheckInputPathValid(const std::string &file_path, const std::string &atc_param) {
  // The specified path is empty
  if (file_path.empty()) {
    if (!atc_param.empty()) {
      REPORT_INPUT_ERROR("E10004", std::vector<std::string>({"parameter"}), std::vector<std::string>({atc_param}));
    } else {
      REPORT_INNER_ERROR("E19999", "Param file_path is empty, check invalid.");
    }
    GELOGW("Input parameter %s is empty.", file_path.c_str());
    return false;
  }
  const std::string real_path = RealPath(file_path.c_str());
  // Unable to get absolute path (does not exist or does not have permission to access)
  if (real_path.empty()) {
    char_t err_buf[kMaxErrorStrLength + 1U] = {};
    const auto err_msg = mmGetErrorFormatMessage(mmGetErrorCode(), &err_buf[0], kMaxErrorStrLength);
    const std::string reason = "realpath error, errmsg:" + std::string(err_msg);
    PathValidErrReport(file_path, atc_param, reason);
    GELOGW("Path[%s]'s realpath is empty, errmsg[%s]", file_path.c_str(), err_msg);
    return false;
  }

  // A regular matching expression to verify the validity of the input file path
  // Path section: Support upper and lower case letters, numbers dots(.) chinese and underscores
  // File name section: Support upper and lower case letters, numbers, underscores chinese and dots(.)
#ifdef __GNUC__
  const std::string mode = "^[\u4e00-\u9fa5A-Za-z0-9./_-]+$";
#else
  std::string mode = "^[a-zA-Z]:([\\\\/][^\\s\\\\/:*?<>\"|][^\\\\/:*?<>\"|]*)*([/\\\\][^\\s\\\\/:*?<>\"|])?$";
#endif

  GE_CHK_BOOL_TRUE_EXEC_WITH_LOG(
    !ValidateStr(real_path, mode),
    PathValidErrReport(file_path, atc_param, kPathValidReason);
    return false, "Invalid value for %s[%s], %s.", atc_param.c_str(), real_path.c_str(), kPathValidReason);

  // The absolute path points to a file that is not readable
  if (mmAccess2(real_path.c_str(), M_R_OK) != EN_OK) {
    char_t err_buf[kMaxErrorStrLength + 1U] = {};
    const auto err_msg = mmGetErrorFormatMessage(mmGetErrorCode(), &err_buf[0], kMaxErrorStrLength);
    PathValidErrReport(file_path, atc_param, "cat not access, errmsg:" + std::string(err_msg));
    GELOGW("Read file[%s] failed, errmsg[%s]", file_path.c_str(), err_msg);
    return false;
  }

  return true;
}

bool CheckOutputPathValid(const std::string &file_path, const std::string &atc_param) {
  // The specified path is empty
  if (file_path.empty()) {
    if (!atc_param.empty()) {
      REPORT_INPUT_ERROR("E10004", std::vector<std::string>({"parameter"}), std::vector<std::string>({atc_param}));
    } else {
      REPORT_INNER_ERROR("E19999", "Param file_path is empty, check invalid.");
    }
    ErrorManager::GetInstance().ATCReportErrMessage("E10004", {"parameter"}, {atc_param});
    GELOGW("Input parameter's value is empty.");
    return false;
  }

  GE_CHK_BOOL_TRUE_EXEC_WITH_LOG(file_path.length() >= static_cast<size_t>(MMPA_MAX_PATH),
                                 const std::string reason = "len is too long, it must be less than " +
                                                      std::to_string(MMPA_MAX_PATH);
                                 PathValidErrReport(file_path, atc_param, reason);
                                 return false, "Path[%s] len is too long, it must be less than %d", file_path.c_str(),
                                        MMPA_MAX_PATH);

  // A regular matching expression to verify the validity of the input file path
  // Path section: Support upper and lower case letters, numbers dots(.) chinese and underscores
  // File name section: Support upper and lower case letters, numbers, underscores chinese and dots(.)
#ifdef __GNUC__
  const std::string mode = "^[\u4e00-\u9fa5A-Za-z0-9./_-]+$";
#else
  std::string mode = "^[a-zA-Z]:([\\\\/][^\\s\\\\/:*?<>\"|][^\\\\/:*?<>\"|]*)*([/\\\\][^\\s\\\\/:*?<>\"|])?$";
#endif

  GE_CHK_BOOL_TRUE_EXEC_WITH_LOG(
    !ValidateStr(file_path, mode),
    PathValidErrReport(file_path, atc_param, kPathValidReason);
    return false, "Invalid value for %s[%s], %s.", atc_param.c_str(), file_path.c_str(), kPathValidReason);

  const std::string real_path = RealPath(file_path.c_str());
  // Can get absolute path (file exists)
  if (!real_path.empty()) {
    // File is not readable or writable
    if (mmAccess2(real_path.c_str(),
        static_cast<int32_t>(static_cast<uint32_t>(M_W_OK) | static_cast<uint32_t>(M_F_OK))) != EN_OK) {
      char_t err_buf[kMaxErrorStrLength + 1U] = {};
      const auto err_msg = mmGetErrorFormatMessage(mmGetErrorCode(), &err_buf[0], kMaxErrorStrLength);
      PathValidErrReport(file_path, atc_param, "cat not access, errmsg:" + std::string(err_msg));
      GELOGW("Write file[%s] failed, errmsg[%s]", real_path.c_str(), err_msg);
      return false;
    }
  } else {
    // Find the last separator
    int32_t path_split_pos = static_cast<int32_t>(file_path.size() - 1U);
    for (; path_split_pos >= 0; path_split_pos--) {
      if ((file_path[static_cast<uint64_t>(path_split_pos)] == '\\') ||
          (file_path[static_cast<uint64_t>(path_split_pos)] == '/')) {
        break;
      }
    }
    if (path_split_pos == 0) {
      return true;
    }
    if (path_split_pos != -1) {
      const std::string prefix_path = std::string(file_path).substr(0U, static_cast<size_t>(path_split_pos));
      // Determine whether the specified path is valid by creating the path
      if (CreateDirectory(prefix_path) != 0) {
        PathValidErrReport(file_path, atc_param, "Can not create directory");
        GELOGW("Can not create directory[%s].", file_path.c_str());
        return false;
      }
    }
  }

  return true;
}

FMK_FUNC_HOST_VISIBILITY bool ValidateStr(const std::string &file_path, const std::string &mode) {
#ifdef __GNUC__
  char_t ebuff[kMaxBuffSize];
  regex_t reg;
  const int32_t cflags = static_cast<int32_t>(static_cast<uint32_t>(REG_EXTENDED) | static_cast<uint32_t>(REG_NOSUB));
  int32_t ret = regcomp(&reg, mode.c_str(), cflags);
  if (static_cast<bool>(ret)) {
    (void)regerror(ret, &reg, &ebuff[0U], static_cast<size_t>(kMaxBuffSize));
    GELOGW("regcomp failed, reason: %s", &ebuff[0U]);
    regfree(&reg);
    return true;
  }

  ret = regexec(&reg, file_path.c_str(), 0U, nullptr, 0);
  if (static_cast<bool>(ret)) {
    (void)regerror(ret, &reg, &ebuff[0], static_cast<size_t>(kMaxBuffSize));
    GELOGE(ge::PARAM_INVALID, "[Rgexec][Param]Failed, reason %s", &ebuff[0]);
    REPORT_CALL_ERROR("E19999", "Rgexec failed, reason %s", &ebuff[0]);
    regfree(&reg);
    return false;
  }

  regfree(&reg);
  return true;
#else
  std::wstring wstr(file_path.begin(), file_path.end());
  std::wstring wmode(mode.begin(), mode.end());
  std::wsmatch match;
  bool res = false;

  try {
    std::wregex reg(wmode, std::regex::icase);
    // Matching std::string part
    res = regex_match(wstr, match, reg);
    res = regex_search(file_path, std::regex("[`!@#$%^&*()|{}';',<>?]"));
  } catch (std::exception &ex) {
    GELOGW("The directory %s is invalid, error: %s.", file_path.c_str(), ex.what());
    return false;
  }
  return !(res) && (file_path.size() == match.str().size());
#endif
}

Status ConvertToInt32(const std::string &str, int32_t &val) {
  try {
    val = std::stoi(str);
  } catch (std::invalid_argument &) {
    GELOGE(FAILED, "[Parse][Param]Failed, digit str:%s is invalid", str.c_str());
    REPORT_CALL_ERROR("E19999", "Parse param failed, digit str:%s is invalid", str.c_str());
    return FAILED;
  } catch (std::out_of_range &) {
    GELOGE(FAILED, "[Parse][Param]Failed, digit str:%s cannot change to int", str.c_str());
    REPORT_CALL_ERROR("E19999", "Parse param failed, digit str:%s cannot change to int", str.c_str());
    return FAILED;
  } catch (...) {
    GELOGE(FAILED, "[Parse][Param]Failed, digit str:%s cannot change to int", str.c_str());
    REPORT_CALL_ERROR("E19999", "Parse param failed, digit str:%s cannot change to int", str.c_str());
    return FAILED;
  }

  return SUCCESS;
}
}  //  namespace ge
