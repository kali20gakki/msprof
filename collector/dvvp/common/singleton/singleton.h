/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
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