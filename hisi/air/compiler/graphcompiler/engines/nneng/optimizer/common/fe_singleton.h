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

#ifndef FUSION_ENGINE_OPTIMIZER_COMMON_FE_SINGLETON_H_
#define FUSION_ENGINE_OPTIMIZER_COMMON_FE_SINGLETON_H_
#include <mutex>

#define FE_DECLARE_SINGLETON(class_name)           \
 public:                                           \
  static class_name* GetInstance();                \
  /* only be called when system end */             \
  static void DestroyInstance();                   \
                                                   \
 protected:                                        \
  class_name();                                    \
  ~class_name();                                   \
  class_name(const class_name&);                   \
  const class_name& operator=(const class_name&);  \
                                                   \
 private:                                          \
  static void Init() {                             \
    mtx_.lock();                                   \
    if (instance_ == nullptr) {                    \
      instance_ = new (std::nothrow) class_name(); \
      if (instance_ == nullptr) {                  \
        mtx_.unlock();                             \
        return;                                    \
      }                                            \
    }                                              \
    mtx_.unlock();                                 \
  }                                                \
  static void restore() {                          \
    mtx_.lock();                                   \
    if (instance_ == nullptr) {                    \
      mtx_.unlock();                               \
      return;                                      \
    }                                              \
    delete instance_;                              \
    instance_ = nullptr;                           \
    mtx_.unlock();                                 \
  }                                                \
                                                   \
 private:                                          \
  static class_name* instance_;                    \
  static std::mutex mtx_;

#define FE_DEFINE_SINGLETON_EX(class_name)     \
  class_name* class_name::instance_ = nullptr; \
  std::mutex class_name::mtx_;                 \
  class_name* class_name::GetInstance() {      \
    if (instance_ == nullptr) {                \
      class_name::Init();                      \
    }                                          \
    return instance_;                          \
  }                                            \
  void class_name::DestroyInstance() { restore(); }

#define FE_DEFINE_SINGLETON(class_name) \
  FE_DEFINE_SINGLETON_EX(class_name)    \
  class_name::class_name() {}           \
  class_name::~class_name() {}

#endif  // FUSION_ENGINE_OPTIMIZER_COMMON_FE_SINGLETON_H_
