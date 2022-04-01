# Copyright 2021 Huawei Technologies Co., Ltd
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ============================================================================


file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "*.cc" "*.CC" "*.cpp" "*.CPP" "*.c++")

add_executable(hybrid_model_async_exec_test ${SOURCES})

target_include_directories(hybrid_model_async_exec_test
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
        )

set_target_properties(hybrid_model_async_exec_test PROPERTIES CXX_STANDARD 17)

target_link_libraries(hybrid_model_async_exec_test PUBLIC 
        $<BUILD_INTERFACE:intf_pub>
        -Wl,--whole-archive
        graphengine
        -Wl,--no-whole-archive
        gtest gtest_main st_stubs ge_graph_dsl ge_running_env)

include(CTest)
enable_testing()
add_test(NAME test COMMAND hybrid_model_async_exec_test)