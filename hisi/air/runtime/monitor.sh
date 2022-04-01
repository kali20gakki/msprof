#! /bin/bash
# Perform monitor for cann/air
# Copyright (c) Huawei Technologies Co., Ltd. 2021. All right reserved

current_path=$(dirname "$(readlink -f ${BASH_SOURCE[0]})")

if [[ "$1" == "status" ]]
then
  cat ./client.json
elif [[ "$1" == "start" ]]
then
  $current_path/grpc_server
elif [[ "$1" == "stop" ]]
then
  ps -ef | grep grpc_server | grep -v grep | awk '{print $2}' | xargs kill
  ps -ef | grep queue_schedule | grep -v grep | awk '{print $2}' | xargs kill
else
  echo "invalid input command."
fi