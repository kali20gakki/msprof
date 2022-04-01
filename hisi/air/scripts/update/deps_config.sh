#!/bin/bash
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
set -e

SERVER_CONFIG_FILE=${PROJECT_HOME}/scripts/config/server_config.sh

[ -e "$SERVER_CONFIG_FILE" ] || {
    echo "You have not config dependencies account info first !!!!!"
    ${PROJECT_HOME}/scripts/config/ge_config.sh -h
    exit 1;
}

source scripts/config/server_config.sh

CPU_ARCH=ubuntu18.04.x86_64
DRIVER_VERSION=20.2.0
CHIP_NAME=A800-9010
PRODUCT_VERSION=driver_C76_TR5

DRIVER_NAME=npu-driver
DRIVER_RUN_NAME=${CHIP_NAME}-${DRIVER_NAME}_${DRIVER_VERSION}_ubuntu18.04-x86_64.run

DEV_TOOLS_VERSION=1.80.t22.0.b220
export COMPILER_RUN_NAME=CANN-compiler-${DEV_TOOLS_VERSION}-ubuntu18.04.x86_64.run
export RUNTIME_RUN_NAME=CANN-runtime-${DEV_TOOLS_VERSION}-ubuntu18.04.x86_64.run
export OPP_RUN_NAME=CANN-opp-${DEV_TOOLS_VERSION}-ubuntu18.04.x86_64.run

DEV_TOOLS_PACKAGE_NAME=ai_cann_x86
DEV_TOOLS_PACKAGE=${DEV_TOOLS_PACKAGE_NAME}.tar.gz

export DRIVER_URL=${SERVER_PATH}/${PRODUCT_VERSION}/${DRIVER_RUN_NAME}
export DEV_TOOLS_URL=${SERVER_PATH}/20211211/${DEV_TOOLS_PACKAGE}

set +e