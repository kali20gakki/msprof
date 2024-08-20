/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : json_writer.h
 * Description        : Json格式写入工具声明
 * Author             : msprof team
 * Creation Date      : 2024/7/16
 * *****************************************************************************
 */

#ifndef ANALYSIS_INFRASTRUCTURE_DUMP_TOOLS_JSON_TOOL_INCLUDE_JSON_WRITER_H
#define ANALYSIS_INFRASTRUCTURE_DUMP_TOOLS_JSON_TOOL_INCLUDE_JSON_WRITER_H

#include <cstddef>
#include <string>
#include <memory>
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

namespace Analysis {

namespace Infra {

class JsonWriter final {
public:
    JsonWriter();

    ~JsonWriter();

    /// Obtains the serialized JSON string.
    const char *GetString() const;
    size_t GetSize() const {return stream_->GetSize(); }

    JsonWriter &StartObject();

    JsonWriter &operator[](const char *name)
    {
        this->Member(name);
        return *this;
    };

    JsonWriter &Member(const char *name);

    JsonWriter &EndObject();

    JsonWriter &StartArray();

    JsonWriter &EndArray();

    JsonWriter &operator<<(const bool &b);

    JsonWriter &operator<<(const unsigned &u);

    JsonWriter &operator<<(const int &i);

    JsonWriter &operator<<(const double &d);

    JsonWriter &operator<<(const uint64_t &d);

    JsonWriter &operator<<(const int64_t &d);

    JsonWriter &operator<<(const std::string &s);

    JsonWriter &SetNull();

private:
    JsonWriter(const JsonWriter &);

    JsonWriter &operator=(const JsonWriter &);

    // PIMPL idiom
    std::unique_ptr<rapidjson::StringBuffer> stream_;      // Stream buffer.
    std::unique_ptr<rapidjson::Writer<rapidjson::StringBuffer>> writer_;      // JSON writer.
};

}

}

#endif