#!/bin/bash
sleep 1
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/lib64
nohup /var/adda &

sleep 0.05
