/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : meta_data_processor.cpp
 * Description        : MetaDataProcessor UT
 * Author             : msprof team
 * Creation Date      : 2024/05/14
 * *****************************************************************************
 */

#include "analysis/csrc/viewer/database/finals/meta_data_processor.h"

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Viewer::Database;
using namespace Analysis::Utils;
using DataFormat = std::vector<std::tuple<std::string, std::string>>;

const std::string DETA_DATA_DIR = "./meta_data";
const std::string MSPROF = "msprof.db";
const std::string DB_PATH = File::PathJoin({DETA_DATA_DIR, MSPROF});

class MetaDataProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(DETA_DATA_DIR));
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(DETA_DATA_DIR, 0));
    }

    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
        if (File::Exist(DB_PATH)) {
            EXPECT_TRUE(File::DeleteFile(DB_PATH));
        }
    }
};

void CheckExportMetaData(const std::shared_ptr<DBRunner> &dbRunner)
{
    DataFormat checkData;
    std::string sqlStr = "SELECT name, value FROM " + TABLE_NAME_META_DATA;
    const uint16_t expectNum = 4;
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
    EXPECT_EQ(expectNum, checkData.size());
}

TEST_F(MetaDataProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    // 执行meta data processor
    auto processor = MetaDataProcessor(DB_PATH);
    EXPECT_TRUE(processor.Run());

    // 校验processor生成的若干表及内容
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, DB_PATH);
    ASSERT_NE(dbRunner, nullptr);
    CheckExportMetaData(dbRunner);
}

TEST_F(MetaDataProcessorUTest, TestRunShouldReturnFalseWhenReserveFailedThenDataIsEmpty)
{
    using TempT = std::tuple<std::string, std::string>;
    MOCKER_CPP(&std::vector<TempT>::reserve).stubs().will(throws(std::bad_alloc()));
    auto processor = MetaDataProcessor(DB_PATH);
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&std::vector<TempT>::reserve).reset();
}
