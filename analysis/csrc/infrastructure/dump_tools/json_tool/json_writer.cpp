/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : json_writer.cpp
 * Description        : Json格式写入工具实现
 * Author             : msprof team
 * Creation Date      : 2024/4/12
 * *****************************************************************************
 */
#include "analysis/csrc/infrastructure/dump_tools/json_tool/include/json_writer.h"

#include "analysis/csrc/infrastructure/utils/utils.h"

using namespace rapidjson;

namespace Analysis {

namespace Infra {

JsonWriter::JsonWriter()
    : stream_(Utils::MAKE_UNIQUE_PTR<StringBuffer>()), writer_(Utils::MAKE_UNIQUE_PTR<PrettyWriter<StringBuffer>>(*stream_))
{
}

JsonWriter::~JsonWriter()
{
}

const char* JsonWriter::GetString() const
{
    return stream_->GetString();
}

JsonWriter& JsonWriter::StartObject()
{
    writer_->StartObject();
    return *this;
}

JsonWriter& JsonWriter::EndObject()
{
    writer_->EndObject();
    return *this;
}

JsonWriter& JsonWriter::Member(const char* name)
{
    writer_->String(name, static_cast<SizeType>(strlen(name)));
    return *this;
}

JsonWriter& JsonWriter::StartArray()
{
    writer_->StartArray();
    return *this;
}

JsonWriter& JsonWriter::EndArray()
{
    writer_->EndArray();
    return *this;
}

JsonWriter& JsonWriter::operator<<(const bool& b)
{
    writer_->Bool(b);
    return *this;
}

JsonWriter& JsonWriter::operator<<(const unsigned& u)
{
    writer_->Uint(u);
    return *this;
}

JsonWriter& JsonWriter::operator<<(const int& i)
{
    writer_->Int(i);
    return *this;
}

JsonWriter& JsonWriter::operator<<(const double& d)
{
    writer_->Double(d);
    return *this;
}

JsonWriter& JsonWriter::operator<<(const uint64_t& d)
{
    writer_->Uint64(d);
    return *this;
}

JsonWriter& JsonWriter::operator<<(const int64_t& d)
{
    writer_->Int64(d);
    return *this;
}

JsonWriter& JsonWriter::operator<<(const std::string& s)
{
    writer_->String(s.c_str(), static_cast<SizeType>(s.size()));
    return *this;
}

JsonWriter& JsonWriter::SetNull()
{
    writer_->Null();
    return *this;
}

}

}