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

#ifndef ANALYSIS_UTILS_SINGLETON_H
#define ANALYSIS_UTILS_SINGLETON_H

namespace Analysis {
namespace Utils {
template <typename T>
class Singleton {
public:
    static T& GetInstance()
    {
        static T instance;
        return instance;
    }

protected:
    Singleton() = default;
    virtual ~Singleton() = default;
    Singleton(const Singleton & s) = delete;
    Singleton& operator=(const Singleton & s) = delete;
};  // class Singleton
}  // namespace Utils
}  // namespace Analysis

#endif // ANALYSIS_UTILS_SINGLETON_H
