/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 * Description: handle profiling request
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_MESSAGE_MESSAGE_H
#define ANALYSIS_DVVP_MESSAGE_MESSAGE_H

#include <iostream>
#include <sstream>
#include "msprof_dlog.h"
#include "nlohmann/json.hpp"
#include "utils/utils.h"
#include "config/config.h"

namespace analysis {
namespace dvvp {
namespace message {

#define MSG_STR(s) #s

#define SET_VALUE(root, value)        \
    do {                              \
        (root)[MSG_STR(value)] = (value); \
    } while (0)

#define SET_ARRAY_VALUE(root, vector)             \
    do {                                          \
        nlohmann::json array_obj;                 \
        for (int i = 0; i < (vector).size(); ++i) { \
            array_obj.push_back((vector)[i]);       \
        }                                         \
        (root)[MSG_STR(vector)] = array_obj;        \
    } while (0)

#define SET_ARRAY_OBJECT_VALUE(root, vector)         \
    do {                                             \
        nlohmann::json array_obj;                    \
        for (size_t i = 0; i < (vector).size(); ++i) { \
            nlohmann::json obj;                      \
            (vector)[i].ToObject(obj);                \
            array_obj.push_back(obj);                \
        }                                            \
        (root)[MSG_STR(vector)] = array_obj;           \
    } while (0)

#define FROM_ARRAY_OBJECT_VALUE(root, field, Type) do {                  \
    const nlohmann::json & jarray = (root).at(MSG_STR(field));           \
    if (jarray.is_array()) {                                             \
        for (auto iter = jarray.begin(); iter != jarray.end(); ++iter) { \
            Type value;                                                  \
            value.FromObject(*iter);                                     \
            (field).push_back(value);                                    \
        }                                                                \
    }                                                                    \
} while (0)

#define FROM_STRING_VALUE(root, field)          \
    do {                                        \
        field = (root).value(MSG_STR(field), ""); \
    } while (0)

#define FROM_INT_VALUE(root, field, def)         \
    do {                                         \
        field = (root).value(MSG_STR(field), def); \
    } while (0)

#define FROM_BOOL_VALUE(root, field)               \
    do {                                           \
        field = (root).value(MSG_STR(field), FALSE); \
    } while (0)


enum COLLECTON_STATUS {
    SUCCESS = 0,
    ERR = 1
};

struct BaseInfo {
    std::string ToString()
    {
        std::string result;

        nlohmann::json root;
        ToObject(root);
        result = root.dump();

        return result;
    }

    virtual void ToObject(nlohmann::json &object) = 0;

    virtual void FromObject(const nlohmann::json &object) = 0;

    virtual std::string GetStructName()
    {
        return "BaseInfo";
    }

    bool FromString(const std::string &value)
    {
        if (value.empty()) {
            return false;
        }

        bool ok = false;

        try {
            auto root = nlohmann::json::parse(value);
            FromObject(root);
            ok = true;
        } catch (std::exception &e) {
            MSPROF_LOGE("%s ::FromString(): %s", GetStructName().c_str(), e.what());
        }
        return ok;
    }
};

struct StatusInfo : BaseInfo {
    std::string dev_id;
    int status;
    std::string info;

    explicit StatusInfo(std::string dev_id, int status = ERR, const std::string &info = "")
        : dev_id(dev_id),
          status(status),
          info(info)
        {
    }

    StatusInfo()
        : dev_id("-1"),
          status(ERR)
        {
    }
    virtual ~StatusInfo() {}

    std::string GetStructName() override
    {
        return "StatusInfo";
    }

    void ToObject(nlohmann::json &object) override
    {
        SET_VALUE(object, dev_id);
        SET_VALUE(object, status);
        SET_VALUE(object, info);
    }

    void FromObject(const nlohmann::json &object) override
    {
        FROM_STRING_VALUE(object, dev_id);
        FROM_INT_VALUE(object, status, ERR);
        FROM_STRING_VALUE(object, info);
    }
};

struct Status : BaseInfo {
    int status;
    std::vector<StatusInfo> info;

    Status() : status(ERR)
        {
    }
    virtual ~Status() {}

    std::string GetStructName() override
    {
        return "Status";
    }

    void AddStatusInfo(const StatusInfo &statusInfo)
    {
        info.push_back(statusInfo);
    }

    void ToObject(nlohmann::json &object) override
    {
        SET_VALUE(object, status);
        SET_ARRAY_OBJECT_VALUE(object, info);
    }

    void FromObject(const nlohmann::json &object) override
    {
        FROM_INT_VALUE(object, status, ERR);
        info.clear();
        FROM_ARRAY_OBJECT_VALUE(object, info, StatusInfo);
    }
};

// Attention:
// intervals of ProfileParams maybe large,
// remember to cast to long long before calculation (for example, convert ms to ns)
struct JobContext : BaseInfo {
    std::string result_dir;  // result_dir
    std::string module;  // module: runtime,Framework
    std::string tag;  // tag: module tag
    std::string dev_id;  // dev_id
    std::string job_id;  // job_id
    uint64_t chunkStartTime;
    uint64_t chunkEndTime;
    int32_t dataModule;

    // for debug purpose
    std::string stream_enabled;

    JobContext() : chunkStartTime(0), chunkEndTime(0), dataModule(0)
    {
    }
    virtual ~JobContext() {}

    std::string GetStructName() override
    {
        return "JobContext";
    }

    void ToObject(nlohmann::json &object) override
    {
        SET_VALUE(object, result_dir);
        SET_VALUE(object, module);
        SET_VALUE(object, tag);
        SET_VALUE(object, dev_id);
        SET_VALUE(object, job_id);
        SET_VALUE(object, chunkStartTime);
        SET_VALUE(object, chunkEndTime);
        SET_VALUE(object, dataModule);
        SET_VALUE(object, stream_enabled);
    }

    void FromObject(const nlohmann::json &object) override
    {
        FROM_STRING_VALUE(object, result_dir);
        FROM_STRING_VALUE(object, module);
        FROM_STRING_VALUE(object, tag);
        FROM_STRING_VALUE(object, dev_id);
        FROM_STRING_VALUE(object, job_id);
        FROM_INT_VALUE(object, chunkStartTime, 0);
        FROM_INT_VALUE(object, chunkEndTime, 0);
        FROM_INT_VALUE(object, dataModule, 0);
        FROM_STRING_VALUE(object, stream_enabled);
    }
};
}  // namespace message
}  // namespace dvvp
}  // namespace analysis
#endif

