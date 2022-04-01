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

#ifndef GE_COMMON_GE_GE_UTIL_H_
#define GE_COMMON_GE_GE_UTIL_H_

#include <iostream>
#include <memory>
#include <utility>

#define GE_DELETE_ASSIGN_AND_COPY(Classname)        \
  Classname &operator=(const Classname &) = delete; \
  Classname(const Classname &) = delete

namespace ge {
template <typename T, typename... Args>
static inline std::shared_ptr<T> MakeShared(Args &&... args) {
  typedef typename std::remove_const<T>::type T_nc;
  return std::shared_ptr<T>(new (std::nothrow) T_nc(std::forward<Args>(args)...));
}

template <typename T>
struct MakeUniq {
  typedef std::unique_ptr<T> unique_object;
};

template <typename T>
struct MakeUniq<T[]> {
  typedef std::unique_ptr<T[]> unique_array;
};

template <typename T, size_t B>
struct MakeUniq<T[B]> {
  struct invalid_type { };
};

template <typename T, typename... Args>
static inline typename MakeUniq<T>::unique_object MakeUnique(Args &&... args) {
  typedef typename std::remove_const<T>::type T_nc;
  return std::unique_ptr<T>(new (std::nothrow) T_nc(std::forward<Args>(args)...));
}

template <typename T>
static inline typename MakeUniq<T>::unique_array MakeUnique(const size_t num) {
  return std::unique_ptr<T>(new (std::nothrow) typename std::remove_extent<T>::type[num]());
}

template <typename T, typename... Args>
static inline typename MakeUniq<T>::invalid_type MakeUnique(Args &&...) = delete;
}  // namespace ge
#endif  // GE_COMMON_GE_GE_UTIL_H_
