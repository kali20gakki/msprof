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
#include <iostream>
#include "gtest/gtest.h"
#include "securec.h"
#include "secodeFuzz.h"
#include "prof_api_test.h"
 
int g_fuzzRunTime = 1000000;
 
GTEST_API_ int main(int argc, char **argv)
{
    DT_Set_Report_Path("./");
    DT_SetEnableFork(1);
    if (argc == 2) { // 2 input with fuzz run time
        if (sscanf_s(argv[1], "%d", &g_fuzzRunTime) == -1) {
            std::cout << "failed to get fuzz run time" << std::endl;
            return -1;
        }
    }
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}