# Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#cmake_minimum_required(VERSION 2.8)

project(tunner_stub)

FILE(GLOB_RECURSE STUB_SOURCE "${CMAKE_CURRENT_LIST_DIR}/src/" *.cc)

include_directories(${METADEF_DIR}/inc)
include_directories(${METADEF_DIR}/inc/external)
include_directories(${AIR_CODE_DIR}/inc)
include_directories(${AIR_CODE_DIR}/third_party/inc)

add_library(tunner_stub SHARED
    ${STUB_SOURCE}
)

target_compile_options(tunner_stub PRIVATE
    -g -O0
)

target_link_libraries(tunner_stub PRIVATE
    $<BUILD_INTERFACE:intf_pub>
)

target_include_directories(tunner_stub INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src
)
