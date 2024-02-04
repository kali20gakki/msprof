/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hp_float.h
 * Description        : 高精度计算
 * Author             : msprof team
 * Creation Date      : 2024/1/10
 * *****************************************************************************
 */
#include "analysis/csrc/utils/hp_float.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace Analysis {
namespace Utils {
namespace {
const int ROUNDING_THRESHOLD = 5;
const int CARRY_THRESHOLD = 10;
const int ASCII_0 = 48;
const int ASCII_9 = 57;
const int POWER_10 = 10; // 根据用途分别定义10，增加代码可读性
}
// 主要用于msprof相关的大数运算
HPFloat::HPFloat()
{
    for (int i = 0; i < defaultPrecision_; i++) {
        num_.emplace_back(0);
    }
    symbol_ = false;
    dynamicLen_ = 0;
    digit_ = 0;
}

HPFloat::HPFloat(const HPFloat& value)
{
    *this = value;
}

size_t HPFloat::Len() const
{
    return num_.size();
}

// 清空数据，保留精度
void HPFloat::Clear()
{
    for (signed char &i : num_) {
        i = 0;
    }
    digit_ = 0;
    symbol_ = false;
    dynamicLen_ = 0;
}

void HPFloat::Simple()
{
    unsigned long index = 0;
    // 计算末尾的零
    while (index < num_.size() && num_[index] == 0) {
        index++;
    }
    // 无需化简，返回
    if (index == 0) {
        return;
    }
    if (index == num_.size()) {
        // 数量级和符号归零
        digit_ = 0;
        symbol_ = false;
        dynamicLen_ = 0;
    } else {
        // 后退去零
        MoveBackward(index);
    }
}

void HPFloat::SetPrecision(unsigned long length)
{
    unsigned long oriLen = num_.size();
    if (oriLen == length) {
        return;
    }
    if (length < minPrecision_) {
        return;
    }
    // 判断需要扩容还是收缩
    if (length > oriLen) {
        for (unsigned long i = 0; i < length - oriLen; i++) {
            num_.emplace_back(0);
        }
    } else {
        MoveForward(static_cast<long long>(oriLen - dynamicLen_ - 1));
        MoveBackward(static_cast<long long>(oriLen - length));
        num_.erase(num_.end() - oriLen + length, num_.end());
    }
    Simple();
}

void HPFloat::SetPrecision(long long length)
{
    if (length < 0) {
        throw std::invalid_argument("Invalid argument, SetPrecision function accepts only positive integer arguments");
    }
    SetPrecision(static_cast<unsigned long>(length));
}

void HPFloat::SetDynamicLen()
{
    // 如果精度全部占满，设置全长
    if (num_[num_.size() - 1] != 0) {
        dynamicLen_ = num_.size() - 1;
        return;
    }
    // 计算高位零的数量，这里i使用long long为了避免死循环
    for (auto i = static_cast<long long>(num_.size() - 1); i >= 0; i--) {
        if (num_[i] == 0) {
            continue;
        } else {
            dynamicLen_ = i;
            return;
        }
    }
}

long long HPFloat::MinDig() const
{
    return digit_ - static_cast<long long>(dynamicLen_);
}

long long HPFloat::TheoreticalMinDig() const
{
    return digit_ - static_cast<long long>(num_.size()) + 1;
}

void HPFloat::BackSpace()
{
    // 记录末位
    signed char tail = num_[0];
    unsigned long len = dynamicLen_;
    for (unsigned long i = 1; i <= len; i++) {
        num_[i - 1] = num_[i];
        num_[i] = 0;
    }
    // 四舍五入
    if (tail >= ROUNDING_THRESHOLD) {
        CoorAdd(1, 0);
    }
    dynamicLen_ -= 1;
}

void HPFloat::MoveForward(long long step)
{
    if (!step) {
        return;
    }
    // 负值则后移
    if (step < 0) {
        MoveBackward(-step);
        return;
    }
    unsigned long len = dynamicLen_;
    // 这里i使用long long为了避免死循环
    for (long long i = len; i >= 0; i--) {
        // 处理超出精度范围情况
        if (step + i >= num_.size()) {
            continue;
        }
        num_[step + i] = num_[i];
        num_[i] = 0;
    }
    SetDynamicLen();
}

void HPFloat::MoveBackward(long long step)
{
    if (!step) {
        return;
    }
    if (step < 0) {
        MoveForward(-step);
        return;
    }
    unsigned long len = dynamicLen_;
    // 移动大于精度，全清，保留数量级
    if (step > len + 1) {
        Clear();
        return;
    }
    signed char tail = num_[step -1];
    for (unsigned long i = step; i <= len; i++) {
        num_[i - step] = num_[i];
        num_[i] = 0;
    }
    if (tail >= ROUNDING_THRESHOLD) {
        CoorAdd(1, 0); // 四舍五入后进位
    }
    dynamicLen_ -= step;
}

bool HPFloat::SimplifyStr(std::string &str)
{
    bool symbol = false;
    if (str[0] == '-') {
        symbol = true;
        str.erase(0, 1);
    }
    long long index = 0;
    while (index < num_.size() && str[index] == '0') {
        index++;
    }
    if (index != 0) {
        str.erase(0, index); // 删掉多余的零
    }
    reverse(str.begin(), str.end()); // 倒序，和HPFloat的顺序相同
    return symbol;
}

void HPFloat::CoorAdd(signed char op, unsigned long psi)
{
    // 如果在已用精度之前，直接填充
    if (psi > dynamicLen_) {
        digit_ += static_cast<long long>(psi - dynamicLen_);
        num_[psi] = op;
        dynamicLen_ = psi;
        Simple();
        return;
    }
    num_[psi] += op;
    // 是否进位
    if (num_[psi] >= CARRY_THRESHOLD) {
        // 如果精度已满
        if (psi == num_.size() - 1) {
            // 退格让出一位精度
            BackSpace();
            // 这里不会出现psi-1小于0的情况
            num_[psi - 1] -= CARRY_THRESHOLD;
            digit_++;
            num_[psi] = 1;
            // 还原因BackSpace造成的dynamicLen_变化
            dynamicLen_ = psi;
        } else {
            CoorAdd(1, psi + 1);
            num_[psi] -= CARRY_THRESHOLD;
        }
    }
    Simple();
}

HPFloat operator-(const HPFloat &op)
{
    HPFloat ans(op);
    ans.symbol_ = !op.symbol_;
    return ans;
}

std::string HPFloat::Str()
{
    std::string str;
    if (symbol_) {
        str+='-';
    }
    // 是否需要补零
    if (digit_ < 0) {
        str+= "0.";
        for (long long i = -1; i > digit_; i--) {
            str+= '0'; // 高位补零
        }
    }
    // 这里i使用long long为了避免死循环
    for (long long i = dynamicLen_; i >= 0; i--) {
        str+= static_cast<char>(num_[i] + ASCII_0); // 循环输出
        if (static_cast<long long>(dynamicLen_ - i) == digit_ && i != 0) {
            str+= '.'; // 小数点
        }
    }
    // 低位补零
    for (long long i = MinDig(); i > 0; i--) {
        str+= '0';
    }

    return str;
}

double HPFloat::Double()
{
    // 不建议输出Double格式，因为转Double过程本身会丢失精度
    double ans = 0;
    // 这里i需要定义为long long，如果定义为unsigned long pow参数会发生“整数提升”问题指数变为unsigned long
    for (long long i = 0; i <= dynamicLen_; i++) {
        double temp = num_[i];
        temp *= pow(POWER_10, MinDig() + i);
        ans += temp; // 循环加法输出
    }
    if (this->symbol_) {
        ans *= -1;
    }
    return ans;
}

HPFloat &HPFloat::operator=(const HPFloat &op)
{
    if (this == &op) {
        return *this;
    }
    Clear();
    SetPrecision(op.Len());
    unsigned long len = op.dynamicLen_;
    for (unsigned long i = 0; i <= len; ++i) {
        this->num_[i] = op.num_[i];
    }
    digit_ = op.digit_;
    symbol_ = op.symbol_;
    dynamicLen_ = op.dynamicLen_;
    Simple();
    return *this;
}

HPFloat &HPFloat::operator=(unsigned long long op)
{
    Clear();
    // 为了处理num数量级超过用例最大精度溢出情况，记录原始精度同时扩大当前精度
    unsigned long oriPrecision = num_.size();
    // 取正
    unsigned long long num = op;
    unsigned long n = 0;
    unsigned long long tmp = 1;
    while (tmp <= num) {
        // 计算数量级
        n++;
        num /= POWER_10;
    }
    SetPrecision(std::max(oriPrecision, n));
    for (unsigned long i = 0; i <= n; i++) {
        // 逐个填入
        num_[i] = op % POWER_10;
        op = (op - op % POWER_10) / POWER_10;
    }
    digit_ = static_cast<long long>(n - 1);
    dynamicLen_ = n - 1;
    symbol_ = false;
    // 还原精度，正确处理精度丢失
    SetPrecision(oriPrecision);
    Simple();
    return *this;
}

HPFloat &HPFloat::operator=(long long op)
{
    Clear();
    // 取正
    unsigned long long num = (op > 0) ? op : -op;
    *this = num;
    symbol_ = op < 0;
    Simple();
    return *this;
}

HPFloat &HPFloat::operator=(const int op)
{
    return *this = static_cast<long long>(op);
}

HPFloat &HPFloat::operator=(const unsigned int op)
{
    return *this = static_cast<unsigned long long>(op);
}

HPFloat &HPFloat::operator=(const long op)
{
    return *this = static_cast<long long>(op);
}

HPFloat &HPFloat::operator=(const unsigned long op)
{
    return *this = static_cast<unsigned long long >(op);
}

HPFloat &HPFloat::operator=(const std::string &num)
{
    Clear();
    if (num.empty()) {
        return *this;
    }
    unsigned long oriPrecision = num_.size(); // 为了处理num数量级超过用例最大精度溢出情况，记录原始精度同时扩大当前精度
    SetPrecision(std::max(oriPrecision, num.size()));
    std::string op = num;
    symbol_ = SimplifyStr(op);
    bool ifDot = false;
    unsigned long dotLoc = 0;
    for (unsigned long i = 0; i < op.size(); i++) {
        if (op[i] == '.') { // 记录小数点的位置
            if (ifDot) {
                throw std::invalid_argument("Invalid character encountered");
            }
            ifDot = true;
            dotLoc = i;
            continue;
        }
        if (op[i] < ASCII_0 || op[i] > ASCII_9) { // 遇到异常数据，抛出错误,行为与库函数一致,48:'0',57:'9'
            throw std::invalid_argument("Invalid character encountered");
        }
        num_[i - ifDot] = op[i] - ASCII_0; // 录入数据
    }
    if (op[op.size() - 1] == '.') { // 如果小数点在末尾
        op.erase(op.size() - 1, 1);
        long long zeroNum = 0;
        while (zeroNum <= op.size() - 1 && op[op.size() - 1 - zeroNum] == '0') {
            zeroNum++;
        }
        digit_ += -zeroNum - 1; // 算出数量级
    } else {
        digit_ += static_cast<long long>(op.size() - 1 - dotLoc - ifDot); // 算出数量级
    }
    SetDynamicLen();
    SetPrecision(oriPrecision);
    Simple(); // 化简返回
    return *this;
}

HPFloat &HPFloat::operator=(const char* num)
{
    std::string tmp = num;
    *this = tmp;
    return *this;
}

HPFloat abs(const HPFloat &op)
{
    HPFloat ans(op);
    ans.symbol_ = false;
    return ans;
}

bool operator==(const HPFloat &op1, const HPFloat &op2)
{ // 先判断符号、数量级、已用精度
    if (!(op1.digit_ == op2.digit_ && op1.dynamicLen_ == op2.dynamicLen_ && op1.symbol_ == op2.symbol_)) {
        return false;
    }
    unsigned long length = op1.dynamicLen_;
    for (unsigned long i = 0; i <= length; i++) {
        if (op1.num_[i] != op2.num_[i]) { // 数位一一比较
            return false;
        }
    }
    return true; // 全部通过，输出1
}

bool operator!=(const HPFloat &op1, const HPFloat &op2)
{
    return !(op1 == op2);
}

bool operator>(const HPFloat &op1, const HPFloat &op2)
{
    if (op1 == ZERO && op2 == ZERO) {
        return false;
    }
    if (op1 == ZERO && op2 != ZERO) {
        return op2.symbol_;
    }
    if (op2 == ZERO) {
        return !op1.symbol_;
    }
    // 符号不同情况
    if (op1.symbol_ != op2.symbol_) {
        return op2.symbol_;
    }
    if (op1.digit_ != op2.digit_) {
        return (op1.digit_ > op2.digit_) && !op1.symbol_;
    }
    unsigned long len1 = op1.dynamicLen_;
    unsigned long len2 = op2.dynamicLen_;
    unsigned long minLen = std::min(len1, len2);
    // 从最高数量级开始比较
    for (unsigned long i = 0; i <= minLen; i++) {
        if (op1.num_[len1- i] != op2.num_[len2 - i]) {
            return (op1.num_[len1 - i] > op2.num_[len2 - i]) && !op1.symbol_;
        }
    }
    // 公有数量级内两者相等，比较有效数量级
    if (len1 > len2) {
        return !op1.symbol_;
    }
    return op1.symbol_;
}

bool operator>=(const HPFloat &op1, const HPFloat &op2)
{
    return (op1 > op2 || op1 == op2);
}

bool operator<(const HPFloat &op1, const HPFloat &op2)
{
    return !(op1 >= op2);
}

bool operator<=(const HPFloat &op1, const HPFloat &op2)
{
    return !(op1 > op2);
}

}
}