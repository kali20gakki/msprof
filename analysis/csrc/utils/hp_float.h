/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hp_float.h
 * Description        : 高精度计算
 * Author             : msprof team
 * Creation Date      : 2024/1/10
 * *****************************************************************************
 */

#ifndef ANALYSIS_UTILS_HP_FLOAT_H
#define ANALYSIS_UTILS_HP_FLOAT_H

#include <string>
#include <vector>
namespace Analysis {
namespace Utils {
/*
HPFloat:高精度浮点类型
基于数量级（多项式）运算
支持运算:+,-,<<（乘以10的倍数）>>（乘以10的倍数）,+=,-=,-(负号),=
构造函数支持:string,HPFloat,整型与浮点型
除HPFloat构造时与参数精度相同，其他类型参数构造均为30位精度，EditLen方法可修改精度
使用方法：
定义&构造：
HPFloat a;
std::string str = "2.33";
double num = 2.33;
long long num2 = 233;
HPFloat b(a);
HPFloat c(str);
HPFloat d(num);
HPFloat e(num2);
运算：
a += b;
auto m = c - d;
*/
class HPFloat {
public:
    HPFloat();
    template <typename T>
    HPFloat(T value);
    HPFloat(const HPFloat &value);
    HPFloat &operator=(const HPFloat num);
    HPFloat &operator=(const long long num);
    HPFloat &operator=(const std::string num);
    HPFloat &operator=(const double num);
    HPFloat &operator=(const int num);
    template <typename T>
    HPFloat &operator=(const T num);
    // 由于重载运算符方法内部需要访问参数的私有变量，例如num_，所以需要使用友元函数
    friend HPFloat abs(const HPFloat &op1);
    friend HPFloat operator+(const HPFloat &op1, const HPFloat &op2);
    friend HPFloat operator-(const HPFloat &op1, const HPFloat &op2);
    // 临时方案，十进制左移位，相当于乘以10的n次幂
    friend HPFloat operator<<(const HPFloat &op, const int n);
    // 临时方案，十进制右移位，相当于除以10的n次幂
    friend HPFloat operator>>(const HPFloat &op, const int n);
    friend HPFloat operator-(const HPFloat &op);
    friend HPFloat operator+=(HPFloat &op1, const HPFloat &op2);
    friend HPFloat operator-=(HPFloat &op1, const HPFloat &op2);
    // 定义精度
    void SetPrecision(long long length);
    // 数据位数(整数位+小数位，不包含小数点）
    size_t Len() const;
    // 输出字符串格式
    std::string Str();
    // 输出double格式
    double Double();
private:
    // 清空数据，保留精度
    void Clear();
    // 输出有效长度，即num_中实际使用位数
    long long EffectiveLen() const;
    void Simple();
    // 整体退位,用于将最小数量级位数按照四舍五入舍弃，处理精度溢出使用
    void BackSpace();
    // 移位操作，用于位数对齐
    void MoveForward(long long step);
    void MoveBackward(long long step);
    // 返回最小数量级
    long long Mindig() const;
private:
    // 默认精度
    uint16_t defaultPrecision_{30};
    // 最大数量级,传入数据按数量级（多项式）表示，例如a1*10^2+a2*10^1+a3*10^0+a4*10^-1，本例中digit_=2
    long long digit_{0};
    // 数位,用于倒序存储数据
    std::vector<signed char> num_;
    // 符号
    bool symbol_{true};
};

}
}

#endif // ANALYSIS_UTILS_HP_FLOAT_H
