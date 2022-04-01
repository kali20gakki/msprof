#!/bin/bash
sleep 1
MDC_ROOT_DIR=/usr/lib/mdc
toolsDir=$MDC_ROOT_DIR/driver/tools

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MDC_ROOT_DIR/lib64:$MDC_ROOT_DIR/mdc_toolkit
nohup $toolsDir/ada &

sleep 0.05
