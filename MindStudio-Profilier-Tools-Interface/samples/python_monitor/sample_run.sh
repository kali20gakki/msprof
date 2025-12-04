#!/bin/bash
export LD_PRELOAD=${ASCEND_HOME_PATH}/lib64/libmspti.so
torchrun --nproc_per_node=8 python_monitor.py