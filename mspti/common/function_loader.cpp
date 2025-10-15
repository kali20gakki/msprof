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
#include "common/function_loader.h"

#include <dlfcn.h>
#include <iostream>
#include <set>

#include "common/utils.h"

namespace Mspti {
namespace Common {
// 该模块请使用库函数，不能使用MSPTI_LOGX
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

std::string FunctionLoader::CanonicalSoPath()
{
    static const std::set<std::string> soNameList = {
        "libascend_hal.so",
        "libascendalog.so",
        "libruntime.so",
        "libhccl.so",
        "libprofapi.so",
    };
    if (soNameList.find(soName_) == soNameList.end()) {
        std::cout << soName_ << " was invalid." << std::endl;
        return "";
    }
    char *ascendHomePath = std::getenv("ASCEND_HOME_PATH");
    if (ascendHomePath == nullptr || ascendHomePath[0] == '\0') {
        return soName_;
    }
    auto soPath = std::string(ascendHomePath) + "/lib64/" + soName_;
    auto canonicalPath = Utils::RealPath(Utils::RelativeToAbsPath(soPath));
    return Utils::FileExist(canonicalPath) && Utils::FileReadable(canonicalPath) ? canonicalPath : soName_;
}

void *FunctionLoader::Get(const std::string& funcName)
{
    if (!handle_) {
        auto soPath = CanonicalSoPath();
        if (soPath.empty()) {
            return nullptr;
        }
        auto handle = dlopen(soPath.c_str(), RTLD_LAZY);
        if (handle == nullptr) {
            std::cout << dlerror() << std::endl;
            return nullptr;
        }
        handle_ = handle;
    }

    auto itr = registry_.find(funcName);
    if (itr == registry_.end()) {
        std::cout << "function(\"" << funcName << "\") is not registered." << std::endl;
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

void FunctionRegister::RegisterFunction(const std::string& soName, const std::string& funcName)
{
    std::lock_guard<std::mutex> lock(mu_);
    auto itr = registry_.find(soName);
    if (itr == registry_.end()) {
        std::unique_ptr<FunctionLoader> func_loader = nullptr;
        Mspti::Common::MsptiMakeUniquePtr(func_loader, soName);
        if (!func_loader) {
            std::cout << "Failed to init FunctionLoader." << std::endl;
            return;
        }

        func_loader->Set(funcName);
        registry_.emplace(soName, std::move(func_loader));
        return;
    }
    itr->second->Set(funcName);
}

FunctionHandle FunctionRegister::Get(const std::string &soName, const std::string &funcName)
{
    std::lock_guard<std::mutex> lock(mu_);
    auto itr = registry_.find(soName);
    if (itr != registry_.end()) {
        return itr->second->Get(funcName);
    }
    return nullptr;
}

void* RegisterFunction(const std::string& soName, const std::string& funcName)
{
    FunctionRegister::GetInstance()->RegisterFunction(soName, funcName);
    return FunctionRegister::GetInstance()->Get(soName, funcName);
}
}  // Common
}  // Mspti