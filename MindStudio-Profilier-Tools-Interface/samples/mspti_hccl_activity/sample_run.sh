#!/bin/bash

# 检查 AscendHome 环境变量是否设置
if [ -z "$ASCEND_HOME_PATH" ]; then
  echo "Error: ASCEND_HOME_PATH environment variable is not set."
  echo "Please set the AscendHome variable by \"source set_env.sh\""
  exit 1
else
  echo "AscendHome is set to: $ASCEND_HOME_PATH"
fi

mkdir ./bin
cd bin
cmake ..
make

export LD_PRELOAD=${ASCEND_HOME_PATH}/lib64/libmspti.so
./mspti_hccl_activity_test