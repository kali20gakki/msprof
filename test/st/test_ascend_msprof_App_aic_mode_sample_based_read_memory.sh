#!/bin/bash

file_name=$(basename $0 .sh)

cur_dir="$(cd "$(dirname "$0")"; pwd)"

MSPROF_OPTS="--ascendcl=on --model-execution=on --runtime-api=on --task-time=on --ai-core=on --l2=on " \
            "--aic-freq=100 --aicpu=on --sys-hardware-mem=on --aic-mode=sample-based --llc-profiling=read " \
            "--aic-metrics=Memory"
MODEL_NAME="resnet50"

"$DIR/test_base_msprof.sh" file_name ${MSPROF_OPTS} ${MODEL_NAME}