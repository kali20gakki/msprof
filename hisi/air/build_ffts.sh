#!/bin/bash
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

set -e
BASEPATH=$(cd "$(dirname $0)"; pwd)
OUTPUT_PATH="${BASEPATH}/output"
export BUILD_PATH="${BASEPATH}/build/"
export ASCEND_CUSTOM_PATH="/mnt/d/Ascend"
export ALL_IN_ONE_ENABLE=1

# print usage message
usage()
{
  echo "Usage:"
  echo "sh build.sh [-h] [-u] [-s] [-p] [-r] [-j[n]]"
  echo ""
  echo "Options:"
  echo "    -h Print usage"
  echo "    -u Build ut"
  echo "    -s Build st"
  echo "    -c Build ut with coverage tag"
  echo "    -r Run ut or st after compile ut or st"
  echo "    -p Build inference, train or all, default is train"
  echo "    -j[n] Set the number of threads used for building AIR, default is 8"
  echo "to be continued ..."
}

# check value of input is 'on' or 'off'
# usage: check_on_off arg_value arg_name
check_on_off()
{
  if [[ "X$1" != "Xon" && "X$1" != "Xoff" ]]; then
    echo "Invalid value $1 for option -$2"
    usage
    exit 1
  fi
}

# parse and set options
checkopts()
{
  VERBOSE=""
  THREAD_NUM=8
  ENABLE_FFTS_UT="off"
  ENABLE_FFTS_ST="off"
  ENABLE_FFTS_LLT="off"
  ENABLE_LLT_COV="off"
  ENABLE_RUN_LLT="off"
  PLATFORM="train"
  # Process the options
  while getopts 'ushj:p:cr' opt
  do
    OPTARG=$(echo ${OPTARG} | tr '[A-Z]' '[a-z]')
    case "${opt}" in
      u)
        ENABLE_FFTS_UT="on"
        ENABLE_FFTS_LLT="on"
        ;;
      s)
        ENABLE_FFTS_ST="on"
        ENABLE_FFTS_LLT="on"
        ;;
      h)
        usage
        exit 0
        ;;
      j)
        THREAD_NUM=$OPTARG
        ;;
      p)
        PLATFORM=$OPTARG
        ;;
      c)
        ENABLE_LLT_COV="on"
        ;;
      r)
        ENABLE_RUN_LLT="on"
        ;;
      *)
        echo "Undefined option: ${opt}"
        usage
        exit 1
    esac
  done
}

mk_dir() {
    local create_dir="$1"  # the target to make

    mkdir -pv "${create_dir}"
    echo "created ${create_dir}"
}

build_ffts()
{
  echo "create build directory and build AIR";

  mk_dir "${BUILD_PATH}"
  cd "${BUILD_PATH}"

  CMAKE_ARGS="-DBUILD_PATH=$BUILD_PATH"
  if [[ "X$ENABLE_FFTS_LLT" = "Xon" ]]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DENABLE_FFTS_LLT=ON"
  fi
  if [[ "X$ENABLE_LLT_COV" = "Xon" ]]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DENABLE_LLT_COV=ON"
  fi

  CMAKE_ARGS="${CMAKE_ARGS} -DBUILD_OPEN_PROJECT=True -DENABLE_OPEN_SRC=True"
  CMAKE_ARGS="${CMAKE_ARGS} -DCMAKE_INSTALL_PREFIX=${OUTPUT_PATH} -DPLATFORM=${PLATFORM} -DPRODUCT=${PRODUCT}"
  echo "${CMAKE_ARGS}"
  cmake ${CMAKE_ARGS} ..
  if [ $? -ne 0 ]
  then
    echo "execute command: cmake ${CMAKE_ARGS} .. failed."
    return 1
  fi

  TARGET="ffts atc_ffts"
  if [ "x${PLATFORM}" = "xtrain" ]
  then
    TARGET="ffts"
  elif [ "x${PLATFORM}" = "xinference" ]
  then
    TARGET="atc_ffts"
  fi

  if [ "X$ENABLE_FFTS_UT" = "Xon" ];then
    TARGET="ffts_ut"
    make ${VERBOSE} ${TARGET} -j${THREAD_NUM}
  elif [ "X$ENABLE_FFTS_ST" = "Xon" ];then
    TARGET="ffts_st"
    make ${VERBOSE} ${TARGET} -j${THREAD_NUM}
  else
    make ${VERBOSE} ${TARGET} -j${THREAD_NUM} && make install
  fi

  if [ $? -ne 0 ]
  then
    echo "execute command: make ${VERBOSE} ${TARGET} -j${THREAD_NUM} && make install failed."
    return 1
  fi
  echo "AIR build success!"
}

run_llt()
{
  if [ "X$ENABLE_RUN_LLT" != "Xon" ]
  then
    return 0
  fi

  cd "${BASEPATH}"
  if [ "X$ENABLE_FFTS_UT" = "Xon" ];then
    cd "${BASEPATH}/../"
    ./air/build/test/engines/fftseng/ut/ffts_ut
    if [ "X$ENABLE_LLT_COV" = "Xon" ]
    then
      cd "${BASEPATH}"
      mkdir "${BASEPATH}/cov_ffts_ut/"
      lcov -c -d build/test/engines/fftseng/ut -o cov_ffts_ut/tmp.info
      lcov -r cov_ffts_ut/tmp.info '*/output/*' '*/build/opensrc/*' '*/build/proto/*' '*/third_party/*' '*/test/*' '/usr/local/*' '/usr/include/*'  -o cov_ffts_ut/coverage.info
      cd "${BASEPATH}/cov_ffts_ut/"
      genhtml coverage.info
      cd "${BASEPATH}"
    fi
  elif [ "X$ENABLE_FFTS_ST" = "Xon" ];then
    cd "${BASEPATH}/../"
    ./air/build/test/engines/fftseng/st/ffts_st
    if [ "X$ENABLE_LLT_COV" = "Xon" ]
    then
      cd "${BASEPATH}"
      mkdir "${BASEPATH}/cov_ffts_st/"
      lcov -c -d build/test/engines/fftseng/st -o cov_ffts_st/tmp.info
      lcov -r cov_ffts_st/tmp.info '*/output/*' '*/build/opensrc/*' '*/build/proto/*' '*/third_party/*' '*/test/*' '/usr/local/*' '/usr/include/*'  -o cov_ffts_st/coverage.info
      cd "${BASEPATH}/cov_ffts_st/"
      genhtml coverage.info
      cd "${BASEPATH}"
    fi
  else
    echo "There is no ut or st need to be run..."
  fi
  cd "${BASEPATH}"
}

main() {
  # AIR build start
  echo "---------------- AIR build start ----------------"
  checkopts "$@"
  cd ./third_party
  git submodule update --init metadef
  cd ..
  chmod 755 third_party/metadef/graph/stub/gen_stubapi.py
  g++ -v

  mk_dir ${OUTPUT_PATH}
  build_ffts || { echo "AIR build failed."; exit 1; }
  echo "---------------- AIR build finished ----------------"

  run_llt || { echo "Run llt failed."; exit 1; }
  echo "---------------- LLT run and generate coverage report finished ----------------"

  chmod -R 750 ${OUTPUT_PATH}
  find ${OUTPUT_PATH} -name "*.so*" -print0 | xargs -0 chmod 500

  echo "---------------- AIR output generated ----------------"
}

main "$@"
