/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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

#ifndef AIR_CXX_TESTS_FRAMEWORK_GE_RUNNING_ENV_INCLUDE_GE_RUNNING_ENV_PATH_UTILS_H_
#define AIR_CXX_TESTS_FRAMEWORK_GE_RUNNING_ENV_INCLUDE_GE_RUNNING_ENV_PATH_UTILS_H_
#include "fake_ns.h"
#include <string>
#include <sstream>
#include <fstream>
#include "mmpa/mmpa_api.h"

FAKE_NS_BEGIN
inline bool IsDir(const char *path) {
  return mmIsDir(path) == EN_OK;
}
inline bool IsFile(const char *path) {
  std::ifstream f(path);
  return f.good();
}
inline void RemoveFile(const char *path) {
  remove(path);
}
inline std::string BaseName(const char *path) {
  std::stringstream ss;
  for (size_t i = strlen(path); i > 0; --i) {
    if (path[i-1] == '/') {
      break;
    } else {
      ss << path[i-1];
    }
  }
  auto reverse_name = ss.str();
  return std::string{reverse_name.rbegin(), reverse_name.rend()};
}
inline std::string DirName(const char *path) {
  size_t i;
  for (i = strlen(path); i > 0; --i) {
    if (path[i-1] == '/') {
      break;
    }
  }
  return {path, i};
}
inline std::string PathJoin(const char *path1, const char *path2) {
  std::stringstream ss;
  ss << path1 << '/' << path2;
  return ss.str();
}
inline int Mkdir(const char *path) {
  if (!IsDir(path)) {
    auto ret = mmMkdir(path,
                       S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH); // 775
    if (ret != EN_OK) {
      return -1;
    }
  }
  return 0;
}
inline const std::string &GetRunPath() {
  static std::string path;
  if (!path.empty()) {
    return path;
  }
  char buf[2048] = {0};
  if (readlink("/proc/self/exe", buf, sizeof(buf) - 1) < 0) {
    return path;
  }
  path = PathJoin(DirName(buf).c_str(), "st_run_data");
  if (Mkdir(path.c_str()) != 0) {
    path = "";
  }
  return path;
}

const std::string GetAirPath();

FAKE_NS_END
#endif //AIR_CXX_TESTS_FRAMEWORK_GE_RUNNING_ENV_INCLUDE_GE_RUNNING_ENV_PATH_UTILS_H_
