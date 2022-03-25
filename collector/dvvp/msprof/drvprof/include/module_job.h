/**
* @file module_job.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef MSPROF_ENGINE_MODULE_JOB_H
#define MSPROF_ENGINE_MODULE_JOB_H

#include <map>
#include <string>
#include "prof_engine.h"
#include "data_dumper.h"
#include "proto/profiler.pb.h"

namespace Msprof {
namespace Engine {
using namespace analysis::dvvp::proto;
class ModuleJob {
public:
    /**
    * @brief ModuleJob: the construct function
    * @param [in] module: the name of the module
    * @param [in] engine: the engine of user
    */
    ModuleJob(const std::string& module, EngineIntf &engine);
    virtual ~ModuleJob();

public:
    /**
    * @brief ProfStart: create a Reporter according the platform  and create a plugin with the API of engine,
    *                   then init the plugin with the new reporter
    * @param [in] taskId: the task id created by EngineMgr, only for host
    * @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
    */
    int ProfStart();

    /**
    * @brief ProfConfig: config the plugin of user self-defined
    * @param [in] config: config info from FMK
    * @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
    */
    int ProfConfig(const SHARED_PTR_ALIA<ProfilerJobConfig>& config);

    /**
    * @brief ProfStop: stop the plugin of user self-defined, then stop and reset the reporter
    * @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
    */
    int ProfStop();

private:
    /**
    * @brief StartPlugin: create a plugin with engine, then init the plugin with a new Reporter
    * @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
    */
    int StartPlugin();

    /**
    * @brief StopPlugin: stop the plugin, then call the engine API to release the plugin
    * @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
    */
    int StopPlugin();

    /**
    * @brief MapProtoToStd: translate the protobuf msg to map info, key-value pair is switch with it value
    * @param [in] src: the key-value pairs from FMK
    * @param [out] dst: the key-value pairs for send to user
    */
    void MapProtoToStd(const google::protobuf::Map<::std::string, ::std::string>& src,
                        std::map<std::string, std::string>& dst);
    SHARED_PTR_ALIA<DataDumper> CreateDumper(const std::string& module);

private:
    std::string module_; // module name
    EngineIntf *engine_; // engine of user defined
    PluginIntf *plugin_; // the plugin create with API of engine
    volatile bool isStarted_; // whether the module has been started
    // for host,  API for save data to local disk by FileDumper
    // for device, API for send data to host by StreamDumper
    SHARED_PTR_ALIA<Msprof::Engine::DataDumper> reporter_;
};
}}
#endif
