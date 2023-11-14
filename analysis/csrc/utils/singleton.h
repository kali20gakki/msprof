/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : singleton.h
 * Description        : 单例类模板
 * Author             : msprof team
 * Creation Date      : 2023/11/11
 * *****************************************************************************
 */

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
