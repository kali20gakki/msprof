/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : device_context.cpp
 * Description        : 结构化context模块
 * Author             : msprof team
 * Creation Date      : 2024/4/29
 * *****************************************************************************
 */
#include "device_context.h"
#include <iostream>
#include <sstream>
#include <functional>
#include <vector>
#include <algorithm>
#include <dirent.h>
#include "nlohmann/json.hpp"
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/utils/file.h"
#include "analysis/csrc/utils/time_logger.h"
#include "analysis/csrc/utils/thread_pool.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/process/include/process_control.h"
#include "device_context_error_code.h"

using namespace Analysis;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Infra;

namespace Analysis {

namespace Domain {
DeviceContext& DeviceContext::Instance()
{
    thread_local DeviceContext ins;
    return ins;
}

bool DeviceContext::Init(const std::string &devicePath)
{
    if (!this->isInitialized_) {
        this->deviceContextInfo.deviceFilePath = devicePath;
        std::vector<std::function<bool()>> funcList = {[this]() { return this->GetInfoJson(); },
                                                       [this]() { return this->GetCpuInfo(); },
                                                       [this]() { return this->GetSampleJson(); },
                                                       [this]() { return this->GetHostStart(); },
                                                       [this]() { return this->GetDeviceStart(); }};
        auto ret = std::all_of(funcList.begin(), funcList.end(), [](std::function<bool()> func) {return func();});
        this->isInitialized_ = ret; // 标记已初始化
        return ret;
    }
    return true;
}

std::vector<std::string> GetDeviceDirectories(const std::string &path)
{
    std::vector<std::string> subdirs;
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        ERROR("Error opening directory: %, error: %", path, DEVICE_CONTEXT_OPEN_DIR_ERROR);
        return subdirs;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_DIR) { // Check if it's a directory
            std::string subdirName = entry->d_name;
            if (subdirName.size() > MIN_SUB_DIR_NBAME_LEN &&
                subdirName.substr(0, MIN_SUB_DIR_NBAME_LEN) == "device" &&
                subdirName.back() >= '0' && subdirName.back() <= '9') {
                std::string subdirPath = File::PathJoin({path, subdirName});
                subdirs.push_back(subdirPath);
            }
        }
    }
    closedir(dir);
    return subdirs;
}

std::vector<DataInventory> DeviceContextEntry(const char *targetDir, const char *stopAt)
{
    Utils::TimeLogger t{"DeviceContextEntry "};
    std::vector<std::string> subdirs = GetDeviceDirectories(targetDir);

    std::function<void()> func;

    ThreadPool tp(subdirs.size());
    std::vector<DataInventory> processDataVec(subdirs.size());
    std::vector<std::string> processStats(subdirs.size());
    size_t i = 0;
    for (const auto &subdir: subdirs) {
        auto &processStat = processStats[i];
        auto &prcessData = processDataVec[i];
        ++i;
        func = [subdir, &processStat, &prcessData, stopAt] {
            DeviceContext &context = DeviceContext::Instance();
            if (!context.Init(subdir)) {
                processStat = "Init failed, exit!";
                return;
            }
            if (stopAt != nullptr) {
                context.SetStopAt(stopAt);
            }

            auto regInfo = ProcessRegister::CopyProcessInfo();
            ProcessControl processControl(regInfo);

            bool ret = processControl.ExecuteProcess(prcessData, context);

            auto stat = processControl.GetExecuteStat();
            RecordProcessStat(stat, subdir, processStat);
        };
        tp.AddTask(func);
    }
    tp.Start();
    tp.WaitAllTasks();
    tp.Stop();

    for (const auto &stat: processStats) {
        INFO("stat info: %", stat);
    }
    return processDataVec;
}
}
}
