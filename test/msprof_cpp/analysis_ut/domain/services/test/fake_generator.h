/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2024
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : fake_generator.h
 * Description        : device侧虚假数据生成
 * Author             : msprof team
 * Creation Date      : 2024/4/30
 * *****************************************************************************
 */

#ifndef TEST_MSPROF_CPP_ANALYSIS_UT_DOMAIN_PARSER_TEST_FAKE_GENERATOR_H
#define TEST_MSPROF_CPP_ANALYSIS_UT_DOMAIN_PARSER_TEST_FAKE_GENERATOR_H

#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

template<typename T>
bool WriteBin(std::vector<T> &datas, const std::string &path, std::string name)
{
    if (!Analysis::Utils::File::Exist(path)) {
        if (!Analysis::Utils::File::CreateDir(path)) {
            return false;
        }
    }
    std::ofstream outFile(Analysis::Utils::File::PathJoin({path, name}),
                          std::ios::out | std::ios::binary | std::ios::app);
    if (!outFile) {
        return false;
    }

    outFile.write(reinterpret_cast<Analysis::Utils::CHAR_PTR>(datas.data()), datas.size() * sizeof(T));
    outFile.close();
    return true;
}

#endif // TEST_MSPROF_CPP_ANALYSIS_UT_DOMAIN_PARSER_TEST_FAKE_GENERATOR_H
