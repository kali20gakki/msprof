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

# print usage message
usage()
{
  echo "Usage:"
  echo "sh build.sh [-h] [-n] [-a] [-f] [-c] [-p] [-v] [-g] [-j[n]]"
  echo ""
  echo "Options:"
  echo "    -h Print usage"
  echo "    -n Build fe ut/st"
  echo "    -f Build ffts ut/st"
  echo "    -a Build aicpu ut/st"
  echo "    -c Build ut/st with coverage tag"
  echo "    -u Build and run ut"
  echo "    -s Build and run st"
  echo "    -p Build inference or train"
  echo "    -v Display build command"
  echo "    -d Enable download cmake compile dependency from gitee , default off"
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
  ENABLE_FE_LLT="off"
  ENABLE_FFTS_LLT="off"
  ENABLE_AICPU_LLT="off"
  ENABLE_LLT_COV="off"
  PLATFORM="all"
  ENABLE_GITEE="on"
  PRODUCT="normal"
  ENABLE_UT="off"
  ENABLE_ST="off"
  # Process the options
  while getopts 'nfachj:vp:egus' opt
  do
    OPTARG=$(echo ${OPTARG} | tr '[A-Z]' '[a-z]')
    case "${opt}" in
      n)
        ENABLE_FE_LLT="on"
        ;;
      f)
        ENABLE_FFTS_LLT="on"
        ;;
      a)
        ENABLE_AICPU_LLT="on"
        ;;
      c)
        ENABLE_LLT_COV="on"
        ;;
      h)
        usage
        exit 0
        ;;
      j)
        THREAD_NUM=$OPTARG
        ;;
      v)
        VERBOSE="VERBOSE=1"
        ;;
      p)
        PLATFORM=$OPTARG
        ;;
      e)
        ENABLE_GITEE="on"
        ;;
      g)
        PRODUCT=$OPTARG
        ;;
      u)
        ENABLE_UT="on"
        ;;
      s)
        ENABLE_ST="on"
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

build_air()
{
  echo "create build directory and build AIR";

  mk_dir "${BUILD_PATH}"
  cd "${BUILD_PATH}"
  CMAKE_ARGS="-DBUILD_PATH=$BUILD_PATH"

  if [ "X$ENABLE_LLT_COV" = "Xon" ]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DENABLE_LLT_COV=ON"
  fi

  if [ "X$ENABLE_FE_LLT" = "Xon" ]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DENABLE_FE_LLT=ON"
    FE_LLT_TARGET="te_fusion_stub"
    if [ "X$ENABLE_UT" = "Xon" ]; then
      FE_LLT_TARGET="${FE_LLT_TARGET} fe_ut"
    fi
    if [ "X$ENABLE_ST" = "Xon" ]; then
      FE_LLT_TARGET="${FE_LLT_TARGET} fe_st"
    fi
  fi

  if [ "X$ENABLE_FFTS_LLT" = "Xon" ]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DENABLE_FFTS_LLT=ON"
    FFTS_LLT_TARGET=""
    if [ "X$ENABLE_UT" = "Xon" ]; then
      FFTS_LLT_TARGET="${FFTS_LLT_TARGET} ffts_ut"
    fi
    if [ "X$ENABLE_ST" = "Xon" ]; then
      FFTS_LLT_TARGET="${FFTS_LLT_TARGET} ffts_st"
    fi
  fi

  if [ "X$ENABLE_AICPU_LLT" = "Xon" ]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DENABLE_AICPU_LLT=ON"
    #CPU_LLT_TARGET="run_cpu_test"
    CPU_LLT_TARGET=""
    if [ "X$ENABLE_UT" = "Xon" ]; then
      CPU_LLT_TARGET="${CPU_LLT_TARGET} aicpu_ut"
      CMAKE_ARGS="${CMAKE_ARGS} -DENABLE_UT=ON"
    fi
    if [ "X$ENABLE_ST" = "Xon" ]; then
      CPU_LLT_TARGET="${CPU_LLT_TARGET} aicpu_st"
      CMAKE_ARGS="${CMAKE_ARGS} -DENABLE_ST=ON"
    fi
  fi

  if [ "X$ENABLE_GITEE" = "Xon" ]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DENABLE_GITEE=ON"
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

  TARGET="aicore_utils opskernel fe ffts fe.ini fusion_config.json \
          aicpu_engine_common aicpu_ascend_engine aicpu_tf_engine \
          host_cpu_engine host_cpu_opskernel_builder"

  if [ "X$ENABLE_AICPU_LLT" = "Xon" ] || [ "X$ENABLE_FE_LLT" = "Xon" ] || [ "X$ENABLE_FFTS_LLT" = "Xon" ];then
    TARGET=""
    if [ "X$ENABLE_AICPU_LLT" = "Xon" ]
    then
      TARGET="${TARGET} ${CPU_LLT_TARGET}"
    fi
    if [ "X$ENABLE_FE_LLT" = "Xon" ]
    then
      TARGET="${TARGET} ${FE_LLT_TARGET}"
    fi
    if [ "X$ENABLE_FFTS_LLT" = "Xon" ]
    then
      TARGET="${TARGET} ${FFTS_LLT_TARGET}"
    fi
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

run_llt_with_cov()
{
  export LD_LIBRARY_PATH=$ASCEND_CUSTOM_PATH/compiler/lib64:$ASCEND_CUSTOM_PATH/runtime/lib64:$LD_LIBRARY_PATH
  cd "${BASEPATH}"
  if [ "X$ENABLE_FE_LLT" = "Xon" ]
  then
    cd "${BASEPATH}/build/test/engines/nneng/depends/te_fusion/"
    ln -sf libte_fusion_stub.so libte_fusion.so
    cd "${BASEPATH}"
    if [ "X$ENABLE_UT" = "Xon" ]
    then
      echo "---------------- Begin to run fe ut ----------------"
      cd "${BASEPATH}/../"
      ./air/build/test/engines/nneng/ut/fe_ut
      echo "---------------- Finish the fe ut ----------------"
      if [ "X$ENABLE_LLT_COV" = "Xon" ]
      then
        echo "---------------- Begin to generate coverage of fe ut ----------------"
        cd "${BASEPATH}"
        mkdir "${BASEPATH}/cov_fe_ut/"
        lcov -c -d build/test/engines/nneng/ut -o cov_fe_ut/tmp.info
        lcov -r cov_fe_ut/tmp.info '*/output/*' '*/build/opensrc/*' '*/build/proto/*' '*/third_party/*' '*/test/*' '/usr/local/*' '/usr/include/*'  -o cov_fe_ut/coverage.info
        cd "${BASEPATH}/cov_fe_ut/"
        genhtml coverage.info
        echo "---------------- Finish the generating coverage of fe ut ----------------"
      fi
    fi
    if [ "X$ENABLE_ST" = "Xon" ]
    then
      echo "---------------- Begin to run fe st ----------------"
      cd "${BASEPATH}/../"
      ./air/build/test/engines/nneng/st/fe_st
      echo "---------------- Finish the fe st ----------------"
      if [ "X$ENABLE_LLT_COV" = "Xon" ]
      then
        cd "${BASEPATH}"
        mkdir "${BASEPATH}/cov_fe_st/"
        lcov -c -d build/test/engines/nneng/st -o cov_fe_st/tmp.info
        lcov -r cov_fe_st/tmp.info '*/output/*' '*/build/opensrc/*' '*/build/proto/*' '*/third_party/*' '*/test/*' '/usr/local/*' '/usr/include/*'  -o cov_fe_st/coverage.info
        cd "${BASEPATH}/cov_fe_st/"
        genhtml coverage.info
      fi
    fi
  fi
  if [ "X$ENABLE_FFTS_LLT" = "Xon" ]
  then
    if [ "X$ENABLE_UT" = "Xon" ]
    then
      echo "---------------- Begin to run ffts ut ----------------"
      cd "${BASEPATH}/../"
      ./air/build/test/engines/fftseng/ut/ffts_ut
      echo "---------------- Finish the ffts ut ----------------"
      if [ "X$ENABLE_LLT_COV" = "Xon" ]
      then
        echo "---------------- Begin to generate coverage of ffts ut ----------------"
        cd "${BASEPATH}"
        mkdir "${BASEPATH}/cov_ffts_ut/"
        lcov -c -d build/test/engines/fftseng/ut -o cov_ffts_ut/tmp.info
        lcov -r cov_ffts_ut/tmp.info '*/output/*' '*/build/opensrc/*' '*/build/proto/*' '*/third_party/*' '*/test/*' '/usr/local/*' '/usr/include/*'  -o cov_ffts_ut/coverage.info
        cd "${BASEPATH}/cov_ffts_ut/"
        genhtml coverage.info
        echo "---------------- Finish the generating coverage of ffts ut ----------------"
      fi
    fi
    if [ "X$ENABLE_ST" = "Xon" ]
    then
      echo "---------------- Begin to run ffts st ----------------"
      cd "${BASEPATH}/../"
      ./air/build/test/engines/fftseng/st/ffts_st
      echo "---------------- Finish the ffts st ----------------"
      if [ "X$ENABLE_LLT_COV" = "Xon" ]
      then
        cd "${BASEPATH}"
        mkdir "${BASEPATH}/cov_ffts_st/"
        lcov -c -d build/test/engines/fftseng/st -o cov_ffts_st/tmp.info
        lcov -r cov_ffts_st/tmp.info '*/output/*' '*/build/opensrc/*' '*/build/proto/*' '*/third_party/*' '*/test/*' '/usr/local/*' '/usr/include/*'  -o cov_ffts_st/coverage.info
        cd "${BASEPATH}/cov_ffts_st/"
        genhtml coverage.info
      fi
    fi
  fi
  if [ "X$ENABLE_AICPU_LLT" = "Xon" ]
  then
    if [ "X$ENABLE_UT" = "Xon" ]
    then
      if [ "X$ENABLE_LLT_COV" = "Xon" ]
      then
        echo "---------------- Begin to generate coverage of aicpu ut ----------------"
        cd "${BASEPATH}"
        if [ -d "${BASEPATH}/cov_aicpu_ut/" ];then
          rm -rf "${BASEPATH}/cov_aicpu_ut/"
        fi
        mkdir "${BASEPATH}/cov_aicpu_ut/"
        lcov -c -d build/test/engines/cpueng/ut -o cov_aicpu_ut/tmp.info
        lcov -r cov_aicpu_ut/tmp.info '*/output/*' '*/build/opensrc/*' '*/build/proto/*' '*/third_party/*' '*/test/*' '/usr/local/*' '/usr/include/*'  -o cov_aicpu_ut/coverage.info
        cd "${BASEPATH}/cov_aicpu_ut/"
        genhtml coverage.info
        echo "---------------- Finish the generating coverage of aicpu ut ----------------"
      fi
    fi
    if [ "X$ENABLE_ST" = "Xon" ]
    then
      if [ "X$ENABLE_LLT_COV" = "Xon" ]
      then
        cd "${BASEPATH}"
        if [ -d "${BASEPATH}/cov_aicpu_st/" ];then
          rm -rf "${BASEPATH}/cov_aicpu_st/"
        fi
        mkdir "${BASEPATH}/cov_aicpu_st/"
        lcov -c -d build/test/engines/cpueng/st -o cov_aicpu_st/tmp.info
        lcov -r cov_aicpu_st/tmp.info '*/output/*' '*/build/opensrc/*' '*/build/proto/*' '*/third_party/*' '*/test/*' '/usr/local/*' '/usr/include/*'  -o cov_aicpu_st/coverage.info
        cd "${BASEPATH}/cov_aicpu_st/"
        genhtml coverage.info
      fi
    fi
  fi

  cd "${BASEPATH}"
}

generate_package()
{
  cd "${BASEPATH}"

  AIR_LIB_PATH="lib"
  FWK_PATH="fwkacllib/lib64"
  ATC_PATH="atc/lib64"
  COMPILER_PATH="compiler/lib64"

  OPSKERNEL_PATH="plugin/opskernel"
  OPSKERNEL_FE_CONFIG_PATH="plugin/opskernel/fe_config"
  OPSKERNEL_CPU_CONFIG_PATH="plugin/opskernel/config"

  COMMON_LIB=("libaicore_utils.so" "libopskernel.so" "libaicpu_engine_common.so")
  OPSKERNEL_LIB=("libfe.so" "libffts.so" "libaicpu_ascend_engine.so"
                 "libaicpu_tf_engine.so" "libhost_cpu_opskernel_builder.so" "libhost_cpu_engine.so")
  PLUGIN_OPSKERNEL_FE=("fusion_config.json" "fe.ini")
  PLUGIN_OPSKERNEL_CPU=("init.conf" "ir2tf_op_mapping_lib.json" "aicpu_ops_parallel_rule.json")

  rm -rf ${OUTPUT_PATH:?}/${FWK_PATH}/
  rm -rf ${OUTPUT_PATH:?}/${ATC_PATH}/
  rm -rf ${OUTPUT_PATH:?}/${COMPILER_PATH}/

  mk_dir "${OUTPUT_PATH}/${FWK_PATH}/${OPSKERNEL_FE_CONFIG_PATH}"
  mk_dir "${OUTPUT_PATH}/${FWK_PATH}/${OPSKERNEL_CPU_CONFIG_PATH}"
  mk_dir "${OUTPUT_PATH}/${ATC_PATH}/${OPSKERNEL_FE_CONFIG_PATH}"
  mk_dir "${OUTPUT_PATH}/${ATC_PATH}/${OPSKERNEL_CPU_CONFIG_PATH}"
  mk_dir "${OUTPUT_PATH}/${COMPILER_PATH}/${OPSKERNEL_FE_CONFIG_PATH}"
  mk_dir "${OUTPUT_PATH}/${COMPILER_PATH}/${OPSKERNEL_CPU_CONFIG_PATH}"

  cd "${OUTPUT_PATH}"

  find ./ -name air.tar -exec rm {} \;

  MAX_DEPTH=1

  for lib in "${COMMON_LIB[@]}";
  do
    find ${OUTPUT_PATH}/${AIR_LIB_PATH} -maxdepth ${MAX_DEPTH} -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${ATC_PATH} \;
    find ${OUTPUT_PATH}/${AIR_LIB_PATH} -maxdepth ${MAX_DEPTH} -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${FWK_PATH} \;
    find ${OUTPUT_PATH}/${AIR_LIB_PATH} -maxdepth ${MAX_DEPTH} -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${COMPILER_PATH} \;
  done

  for lib in "${OPSKERNEL_LIB[@]}";
  do
    find ${OUTPUT_PATH}/${AIR_LIB_PATH} -maxdepth ${MAX_DEPTH} -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${ATC_PATH}/${OPSKERNEL_PATH} \;
    find ${OUTPUT_PATH}/${AIR_LIB_PATH} -maxdepth ${MAX_DEPTH} -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${FWK_PATH}/${OPSKERNEL_PATH} \;
    find ${OUTPUT_PATH}/${AIR_LIB_PATH} -maxdepth ${MAX_DEPTH} -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${COMPILER_PATH}/${OPSKERNEL_PATH} \;
  done

  for lib in "${PLUGIN_OPSKERNEL_CPU[@]}";
  do
    find ${OUTPUT_PATH}/${AIR_LIB_PATH} -maxdepth ${MAX_DEPTH} -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${FWK_PATH}/${OPSKERNEL_CPU_CONFIG_PATH} \;
    find ${OUTPUT_PATH}/${AIR_LIB_PATH} -maxdepth ${MAX_DEPTH} -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${ATC_PATH}/${OPSKERNEL_CPU_CONFIG_PATH} \;
    find ${OUTPUT_PATH}/${AIR_LIB_PATH} -maxdepth ${MAX_DEPTH} -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${COMPILER_PATH}/${OPSKERNEL_CPU_CONFIG_PATH} \;
  done

  for lib in "${PLUGIN_OPSKERNEL_FE[@]}";
  do
    find ${OUTPUT_PATH}/${AIR_LIB_PATH} -maxdepth ${MAX_DEPTH} -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${FWK_PATH}/${OPSKERNEL_FE_CONFIG_PATH} \;
    find ${OUTPUT_PATH}/${AIR_LIB_PATH} -maxdepth ${MAX_DEPTH} -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${ATC_PATH}/${OPSKERNEL_FE_CONFIG_PATH} \;
    find ${OUTPUT_PATH}/${AIR_LIB_PATH} -maxdepth ${MAX_DEPTH} -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${COMPILER_PATH}/${OPSKERNEL_FE_CONFIG_PATH} \;
  done

  if [ "x${ALL_IN_ONE_ENABLE}" = "x1" ]
  then
    tar -zcf air.tar compiler
  elif [ "x${PLATFORM}" = "xtrain" ]
  then
    tar -zcf air.tar fwkacllib
  elif [ "x${PLATFORM}" = "xinference" ]
  then
    tar -zcf air.tar atc
  elif [ "x${PLATFORM}" = "xall" ]
  then
    tar -zcf air.tar fwkacllib atc
  fi
}

main() {
  # AIR build start
  echo "---------------- AIR build start ----------------"
  checkopts "$@"
  chmod 755 third_party/metadef/graph/stub/gen_stubapi.py
  g++ -v

  mk_dir ${OUTPUT_PATH}
  build_air || { echo "AIR build failed."; exit 1; }
  echo "---------------- AIR build finished ----------------"

  chmod -R 750 ${OUTPUT_PATH}
  find ${OUTPUT_PATH} -name "*.so*" -print0 | xargs -0 chmod 500

  echo "---------------- AIR output generated ----------------"

  if [[ "X$ENABLE_FE_LLT" = "Xoff" ]] && [[ "X$ENABLE_AICPU_LLT" = "Xoff" ]] && [[ "X$ENABLE_FFTS_LLT" = "Xoff" ]]; then
    generate_package
  else
    run_llt_with_cov
  fi
  echo "---------------- AIR package archive generated ----------------"
}

main "$@"
