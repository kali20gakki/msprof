/* ******************************************************************************
版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : acsq_log_parser_item_utest.cpp
 * Description        : acsq_log_parser_item_utest类UT
 * Author             : msprof team
 * Creation Date      : 2025/11/24
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "analysis/csrc/domain/services/parser/parser_item/acsq_log_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"

namespace Analysis {
    using namespace testing;
    using namespace Analysis::Domain;

    class AcsqLogParserItemUtest : public Test {
    protected:
        void SetUp() override
        {
        }
    };

    TEST_F(AcsqLogParserItemUtest, TestAcsqLogParseItemV6ShouldReturnErrorWhenSizeNotMatch)
    {
        AcsqLogV6 acsqLogV6;
        HalLogData halLogData;
        ASSERT_EQ(AcsqLogParseItem_V6(reinterpret_cast<uint8_t*>(&acsqLogV6),
            99, reinterpret_cast<uint8_t*>(&halLogData), false), Analysis::PARSER_ERROR_SIZE_MISMATCH);
    }

    TEST_F(AcsqLogParserItemUtest, TestAcsqLogParseItemV6Success)
    {
        AcsqLogV6 acsqLogV6;
        acsqLogV6.cnt = 9;
        HalLogData halLogData;
        ASSERT_EQ(AcsqLogParseItem_V6(reinterpret_cast<uint8_t*>(&acsqLogV6),
            32, reinterpret_cast<uint8_t*>(&halLogData), false), 9);
    }
}
