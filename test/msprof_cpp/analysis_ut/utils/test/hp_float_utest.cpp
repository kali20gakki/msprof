/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hp_float_utest.cpp
 * Description        : 高精度计算 UT
 * Author             : msprof team
 * Creation Date      : 2024/1/25
 * *****************************************************************************
 */

#include <stdexcept>
#include "gtest/gtest.h"
#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/utils/hp_float.h"

using namespace Analysis::Utils;

class HPFloatUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

// 测试构造函数
TEST_F(HPFloatUTest, TestConstructorSuccessWhenNoInput)
{
    HPFloat res;
    EXPECT_EQ(res.Str(), "0");
}

TEST_F(HPFloatUTest, TestConstructorSuccessWhenInputIsChar)
{
    char input = 123;
    HPFloat res{input};
    EXPECT_EQ(res.Str(), std::to_string(input));
}

TEST_F(HPFloatUTest, TestConstructorSuccessWhenInputIsUnsignedChar)
{
    unsigned char input = 255;
    HPFloat res{input};
    EXPECT_EQ(res.Str(), std::to_string(input));
}

TEST_F(HPFloatUTest, TestConstructorSuccessWhenInputIsInt)
{
    int input = -123456789;
    HPFloat res{input};
    EXPECT_EQ(res.Str(), std::to_string(input));
}

TEST_F(HPFloatUTest, TestConstructorSuccessWhenInputIsUnsignedInt)
{
    unsigned int input = 123456789;
    HPFloat res{input};
    EXPECT_EQ(res.Str(), std::to_string(input));
}

TEST_F(HPFloatUTest, TestConstructorSuccessWhenInputIsShort)
{
    short input = -12345;
    HPFloat res{input};
    EXPECT_EQ(res.Str(), std::to_string(input));
}

TEST_F(HPFloatUTest, TestConstructorSuccessWhenInputIsUnsignedShort)
{
    unsigned short input = 12345;
    HPFloat res{input};
    EXPECT_EQ(res.Str(), std::to_string(input));
}

TEST_F(HPFloatUTest, TestConstructorSuccessWhenInputIsLong)
{
    long input = 1234567898765432123;
    HPFloat res{input};
    EXPECT_EQ(res.Str(), std::to_string(input));
}

TEST_F(HPFloatUTest, TestConstructorSuccessWhenInputIsUnsignedLong)
{
    unsigned long input = 1234567898765432123;
    HPFloat res{input};
    EXPECT_EQ(res.Str(), std::to_string(input));
}

TEST_F(HPFloatUTest, TestConstructorSuccessWhenInputIsLongLong)
{
    long long input = 1234567898765432123;
    HPFloat res{input};
    EXPECT_EQ(res.Str(), std::to_string(input));
}

TEST_F(HPFloatUTest, TestConstructorSuccessWhenInputIsUnsignedLongLong)
{
    unsigned long long input = 1234567898765432123;
    HPFloat res{input};
    EXPECT_EQ(res.Str(), std::to_string(input));
}

TEST_F(HPFloatUTest, TestConstructorSuccessWhenInputIsFloat)
{
    float input = 324.123;
    HPFloat res{input};
    EXPECT_EQ(res.Str(), std::to_string(input));
}

TEST_F(HPFloatUTest, TestConstructorSuccessWhenInputIsDouble)
{
    double input = 324.123456789;
    HPFloat res{input};
    EXPECT_EQ(res.Str(), std::to_string(input));
}

TEST_F(HPFloatUTest, TestConstructorSuccessWhenInputIsStr)
{
    std::string input = "12345678987.12345678987";
    HPFloat res{input};
    EXPECT_EQ(res.Str(), input);
}

TEST_F(HPFloatUTest, TestConstructorSuccessWhenInputIsCharPoint)
{
    HPFloat res{"12345678987.12345678987"};
    EXPECT_EQ(res.Str(), "12345678987.12345678987");
}

TEST_F(HPFloatUTest, TestConstructorSuccessWhenInputIsHPFloat)
{
    HPFloat input{"12345678987.12345678987"};
    HPFloat res{input};
    EXPECT_EQ(res.Str(), input.Str());
    HPFloat res2 = input;
    EXPECT_EQ(res2.Str(), input.Str());
    // 测试复制构造函数，参数为自身
    res2 = res2;
    EXPECT_EQ(res2.Str(), input.Str());
}

TEST_F(HPFloatUTest, TestConstructorFailedWhenInputIsInvalidStr)
{
    std::string input = "12345678987.12345678abc";
    try {
        HPFloat num{input};
    } catch (const std::invalid_argument &err) {
        EXPECT_STREQ(err.what(), "Invalid character encountered");
    }
}

// 测试比较运算符
TEST_F(HPFloatUTest, TestEqual)
{
    long testNum = 987654322;
    long long testNum2 = 9876543220;
    unsigned long testNum3 = 987654328;
    // 符号，数量级，已用精度相等情况
    HPFloat num1{987654321};
    HPFloat num2{987654321};
    EXPECT_TRUE(num1 == num2);
    num2 = testNum;
    EXPECT_FALSE(num1 == num2);
    // 符号不同情况
    num2 = -num2;
    EXPECT_FALSE(num1 == num2);
    // 数量级不同情况
    num2 = testNum2;
    EXPECT_FALSE(num1 == num2);
    // 已用精度不同情况
    num2 = testNum3;
    EXPECT_FALSE(num1 == num2);
}

TEST_F(HPFloatUTest, TestNotEqual)
{
    HPFloat num1{987654321};
    HPFloat num2{987654321};
    HPFloat num3{987654322};
    EXPECT_TRUE(num1 != num3);
    EXPECT_FALSE(num1 != num2);
}

TEST_F(HPFloatUTest, TestGreaterThan)
{
    // 非零，正数，公有数量级不相等
    HPFloat num1{987654321};
    HPFloat num2{987654233};
    EXPECT_TRUE(num1 > num2);
    EXPECT_FALSE(num2 > num1);
    num1 = 233; // 233， 公有数量级33
    num2 = 12; // 12，公有数量级12
    EXPECT_TRUE(num1 > num2);
    // 非零，正数，公有数量级相等情况
    num2 = 33; // 33，公有数量级33
    EXPECT_TRUE(num1 > num2);
    // A或B为0情况
    num2 = 0;
    EXPECT_TRUE(num1 > num2);
    num1 = 0;
    EXPECT_FALSE(num2 > num1);
    // 符号不同情况
    num1 = 3; // 3，正数
    num2 = -1; // -1，负数
    EXPECT_TRUE(num1 > num2);
    num1 = -8; // -8，两个负数比较
    EXPECT_FALSE(num1 > num2);
    num1 = -11; // -11，两个负数比较
    num2 = 3; // 3，正数和负数比较
    EXPECT_FALSE(num1 > num2);
}

TEST_F(HPFloatUTest, TestGreaterThanOrEqual)
{
    HPFloat num1{987654321};
    HPFloat num2{987654321};
    EXPECT_TRUE(num1 >= num2); // 验证true情况
    num2 = 9876543210; // 9876543210，验证false情况
    EXPECT_FALSE(num1 >= num2);
}

TEST_F(HPFloatUTest, TestLessThan)
{
    HPFloat num1{987654321};
    HPFloat num2{987654233};
    EXPECT_FALSE(num1 < num2);
    EXPECT_TRUE(num2 < num1);
}

TEST_F(HPFloatUTest, TestLessThanOrEqual)
{
    HPFloat num1{987654321};
    HPFloat num2{987654321};
    EXPECT_TRUE(num1 <= num2); // 验证true情况
    num2 = 123456; // 123456，验证false情况
    EXPECT_FALSE(num1 <= num2);
}

TEST_F(HPFloatUTest, TestAbs)
{
    HPFloat num1{-987654321};
    HPFloat num2{987654321};
    EXPECT_TRUE(abs(num1) == num2);
}

TEST_F(HPFloatUTest, TestMinus)
{
    HPFloat num1{-987654321};
    HPFloat num2 = -num1;
    auto num3 = num1.Double() + num2.Double();
    ASSERT_FLOAT_EQ(num3, 0);
}

TEST_F(HPFloatUTest, TestSetPrecisionSuccessWhenInputIsValid)
{
    int precision30 = 30;
    unsigned long precision50 = 50;
    long long precision8 = 8;
    std::string input = "987654321";
    HPFloat num{input};
    EXPECT_EQ(num.Len(), precision30);
    num.SetPrecision(precision50);
    EXPECT_EQ(num.Len(), precision50);
    EXPECT_EQ(num.Str(), input);
    num.SetPrecision(precision8);
    EXPECT_EQ(num.Len(), precision8);
    // 设置精度低于有效精度，丢失设置精度外数据，设置精度最小位四舍五入，本例中1<5舍去
    EXPECT_EQ(num.Str(), "987654320");
    std::string input2 = "123456789";
    num = input2;
    EXPECT_EQ(num.Len(), precision8);
    // 设置精度低于有效精度，丢失设置精度外数据，设置精度最小位四舍五入，本例中9>=5进位
    EXPECT_EQ(num.Str(), "123456790");
}

TEST_F(HPFloatUTest, TestSetPrecisionFailedWhenInputIsInvalid)
{
    std::string input = "987654321";
    int precision = -3;
    HPFloat num{input};
    try {
        num.SetPrecision(precision);
    } catch (const std::invalid_argument &err) {
        EXPECT_STREQ(err.what(), "Invalid argument, SetPrecision function accepts only positive integer arguments");
    }
}

TEST_F(HPFloatUTest, TestPrecisionSuccessAfterMultipleAssignmentAndSetPrecisionOperation)
{
    unsigned int precision30 = 30;
    long long precision5 = 5;
    double input = -987654.321;
    HPFloat num{input};
    EXPECT_EQ(num.Len(), precision30);
    EXPECT_EQ(num.Double(), input);
    num = 12; // 12，改变数量级赋值
    EXPECT_EQ(num.Len(), precision30);
    EXPECT_EQ(num.Str(), "12");
    num = "-987654321.23";
    EXPECT_EQ(num.Len(), precision30);
    EXPECT_EQ(num.Str(), "-987654321.23");
    num.SetPrecision(precision5);
    EXPECT_EQ(num.Len(), precision5);
    // 丢失4321.23，符合预期
    EXPECT_EQ(num.Str(), "-987650000");
    num = "123456789.233";
    // 精度不会因为赋值操作扩大
    EXPECT_EQ(num.Len(), precision5);
    // 赋值一个数量级更大的值，四舍五入丢失精度范围外数据
    EXPECT_EQ(num.Str(), "123460000");
    // 测试整形和浮点数，正确处理溢出
    num = -98765432; // -98765432，测试负整数
    EXPECT_EQ(num.Len(), precision5);
    EXPECT_EQ(num.Str(), "-98765000");
    num = 12.987654; // 12.987654，测试正浮点数
    EXPECT_EQ(num.Len(), precision5);
    EXPECT_EQ(num.Str(), "12.988");
    // HPFloat赋值操作，设计中这个操作会继承参数精度
    HPFloat tmp{input};
    num = tmp;
    EXPECT_EQ(num.Len(), precision30);
    EXPECT_EQ(num.Double(), input);
}

TEST_F(HPFloatUTest, TestDouble)
{
    double input = -987654.321;
    HPFloat num{input};
    EXPECT_EQ(num.Double(), input);
}