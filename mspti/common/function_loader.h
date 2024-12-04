/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : function_loader.h
 * Description        : Load function from dynamic library.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
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
    void RegisteFunction(const std::string& soName, const std::string& funcName);

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

template <typename R, typename... Types>
void GetFunction(const std::string& soName, const std::string& funcName, std::function<R(Types...)>& func)
{
    auto func_ptr = FunctionRegister::GetInstance()->Get(soName, funcName);
    func = (R(*)(Types...))func_ptr;
}

void* RegisterFunction(const std::string& soName, const std::string& funcName);
}  // Common
}  // Mspti

#endif
