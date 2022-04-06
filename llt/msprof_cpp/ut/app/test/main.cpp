#include <stdio.h>
#include "gtest/gtest.h"

std::vector<std::string> g_envpList;
int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    g_envpList.push_back("a=b");
    // Runs all tests using Google Test. 
    return RUN_ALL_TESTS(); 
}
