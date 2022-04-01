/**
 * Copyright 2022-2023 Huawei Technologies Co., Ltd
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

#ifndef FFTS_ENGINE_COMMON_PLUGIN_MANAGER_H_
#define FFTS_ENGINE_COMMON_PLUGIN_MANAGER_H_
#include <dlfcn.h>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include "graph_optimizer/graph_optimize_register_error_codes.h"
#include "inc/ffts_log.h"
#include "inc/ffts_error_codes.h"

namespace ffts {

using std::vector;
using std::string;
using std::function;

class PluginManager {
 public:
  explicit PluginManager(const string &name) : so_name(name) {}

  ~PluginManager() {}

  /*
  *  @ingroup ffts
  *  @brief   return tbe so name
  *  @return  tbe so name
  */
  const string GetSoName() const;

  /*
  *  @ingroup ffts
  *  @brief   initial resources needed by TbeCompilerAdapter,
  *  such as dlopen so files and load function symbols etc.
  *  @return  SUCCESS or FAILED
  */
  Status OpenPlugin(const string& path);

  /*
*  @ingroup ffts
*  @brief   release resources needed by TbeCompilerAdapter,
*  such as dlclose so files and free tbe resources etc.
*  @return  SUCCESS or FAILED
*/
  Status CloseHandle();

  /*
  *  @ingroup ffts
  *  @brief   load function symbols from so files
  *  @return  SUCCESS or FAILED
  */
  template <typename R, typename... Types>
  Status GetFunction(const string& func_name, function<R(Types... args)>& func) const {
    func = (R(*)(Types...))dlsym(handle, func_name.c_str());

    FFTS_CHECK(func == nullptr, FFTS_LOGW("Failed to get function %s in %s!", func_name.c_str(), so_name.c_str()),
             return FAILED);

    return SUCCESS;
  }

 private:
  string so_name;
  void* handle{nullptr};
};

}  // namespace ffts

#endif  // FFTS_ENGINE_COMMON_PLUGIN_MANAGER_H_
