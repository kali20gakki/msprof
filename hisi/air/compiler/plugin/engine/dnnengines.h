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

#ifndef GE_PLUGIN_ENGINE_DNNENGINES_H_
#define GE_PLUGIN_ENGINE_DNNENGINES_H_

#include <map>
#include <memory>
#include <string>

#include "framework/engine/dnnengine.h"
#include "plugin/engine/engine_manage.h"

namespace ge {
class GE_FUNC_VISIBILITY AICoreDNNEngine : public DNNEngine {
 public:
  explicit AICoreDNNEngine(const std::string &engine_name);
  explicit AICoreDNNEngine(const DNNEngineAttribute &attrs) : DNNEngine(attrs) {}
  ~AICoreDNNEngine() override = default;
};

class GE_FUNC_VISIBILITY VectorCoreDNNEngine : public DNNEngine {
 public:
  explicit VectorCoreDNNEngine(const std::string &engine_name);
  explicit VectorCoreDNNEngine(const DNNEngineAttribute &attrs) : DNNEngine(attrs) {}
  ~VectorCoreDNNEngine() override = default;
};


class GE_FUNC_VISIBILITY AICpuDNNEngine : public DNNEngine {
 public:
  explicit AICpuDNNEngine(const std::string &engine_name);
  explicit AICpuDNNEngine(const DNNEngineAttribute &attrs) : DNNEngine(attrs) {}
  ~AICpuDNNEngine() override = default;
};

class GE_FUNC_VISIBILITY AICpuTFDNNEngine : public DNNEngine {
 public:
  explicit AICpuTFDNNEngine(const std::string &engine_name);
  explicit AICpuTFDNNEngine(const DNNEngineAttribute &attrs) : DNNEngine(attrs) {}
  ~AICpuTFDNNEngine() override = default;
};

class GE_FUNC_VISIBILITY GeLocalDNNEngine : public DNNEngine {
 public:
  explicit GeLocalDNNEngine(const std::string &engine_name);
  explicit GeLocalDNNEngine(const DNNEngineAttribute &attrs) : DNNEngine(attrs) {}
  ~GeLocalDNNEngine() override = default;
};

class GE_FUNC_VISIBILITY HostCpuDNNEngine : public DNNEngine {
public:
  explicit HostCpuDNNEngine(const std::string &engine_name);
  explicit HostCpuDNNEngine(const DNNEngineAttribute &attrs) : DNNEngine(attrs) {}
  ~HostCpuDNNEngine() override = default;
};

class GE_FUNC_VISIBILITY RtsDNNEngine : public DNNEngine {
 public:
  explicit RtsDNNEngine(const std::string &engine_name);
  explicit RtsDNNEngine(const DNNEngineAttribute &attrs) : DNNEngine(attrs) {}
  ~RtsDNNEngine() override = default;
};

class GE_FUNC_VISIBILITY HcclDNNEngine : public DNNEngine {
 public:
  explicit HcclDNNEngine(const std::string &engine_name);
  explicit HcclDNNEngine(const DNNEngineAttribute &attrs) : DNNEngine(attrs) {}
  ~HcclDNNEngine() override = default;
};

class GE_FUNC_VISIBILITY FftsPlusDNNEngine : public DNNEngine {
 public:
  explicit FftsPlusDNNEngine(const std::string &engine_name);
  explicit FftsPlusDNNEngine(const DNNEngineAttribute &attrs) : DNNEngine(attrs) {}
  ~FftsPlusDNNEngine() override = default;
};

class GE_FUNC_VISIBILITY DSADNNEngine : public DNNEngine {
 public:
  explicit DSADNNEngine(const std::string &engine_name);
  explicit DSADNNEngine(const DNNEngineAttribute &attrs) : DNNEngine(attrs) {}
  ~DSADNNEngine() override = default;
};
}  // namespace ge
#endif  // GE_PLUGIN_ENGINE_DNNENGINES_H_
