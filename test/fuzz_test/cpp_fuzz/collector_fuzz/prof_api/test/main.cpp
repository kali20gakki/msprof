/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 */
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