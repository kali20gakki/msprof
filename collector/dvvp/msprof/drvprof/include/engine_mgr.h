/**
* @file engine_mgr.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef MSPROF_ENGINE_ENGINE_MGR_H
#define MSPROF_ENGINE_ENGINE_MGR_H

#include <map>
#include <memory>
#include <string>
#include "module_job.h"
#include "prof_engine.h"
#include "singleton/singleton.h"
#include "proto/profiler.pb.h"

namespace Msprof {
namespace Engine {
using namespace analysis::dvvp::proto;
using CONST_ENGINE_INTF_PTR = const EngineIntf *;
class EngineMgr : public analysis::dvvp::common::singleton::Singleton<EngineMgr> {
public:
    EngineMgr();
    virtual ~EngineMgr();

public:
    /**
    * @brief RegisterEngine: register an engine to profiling
    * @param [in] module: the module name
    * @param [in] engine: the profiling engine of user defined
    * @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
    */
    int RegisterEngine(const std::string& module, CONST_ENGINE_INTF_PTR engine);

    /**
    * @brief Init: Initialize an engine to profiling
    * @param [in] module: the module name
    * @param [in] engine: the profiling engine of user defined
    * @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
    */
    int Init(const std::string& module, CONST_ENGINE_INTF_PTR engine);

    /**
    * @brief UnInit: De-initialize the engine to profiling
    * @param [in] module: the module name
    * @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
    */
    int UnInit(const std::string& module);

    /**
    * @brief ProfStart: according the module name to start the engine, it will create a reporter for user
    * @param [in] module: the module name
    * @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
    */
    int ProfStart(const std::string& module);

    /**
    * @brief ProfStop: according the module name to stop the engine, it's the inverse process of ProfStart
    * @param [in] module: the module name
    * @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
    */
    int ProfStop(const std::string& module);

private:
    /**
    * @brief ProfConfig: according the config msg to config the engine
    * @param [in] module: the module name
    * @param [in] config: the config info from FMK
    * @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
    */
    int ProfConfig(const std::string& module,
                   const SHARED_PTR_ALIA<ProfilerJobConfig>& config);

    /**
    * @brief ConfigHandler: call ProfConfig to config the module, it's a call back function for ConfigMgr
    * @param [in] module: the module name
    * @param [in] config: the config info from FMK
    * @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
    */
    static int ConfigHandler(const std::string &module,
                             const SHARED_PTR_ALIA<ProfilerJobConfig> &config);

private:
    std::map<std::string, EngineIntf *> engines_; // the map of module name and engine
    std::map<std::string, SHARED_PTR_ALIA<ModuleJob>> jobs_; // the map of module name and job
    std::mutex mtx_;
};
}}
#endif
