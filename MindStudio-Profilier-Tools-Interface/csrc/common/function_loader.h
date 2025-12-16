/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
*/

#ifndef MSPTI_COMMON_FUNCTION_LOADER_H
#define MSPTI_COMMON_FUNCTION_LOADER_H

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace Mspti {
namespace Common {
using LibraryHandle = void*;
using FunctionHandle = void*;

class FunctionLoader {
public:
    explicit FunctionLoader(const std::string& soName);
    ~FunctionLoader();
    void Set(const std::string& funcName);
    FunctionHandle Get(const std::string& funcName);
    std::string CanonicalSoPath();

private:
    std::string soName_;
    LibraryHandle handle_{nullptr};
    mutable std::unordered_map<std::string, FunctionHandle> registry_;
};

class FunctionRegister {
public:
    static FunctionRegister *GetInstance();
    FunctionHandle Get(const std::string& soName, const std::string& funcName);
    void RegisterFunction(const std::string& soName, const std::string& funcName);

private:
    FunctionRegister() = default;
    ~FunctionRegister() = default;
    explicit FunctionRegister(const FunctionRegister &obj) = delete;
    FunctionRegister& operator=(const FunctionRegister &obj) = delete;
    explicit FunctionRegister(FunctionRegister &&obj) = delete;
    FunctionRegister& operator=(FunctionRegister &&obj) = delete;

private:
    mutable std::mutex mu_;
    mutable std::unordered_map<std::string, std::unique_ptr<FunctionLoader>> registry_;
};

template<typename T>
struct FuncTraits {
};

template<typename R, typename... Args>
struct FuncTraits<std::function<R(Args...)>> {
    using FuncPtr = R(*)(Args...);
};

template<typename Func>
void GetFunction(const std::string& soName, const std::string& funcName, Func& func)
{
    auto funcPtr = FunctionRegister::GetInstance()->Get(soName, funcName);
    using FuncPtr = typename FuncTraits<std::remove_reference_t<Func>>::FuncPtr;
    func = reinterpret_cast<FuncPtr>(funcPtr);
}

void* RegisterFunction(const std::string& soName, const std::string& funcName);
}  // Common
}  // Mspti

#endif
