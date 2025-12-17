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
#ifndef ANALYSIS_DVVP_COMMON_SINGLETON_SINGLETON_H
#define ANALYSIS_DVVP_COMMON_SINGLETON_SINGLETON_H

#include <mutex>

namespace analysis {
namespace dvvp {
namespace common {
namespace singleton {
template<class T>
class Singleton {
public:
    static T *instance()
    {
        static T value;
        return &value;
    }

protected:
    Singleton()
    {}                                        // ctor hidden
    Singleton(Singleton const &);             // copy ctor hidden
    Singleton &operator=(Singleton const &);  // assign op. hidden
    virtual ~Singleton()
    {}  // dtor hidden
};
}  // namespace singleton
}  // namespace common
}  // namespace dvvp
}  // namespace analysis

#endif