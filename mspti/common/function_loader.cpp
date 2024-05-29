/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : function_loader.cpp
 * Description        : Load function from dynamic library.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#include <dlfcn.h>

#include "common/function_loader.h"

namespace Mspti {
namespace Common {

FunctionLoader::FunctionLoader(const std::string& soName)
{
    soName_ = soName + ".so";
}

FunctionLoader::~FunctionLoader()
{
    if (handle_) {
        dlclose(handle_);
    }
}

void FunctionLoader::Set(const std::string& funcName)
{
    registry_[funcName] = nullptr;
}

void *FunctionLoader::Get(const std::string& funcName)
{
    if (!handle_) {
        auto handle = dlopen(soName_.c_str(), RTLD_LAZY);
        if (handle == nullptr) {
            printf("%s\n", dlerror());
            return nullptr;
        }
        handle_ = handle;
    }

    auto itr = registry_.find(funcName);
    if (itr == registry_.end()) {
        printf("function(\"%s\") is not registered.\n", funcName.c_str());
        return nullptr;
    }

    if (itr->second) {
        return itr->second;
    }

    auto func = dlsym(handle_, funcName.c_str());
    if (func == nullptr) {
        return nullptr;
    }
    registry_[funcName] = func;
    return func;
}

FunctionRegister *FunctionRegister::GetInstance()
{
    static FunctionRegister instance;
    return &instance;
}

void FunctionRegister::RegisteFunction(const std::string& soName, const std::string& funcName)
{
    std::lock_guard<std::mutex> lock(mu_);
    auto itr = registry_.find(soName);
    if (itr == registry_.end()) {
        auto func_loader = std::make_unique<FunctionLoader>(soName);
        func_loader->Set(funcName);
        registry_.emplace(soName, std::move(func_loader));
        return;
    }
    itr->second->Set(funcName);
}

FunctionHandle FunctionRegister::Get(const std::string &soName, const std::string &funcName)
{
    // 业务逻辑保证Get时不会有其它线程写
    auto itr = registry_.find(soName);
    if (itr != registry_.end()) {
        return itr->second->Get(funcName);
    }
    return nullptr;
}
}  // Common
}  // Mspti