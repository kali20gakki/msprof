/**
 * @file singleton.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef ADX_COMMON_SINGLETON_SINGLETON_H
#define ADX_COMMON_SINGLETON_SINGLETON_H

namespace Adx {
namespace Common {
namespace Singleton {
template<class T>
class Singleton {
public:
    static T& Instance()
    {
        static T instance;
        return instance;
    }
    virtual ~Singleton()     {}                           // dtor hidden
    Singleton(Singleton const &) = delete;                // copy ctor hidden
    Singleton &operator=(Singleton const &) = delete;     // assign op. hidden
protected:
    Singleton()    {}                                     // ctor hidden
};
}  // namespace singleton
}  // namespace common
}  // namespace analysis

#endif
