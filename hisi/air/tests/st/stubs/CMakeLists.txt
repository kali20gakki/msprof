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


file(GLOB_RECURSE UTILS_SRCS CONFIGURE_DEPENDS *.cc)

add_library(st_stubs STATIC ${UTILS_SRCS})

target_include_directories(st_stubs PUBLIC
        ./
        ${AIR_CODE_DIR}
        )

target_link_libraries(st_stubs PUBLIC
        $<BUILD_INTERFACE:intf_pub>)

target_compile_definitions(st_stubs PRIVATE
        google=ascend_private
        )

set_target_properties(st_stubs PROPERTIES CXX_STANDARD 17)

target_link_libraries(st_stubs PUBLIC ge_graph_dsl ge_running_env graphengine gtest gmock gmock_main)
