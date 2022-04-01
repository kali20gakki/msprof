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
# ---- Target : stub Host engine ----
add_library(ge_running_env STATIC ${SOURCES})
add_dependencies(ge_running_env nnengine local_engine)

target_include_directories(ge_running_env
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${AIR_CODE_DIR}
        ${METADEF_DIR}/inc
        )

target_compile_definitions(ge_running_env PRIVATE
        google=ascend_private
        FMK_SUPPORT_DUMP
        AIR_CODE_DIR="${AIR_CODE_DIR}"
        ENGINE_PATH="$<TARGET_FILE:nnengine>"
        OPS_KERNEL_LIB_PATH="$<TARGET_FILE:local_engine>"
        )

target_compile_options(ge_running_env PRIVATE
        -g --coverage -fprofile-arcs -ftest-coverage
        -Werror=format
        )

target_link_libraries(ge_running_env PUBLIC
        $<BUILD_INTERFACE:intf_pub> ge_running_env_inc graphengine_inc ${STUB_LIBS}
        )

set_target_properties(ge_running_env PROPERTIES CXX_STANDARD 17)
