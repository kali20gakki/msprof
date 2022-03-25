/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: Engine manager
 * Author: dml
 * Create: 2018-06-13
 */
#include "engine_mgr.h"
#include <cstdlib>
#include "error_code.h"
#include "msprof_dlog.h"
#include "utils.h"

using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;

namespace Msprof {
namespace Engine {
EngineMgr::EngineMgr()
{
}

EngineMgr::~EngineMgr()
{
}

/**
* @brief RegisterEngine: register an engine to profiling
* @param [in] module: the module name
* @param [in] engine: the profiling engine of user defined
* @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
*/
int EngineMgr::RegisterEngine(const std::string& module, CONST_ENGINE_INTF_PTR engine)
{
    std::lock_guard<std::mutex> lk(mtx_);
    // all profiling supported module name
    // testModule1 and testModule2 is just for extended
    const std::set<std::string> MODULE_NAME_SET {"Framework", "HCCL"};
    auto it = MODULE_NAME_SET.find(module);
    if (it == MODULE_NAME_SET.end()) {  // check module name in MODULE_NAME_SET
        MSPROF_LOGE("register failed, module %s not in MODULE_NAME_SET", module.c_str());
        return PROFILING_FAILED;
    }
    if (!module.empty() && engine != nullptr) {
        auto iter = engines_.find(module); // check whether the module has been registed
        if (iter == engines_.end()) {
            engines_[module] = const_cast<EngineIntf *>(engine); // add the engine into the map
            MSPROF_LOGI("register module %s successfully.", module.c_str());
            return PROFILING_SUCCESS;
        } else {
            MSPROF_LOGE("module %s has been registed, cannot register same name modules", module.c_str());
        }
    } else {
        MSPROF_LOGE("register engine failed, module name is empty or engine is nullptr");
    }
    return PROFILING_FAILED;
}

/* *
 * @brief Init: Initialize an engine to profiling
 * @param [in] module: the module name
 * @param [in] engine: the profiling engine of user defined
 * @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
 */
int EngineMgr::Init(const std::string& module, CONST_ENGINE_INTF_PTR engine)
{
    std::lock_guard<std::mutex> lk(mtx_);
    // all profiling supported module name
    // testModule1 and testModule2 is just for extended
    const std::set<std::string> MODULE_NAME_SET {
        "Framework", "runtime", "HCCL", "AclModule", "testModule1", "testModule2"};
    auto it = MODULE_NAME_SET.find(module);
    if (it == MODULE_NAME_SET.end()) { // check module name in MODULE_NAME_SET
        if (module.find("DATA_PREPROCESS") == std::string::npos) {
            MSPROF_LOGE("Initialization failed, module %s is not in MODULE_NAME_SET", module.c_str());
            return PROFILING_FAILED;
        }
    }
    if (!module.empty() && engine != nullptr) {
        auto iter = engines_.find(module); // check whether the module has been init
        if (iter == engines_.end()) {
            engines_[module] = const_cast<EngineIntf *>(engine); // add the engine into the map
            MSPROF_LOGI("Initialized module %s successfully.", module.c_str());
            return PROFILING_SUCCESS;
        } else {
            MSPROF_LOGE("Module %s has been initialized, cannot initialize a module with the same name.",
                        module.c_str());
        }
    }
    return PROFILING_FAILED;
}

/* *
 * @brief UnInit: De-initialize the engine to profiling
 * @param [in] module: the module name
 * @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
 */
int EngineMgr::UnInit(const std::string &module)
{
    std::lock_guard<std::mutex> lk(mtx_); // lock
    if (!module.empty()) {
        auto iter = engines_.find(module); // check whether the module has been init
        if (iter != engines_.end()) {
            engines_.erase(iter); // Remove engines_ [module] from the map
            MSPROF_LOGI("UnInit module %s successfully.", module.c_str());
            return PROFILING_SUCCESS;
        } else {
            MSPROF_LOGE("Module %s has been de-initialized.", module.c_str());
        }
    }
    return PROFILING_FAILED;
}

/**
* @brief ProfStart: according the module name to start the engine, it will create a reporter for user
* @param [in] module: the module name
* @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
*/
int EngineMgr::ProfStart(const std::string& module)
{
    std::lock_guard<std::mutex> lk(mtx_);
    int ret = PROFILING_FAILED;
    auto job = jobs_.find(module); // check whether the module has been inserted into the jobs_
    if (job == jobs_.end()) {
        auto iter = engines_.find(module); // get the engine according the name
        if (iter != engines_.end()) {
            auto engine = iter->second;
            SHARED_PTR_ALIA<ModuleJob> mJob;
            MSVP_MAKE_SHARED2_RET(mJob, ModuleJob, module, *engine, PROFILING_FAILED);
            jobs_[module] = mJob; // insert the module into the jobs_
            ret = mJob->ProfStart();
        } else {
            MSPROF_LOGE("Cannot find the module:%s in engines_", module.c_str());
        }
    } else {
        MSPROF_LOGE("Find the module:%s in jobs_", module.c_str());
    }
    return ret;
}

/* *
 * @brief ProfConfig: according the config msg to config the engine
 * @param [in] module: the module name
 * @param [in] config: the config info from FMK
 * @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
 */
int EngineMgr::ProfConfig(const std::string &module,
                          const SHARED_PTR_ALIA<ProfilerJobConfig> &config)
{
    std::lock_guard<std::mutex> lk(mtx_);
    int ret = PROFILING_FAILED;
    auto job = jobs_.find(module);
    if (job != jobs_.end()) {
        SHARED_PTR_ALIA<ModuleJob> mJob = job->second;
        if (mJob != nullptr) {
            ret = mJob->ProfConfig(config); // start to config the module
        } else {
            MSPROF_LOGE("job->sencod is nullptr");
        }
    } else {
        MSPROF_LOGE("Cannot find the module:%s in jobs_", module.c_str());
    }
    return ret;
}

/* *
 * @brief ProfStop: according the module name to stop the engine, it's the inverse process of ProfStart
 * @param [in] module: the module name
 * @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
 */
int EngineMgr::ProfStop(const std::string &module)
{
    std::lock_guard<std::mutex> lk(mtx_);
    int ret = PROFILING_FAILED;
    auto job = jobs_.find(module);
    if (job != jobs_.end()) {
        SHARED_PTR_ALIA<ModuleJob> mJob = job->second;
        if (mJob != nullptr) {
            ret = mJob->ProfStop(); // stop the module
        } else {
            MSPROF_LOGE("job->second is nullptr");
        }
        jobs_.erase(job); // delete the module from the jobs_
    } else {
        MSPROF_LOGE("cannot find the module:%s in jobs_", module.c_str());
    }
    return ret;
}

/* *
 * @brief ConfigHandler: call ProfConfig to config the module, it's a call back function for ConfigMgr
 * @param [in] module: the module name
 * @param [in] config: the config info from FMK
 * @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
 */
int EngineMgr::ConfigHandler(const std::string &module,
                             const SHARED_PTR_ALIA<ProfilerJobConfig> &config)
{
    if (module.empty() || config == nullptr) {
        MSPROF_LOGE("Input parameter is error, module name is empty or config is nullptr");
        return PROFILING_FAILED;
    }
    return EngineMgr::instance()->ProfConfig(module, config);
}

/* *
 * @brief RegisterEngine: the API for user to register engine and auto start the engine
 * @param [in] module: the module name
 * @param [in] engine: the user self-defined engine
 * @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
 */
int RegisterEngine(const std::string& module, CONST_ENGINE_INTF_PTR engine)
{
    if (!module.empty() && engine != nullptr) {
        MSPROF_EVENT("[RegisterEngine]Received request to register engine %s", module.c_str());
        if (EngineMgr::instance()->RegisterEngine(module, engine) == PROFILING_SUCCESS) {
            return EngineMgr::instance()->ProfStart(module);
        } else {
            MSPROF_LOGE("EngineMgr::RegisterEngine failed");
        }
    } else {
        MSPROF_LOGE("register engine failed, module name is empty or engine is nullptr");
    }
    return PROFILING_FAILED;
}

/* *
 * @brief Init: the API for user to init engine and auto start the engine
 * @param [in] module: the module name
 * @param [in] engine: the user self-defined engine
 * @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
 */
int Init(const std::string &module, CONST_ENGINE_INTF_PTR engine)
{
    if (!module.empty() && engine != nullptr) {
        MSPROF_EVENT("[Init]Received request to init engine %s", module.c_str());
        if (EngineMgr::instance()->Init(module, engine) == PROFILING_SUCCESS) {
            return EngineMgr::instance()->ProfStart(module);
        } else {
            MSPROF_LOGE("EngineMgr::Init failed");
        }
    } else {
        MSPROF_LOGE("Initialization engine failed, module name is empty or engine is nullptr");
    }
    return PROFILING_FAILED;
}

/* *
 * @brief UnInit: the API for user to uninit engine
 * @param [in] module: the module name
 * @return : success return UNPROFILING_SUCCESS, failed return UNPROFIING_FAILED
 */
int UnInit(const std::string &module)
{
    int ret = PROFILING_SUCCESS;
    if (!module.empty()) {
        MSPROF_EVENT("[UnInit]Received request to uninit engine %s", module.c_str());
        if (EngineMgr::instance()->ProfStop(module) == PROFILING_FAILED) {
            ret = PROFILING_FAILED;
        }
        if (EngineMgr::instance()->UnInit(module) == PROFILING_FAILED) {
            ret = PROFILING_FAILED;
        }
        if (ret == PROFILING_FAILED) {
            MSPROF_LOGE("EngineMgr::UnInit failed");
        }
    } else {
        MSPROF_LOGE("Failed to UnInit the engine, module name is empty or engine is nullptr");
        ret = PROFILING_FAILED;
    }
    return ret;
}
} // namespace Engine
} // namespace Msprof
