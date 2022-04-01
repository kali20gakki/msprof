/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#ifndef FUSION_ENGINE_OPTIMIZER_COMMON_PLUGIN_MANAGER_H_
#define FUSION_ENGINE_OPTIMIZER_COMMON_PLUGIN_MANAGER_H_
#include <dlfcn.h>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "graph_optimizer/graph_optimize_register_error_codes.h"
#include "common/fe_log.h"

namespace fe {

using std::vector;
using std::string;
using std::function;

class PluginManager {
 public:
  explicit PluginManager(const string &name) : so_name(name) {}

  ~PluginManager() {}

  /*
  *  @ingroup fe
  *  @brief   return tbe so name
  *  @return  tbe so name
  */
  const string GetSoName() const;

  /*
  *  @ingroup fe
  *  @brief   initial resources needed by TbeCompilerAdapter,
  *  such as dlopen so files and load function symbols etc.
  *  @return  SUCCESS or FAILED
  */
  Status OpenPlugin(const string& path);

  /*
*  @ingroup fe
*  @brief   release resources needed by TbeCompilerAdapter,
*  such as dlclose so files and free tbe resources etc.
*  @return  SUCCESS or FAILED
*/
  Status CloseHandle();

  /*
  *  @ingroup fe
  *  @brief   load function symbols from so files
  *  @return  SUCCESS or FAILED
  */
  template <typename R, typename... Types>
  Status GetFunction(const string& func_name, function<R(Types... args)>& func) const {
    func = (R(*)(Types...))dlsym(handle, func_name.c_str());

    FE_CHECK(func == nullptr, FE_LOGW("Failed to get function %s in %s!", func_name.c_str(), so_name.c_str()),
             return FAILED);

    return SUCCESS;
  }

  template <typename R, typename... Types>
  Status GetFunctionFromTbePlugin(const string& func_name, function<R(Types... args)>& func) {
    Status ret = GetFunction(func_name, func);
    if (ret != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][InitTbeFunc]: Failed to dlsym function %s.", func_name.c_str());
      // No matter we close it successfully or not, we will return FAILED because we failed to get fucntion.
      (void)CloseHandle();
      return FAILED;
    }
    return SUCCESS;
  }

 private:
  string so_name;
  void* handle{nullptr};
};

}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_COMMON_PLUGIN_MANAGER_H_
