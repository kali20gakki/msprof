/**
 * Copyright 2020 Huawei Technologies Co., Ltd

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 * http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include <stdio.h>
#include "register/ops_kernel_builder_registry.h"
#include "graph/debug/ge_log.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"

namespace ge {
OpsKernelBuilderRegistry::~OpsKernelBuilderRegistry() {
  for (auto &it : kernel_builders_) {
    printf("[WARNING] %s was not unregistered", it.first.c_str());
    // to avoid coredump when unregister is not called when so was close
    // this is called only when app is shutting down, so no release would be leaked
    new (std::nothrow) std::shared_ptr<OpsKernelBuilder>(it.second);
  }
}
void OpsKernelBuilderRegistry::Register(const string &lib_name, const OpsKernelBuilderPtr &instance) {
  auto it = kernel_builders_.emplace(lib_name, instance);
  if (it.second) {
    printf("[INFO] Done registering OpsKernelBuilder successfully, kernel lib name = %s", lib_name.c_str());
  } else {
    printf("[WARNING] OpsKernelBuilder already registered. kernel lib name = %s", lib_name.c_str());
  }
}

void OpsKernelBuilderRegistry::UnregisterAll() {
  kernel_builders_.clear();
  printf("[INFO] All builders are unregistered");
}

void OpsKernelBuilderRegistry::Unregister(const string &lib_name) {
  kernel_builders_.erase(lib_name);
  printf("[INFO] OpsKernelBuilder of %s is unregistered", lib_name.c_str());
}

const std::map<std::string, OpsKernelBuilderPtr> &OpsKernelBuilderRegistry::GetAll() const {
  return kernel_builders_;
}
OpsKernelBuilderRegistry &OpsKernelBuilderRegistry::GetInstance() {
  static OpsKernelBuilderRegistry instance;
  return instance;
}

OpsKernelBuilderRegistrar::OpsKernelBuilderRegistrar(const string &kernel_lib_name,
                                                     OpsKernelBuilderRegistrar::CreateFn fn)
    : kernel_lib_name_(kernel_lib_name) {
  printf("[INFO] To register OpsKernelBuilder, kernel lib name = %s", kernel_lib_name.c_str());
  std::shared_ptr<OpsKernelBuilder> builder;
  if (fn != nullptr) {
    builder.reset(fn());
    if (builder == nullptr) {
      printf("[ERROR] [Create][OpsKernelBuilder]kernel lib name = %s", kernel_lib_name.c_str());
    }
  } else {
    printf("[ERROR] [Check][Param:fn]Creator is nullptr, kernel lib name = %s", kernel_lib_name.c_str());
  }

  // May add empty ptr, so that error can be found afterward
  OpsKernelBuilderRegistry::GetInstance().Register(kernel_lib_name, builder);
}

OpsKernelBuilderRegistrar::~OpsKernelBuilderRegistrar() {
  printf("[INFO] OpsKernelBuilderRegistrar destroyed. KernelLibName = %s", kernel_lib_name_.c_str());
  OpsKernelBuilderRegistry::GetInstance().Unregister(kernel_lib_name_);
}

Status SetStreamLabel(const ge::NodePtr &node, const std::string &label)
{
  if (node == nullptr) {
    printf("[INFO]node is nullptr.\r\n");
    return FAILED;
  }
  const OpDescPtr tmp_desc = node->GetOpDesc();
  if (tmp_desc == nullptr) {
    printf("[INFO]tmp_desc is nullptr.\r\n");
    return FAILED;
  }

  if (!AttrUtils::SetStr(tmp_desc, ge::ATTR_NAME_STREAM_LABEL, label)) {
    printf("[INFO]Set Attr:%s fail for op:%s(%s)\r\n", ATTR_NAME_STREAM_LABEL.c_str(),
                       node->GetName().c_str(), node->GetType().c_str());
    return FAILED;
  }
  return SUCCESS;
}
}  // namespace ge
