/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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

#ifndef AICPU_FACTORY_H_
#define AICPU_FACTORY_H_

#include <functional>
#include <map>
#include <memory>
#include <string>

namespace aicpu {
/**
 * Template Factory Class
 * @ClassName Factory
 * @Description Automatic registration factory implement, Store a Map which key
 * is KernelLib Name e.g. CCEKernel TFKernel and value is pointer to a instance
 * of KernelLib. when a KernelLib defined, register a instance to the map
 */
template <typename T>
class Factory {
 public:
  using FactoryType = std::shared_ptr<T>;

  ~Factory() = default;

  template <typename N>
  struct Register {
    Register(const std::string &key) {
      Factory::Instance()->creators_[key] = [] { return N::Instance(); };
    }
  };

  static Factory<T> *Instance() {
    static Factory<T> factory;
    return &factory;
  }

  std::shared_ptr<T> Create(const std::string &key) {
    std::shared_ptr<T> re = nullptr;
    auto iter = creators_.find(key);
    if (iter != creators_.end()) {
      re = (iter->second)();
    }
    return re;
  }

  static std::shared_ptr<T> Produce(const std::string &key) {
    return Factory::Instance()->Create(key);
  }

 private:
  // Contructior
  Factory() = default;
  // Copy prohibit
  Factory(const Factory &) = delete;
  // Move prohibit
  Factory(Factory &&) = delete;
  // Copy prohibit
  Factory &operator=(const Factory &) = delete;
  // Move prohibit
  Factory &operator=(const Factory &&) = delete;
  std::map<std::string, std::function<std::shared_ptr<T>()>> creators_;
};
}  // namespace aicpu
#endif  // AICPU_FACTORY_H_
