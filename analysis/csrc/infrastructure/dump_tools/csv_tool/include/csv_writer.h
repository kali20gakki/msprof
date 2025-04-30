/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : csv_writer.h
 * Description        : csv格式写入工具声明
 * Author             : msprof team
 * Creation Date      : 2025/4/27
 * *****************************************************************************
 */

#ifndef ANALYSIS_INFRASTRUCTURE_DUMP_TOOLS_CSV_TOOL_INCLUDE_CSV_WRITER_H
#define ANALYSIS_INFRASTRUCTURE_DUMP_TOOLS_CSV_TOOL_INCLUDE_CSV_WRITER_H

#include <string>
#include <vector>
#include <set>

namespace Analysis {
namespace Infra {
class CsvWriter final {
public:
    CsvWriter();
    ~CsvWriter() = default;
    void WriteCsv(const std::string& filePath, const std::vector<std::string>& headers,
                  const std::vector<std::vector<std::string>>& res, const std::set<int>& maskCols);
private:
    template <typename Iterator>
    void DumpCsvFile(const std::string& filePath, const std::vector<std::string> &headers,
                     Iterator begin, Iterator end, std::set<int> maskCols);

    std::string GetSliceFileName(uint16_t sliceIndex, size_t resSize, const std::string& filePath);

private:
    std::string timestamp_;
};
}
}

#endif // ANALYSIS_INFRASTRUCTURE_DUMP_TOOLS_CSV_TOOL_INCLUDE_CSV_WRITER_H