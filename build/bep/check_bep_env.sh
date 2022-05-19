#!/bin/bash
#Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
# ================================================================================

function bep_env_init() {
    # 使SECBEPKIT_HOME生效
    source /etc/profile
    # bep消除二进制
    local bep_env_config=./bep_env.conf
    # 检查BepKit预置环境
    local bep_sh=$(which bep_env.sh)
    echo "has bep sh :${bep_sh}"
    # 执行bep脚本
    if [ ! -d "${SECBEPKIT_HOME}" ] && [ ! -f "$bep_sh" ]; then
        echo "BepKit is not installed, Please install the tool and configure the env var \$SECBEPKIT_HOME"
    else
        source ${SECBEPKIT_HOME}/bep_env.sh -s $bep_env_config
        if [ $? -ne 0 ]; then
            echo "build bep failed!"
            exit 1
        else
            echo "build bep success."
        fi
    fi
}

bep_env_init