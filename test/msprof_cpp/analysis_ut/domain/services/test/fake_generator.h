/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

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
