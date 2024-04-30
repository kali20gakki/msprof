/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : parser.cpp
 * Description        : 解析二进制数据基类实现
 * Author             : msprof team
 * Creation Date      : 2024/4/29
 * *****************************************************************************
 */

#include "parser.h"
#include <iostream>
#include <algorithm>
#include "analysis/csrc/domain/services/parser/include/parser_error_code.h"

namespace Analysis {
namespace Domain {
using namespace Infra;

uint32_t Parser::GetFileSize(const char *filePath)
{
    FILE *fp = fopen(filePath, "r");
    if (fp == nullptr) {
        ERROR("fopen error! path: %, error: %", filePath, PARSER_FOPEN_ERROR);
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    uint32_t fileSize = ftell(fp);

    fclose(fp);

    return fileSize;
}

uint64_t Parser::GetFilesSize(const std::vector<std::string> &paths)
{
    uint64_t fileSize = 0;

    uint32_t size;
    for (const auto &loop: paths) {
        size = this->GetFileSize(loop.c_str());
        INFO("loop: %, file size: %", loop, size);
        fileSize += size;
    }
    INFO("total size: %", fileSize);
    return fileSize;
}

uint32_t Parser::ReadData(const std::vector<std::string> &files, size_t firstFileOffset)
{
    uint64_t binaryDataOffset = 0;

    for (const auto &loop: files) {
        uint32_t fileSize = this->GetFileSize(loop.c_str());

        auto file = fopen(loop.c_str(), "r");
        if (file == nullptr) {
            ERROR("fopen error! path: %, error: %", loop, PARSER_FOPEN_ERROR);
            return PARSER_FOPEN_ERROR;
        }

        if (loop == *files.begin()) {
            fseek(file, firstFileOffset, SEEK_SET);
            fileSize -= firstFileOffset;
        }

        auto code = fread(this->binaryData.get() + binaryDataOffset, sizeof(uint8_t), fileSize, file);
        if (code != fileSize) {
            ERROR("fread error! code = %, fileSize = %, error: %", code, fileSize, ferror(file));
            fclose(file);
            return Analysis::PARSER_FREAD_ERROR;
        }

        fclose(file);

        binaryDataOffset += fileSize;
    }

    return Analysis::ANALYSIS_OK;
}

std::string Parser::GetFilePath(const DeviceContext &deviceContext)
{
    std::string deviceFilePath = deviceContext.GetDeviceFilePath();
    return deviceFilePath + "/data";
}

uint32_t Parser::GetNoFileCode()
{
    return Analysis::PARSER_GET_FILE_ERROR;
}

// 提取字符串尾部的数字部分
int ExtractNumber(const std::string &str)
{
    size_t pos = str.find_last_of('_');
    if (pos != std::string::npos) {
        std::string numStr = str.substr(pos + 1);
        try {
            return std::stoi(numStr);
        } catch (const std::invalid_argument &) {
            ERROR("Failed to parse the slice number in the binary file: %", str);
        }
    }
    return 0; // 默认返回0
}

uint32_t Parser::ReadDataEntry(const DeviceContext &deviceContext)
{
    // 读取解析文件
    auto files = Analysis::Utils::File::GetOriginData(this->GetFilePath(deviceContext),
                                                      {this->GetFilePattern()},
                                                      {"done", "complete"});
    if (files.size() == 0) {
        ERROR("notify: no files pattern");
        this->binaryData = nullptr;
        this->binaryDataSize = 0;
        return this->GetNoFileCode();
    }

    // 自然排序文件名称
    std::sort(files.begin(), files.end(), [](const std::string &a, const std::string &b) {
        // 提取数字部分并转换为整数进行比较
        int num_a = ExtractNumber(a);
        int num_b = ExtractNumber(b);
        return num_a < num_b;
    });

    // 读取每个处理块大小
    auto trunkSize = this->GetTrunkSize();

    // 读取所有文件大小
    auto fileSize = this->GetFilesSize(files);

    // 计算处理块个数
    size_t structCount = fileSize / trunkSize;
    size_t firstFileOffset = fileSize % trunkSize;
    if (firstFileOffset) {
        // 如果有余，从开始offset偏移获取！
        INFO("offset: %", firstFileOffset);
    }

    this->binaryData.reset(new uint8_t[structCount * trunkSize]);
    if (this->binaryData == nullptr) {
        ERROR("new binary data error!");
        return Analysis::PARSER_NEW_BINARY_DATA_ERROR;
    }
    this->binaryDataSize = structCount * trunkSize;
    INFO("files total size: %", structCount * trunkSize);
    return this->ReadData(files, firstFileOffset);
}

uint32_t Parser::ProcessEntry(DataInventory &dataInventory, const Infra::Context &context)
{
    const DeviceContext& deviceContext = static_cast<const DeviceContext&>(context);
    uint32_t code = this->ReadDataEntry(deviceContext);
    if (code != Analysis::ANALYSIS_OK) {
        ERROR("ReadData error: %", code);
        return Analysis::PARSER_READ_DATA_ERROR;
    }

    code = this->ParseData(dataInventory);
    if (code != Analysis::ANALYSIS_OK) {
        ERROR("ParseData error: %", code);
        return Analysis::PARSER_PARSE_DATA_ERROR;
    }

    return Analysis::ANALYSIS_OK;
}

}
}