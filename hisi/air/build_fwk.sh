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
  echo "sh build.sh [-a] [-j[n]] [-h] [-v] [-s] [-t] [-u] [-c] [-S on|off] [-M]"
  echo ""
  echo "Options:"
  echo "    -a android compile" 
  echo "    -h Print usage"
  echo "    -u Only compile ut, not execute"
  echo "    -s Build and execute st"
  echo "    -j[n] Set the number of threads used for building GraphEngine, default is 8"
  echo "    -t Build and execute ut"
  echo "    -c Build ut with coverage tag"
  echo "    -p Build inference or train"
  echo "    -v Display build command"
  echo "    -S Enable enable download cmake compile dependency from gitee , default off"
  echo "    -M build MindSpore mode"
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
  # ENABLE_GE_UT_ONLY_COMPILE="off"
  ENABLE_GE_UT="off"
  ENABLE_GE_ST="off"
  ENABLE_GE_COV="off"
  PLATFORM=""
  PRODUCT="normal"
  ENABLE_GITEE="off"
  MINDSPORE_MODE="off"
  ENABLE_ANDROID_COMPILE="off"
  # Process the options
  while getopts 'austchj:p:g:vS:M' opt
  do
    OPTARG=$(echo ${OPTARG} | tr '[A-Z]' '[a-z]')
    case "${opt}" in
      a)
        ENABLE_ANDROID_COMPILE="on"
	;;
      u)
        # ENABLE_GE_UT_ONLY_COMPILE="on"
        ENABLE_GE_UT="on"
        ;;
      s)
        ENABLE_GE_ST="on"
        ;;
      t)
        ENABLE_GE_UT="on"
        ;;
      c)
        ENABLE_GE_COV="on"
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
      g)
        PRODUCT=$OPTARG
        ;;
      S)
        check_on_off $OPTARG S
        ENABLE_GITEE="$OPTARG"
        echo "enable download from gitee"
        ;;
      M)
        MINDSPORE_MODE="on"
        ;;
      *)
        echo "Undefined option: ${opt}"
        usage
        exit 1
    esac
  done
}
checkopts "$@"

#git submodule update --init metadef
#git submodule update --init parser

mk_dir() {
    local create_dir="$1"  # the target to make

    mkdir -pv "${create_dir}"
    echo "created ${create_dir}"
}

# GraphEngine build start
echo "---------------- GraphEngine build start ----------------"

# create build path
build_graphengine()
{
  echo "create build directory and build GraphEngine";
  mk_dir "${BUILD_PATH}"
  cd "${BUILD_PATH}"

  CMAKE_ARGS="-DBUILD_PATH=$BUILD_PATH"

  if [[ "X$ENABLE_GE_COV" = "Xon" ]]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DENABLE_GE_COV=ON"
  fi

  if [[ "X$ENABLE_GE_UT" = "Xon" ]]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DENABLE_GE_UT=ON"
  fi

  if [[ "X$ENABLE_GE_ST" = "Xon" ]]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DENABLE_GE_ST=ON"
  fi

  if [[ "X$ENABLE_GITEE" = "Xon" ]]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DENABLE_GITEE=ON"
  fi

  if [[ "X$MINDSPORE_MODE" = "Xoff" ]]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DENABLE_FWK_COMPILE=True -DENABLE_OPEN_SRC=True -DCMAKE_INSTALL_PREFIX=${OUTPUT_PATH} -DPLATFORM=${PLATFORM} -DPRODUCT=${PRODUCT}"
  else
    CMAKE_ARGS="${CMAKE_ARGS} -DENABLE_FWK_COMPILE=True -DENABLE_D=ON -DCMAKE_INSTALL_PREFIX=${OUTPUT_PATH}"
  fi

  if [[ "X$ENABLE_ANDROID_COMPILE" = "Xon" ]]; then
     CMAKE_ARGS="${CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${BASEPATH}/cmake/android_compile.cmake"
  fi

  echo "${CMAKE_ARGS}"
  cmake ${CMAKE_ARGS} ..
  if [ $? -ne 0 ]
  then
    echo "execute command: cmake ${CMAKE_ARGS} .. failed."
    return 1
  fi
  COMMON_TARGET="ge_local_engine ge_local_opskernel_builder ge_common slice engine fmk_parser parser_common _caffe_parser fmk_onnx_parser graph register engine_conf.json optimizer_priority.pbtxt"
  TARGET=${COMMON_TARGET}
  if [ "x${PLATFORM}" = "xtrain" ]
  then
    TARGET="ge_runner fwk_atc.bin ${TARGET}"
  elif [ "x${PLATFORM}" = "xinference" ]
  then
    TARGET="ge_compiler atc_atc.bin ge_executor_shared grpc_client grpc_server ${TARGET}"
  elif [ "X$ENABLE_GE_ST" = "Xon" ]
  then
    TARGET="ge_graph_dsl_test ge_running_env_test graph_engine_test hybrid_model_async_exec_test helper_runtime_test \
    ut_libge_kernel_utest ut_libge_common_utest"
  elif [ "X$ENABLE_GE_UT" = "Xon" ]
  then
    TARGET="ut_libgraph ut_libge_multiparts_utest ut_libge_others_utest \
    ut_libge_kernel_utest ut_libge_distinct_load_utest ut_libge_helper_utest ut_libge_label_maker_utest \
    ut_libge_common_utest"
  elif [ "X$MINDSPORE_MODE" = "Xon" ]
  then
    TARGET="ge_common graph"
  elif [ "x${PLATFORM}" = "xall" ]
  then
    # build all the target
    TARGET="ge_runner ge_compiler fwk_atc.bin atc_atc.bin ge_executor_shared ${TARGET}"
  fi

  make ${VERBOSE} ${TARGET} -j${THREAD_NUM} && make install
  if [ $? -ne 0 ]
  then
    echo "execute command: make ${VERBOSE} -j${THREAD_NUM} && make install failed."
    return 1
  fi
  echo "GraphEngine build success!"
}

generate_inc_coverage() {
  echo "Generating inc coverage, please wait..."
  rm -rf ${BASEPATH}/diff
  mkdir -p ${BASEPATH}/cov/diff

  git diff --src-prefix=${BASEPATH}/ --dst-prefix=${BASEPATH}/ HEAD^ > ${BASEPATH}/cov/diff/inc_change_diff.txt
  addlcov --diff ${BASEPATH}/cov/coverage.info ${BASEPATH}/cov/diff/inc_change_diff.txt -o ${BASEPATH}/cov/diff/inc_coverage.info
  genhtml --prefix ${BASEPATH} -o ${BASEPATH}/cov/diff/html ${BASEPATH}/cov/diff/inc_coverage.info --legend -t CHG --no-branch-coverage --no-function-coverage
}

g++ -v
mk_dir ${OUTPUT_PATH}
build_graphengine || { echo "GraphEngine build failed."; return; }
echo "---------------- GraphEngine build finished ----------------"
rm -f ${OUTPUT_PATH}/libgmock*.so
rm -f ${OUTPUT_PATH}/libgtest*.so
rm -f ${OUTPUT_PATH}/lib*_stub.so

chmod -R 750 ${OUTPUT_PATH}
find ${OUTPUT_PATH} -name "*.so*" -print0 | xargs -0 chmod 500

echo "---------------- GraphEngine output generated ----------------"

if [[ "X$ENABLE_GE_UT" = "Xon" ]]; then
    cp -rf ${BUILD_PATH}/tests/ut/common/graph/ut_libgraph ${OUTPUT_PATH}
    cp -rf ${BUILD_PATH}/tests/ut/ge/ut_libge_multiparts_utest ${OUTPUT_PATH}
    cp -rf ${BUILD_PATH}/tests/ut/ge/ut_libge_distinct_load_utest ${OUTPUT_PATH}
    cp -rf ${BUILD_PATH}/tests/ut/ge/ut_libge_others_utest ${OUTPUT_PATH}
    cp -rf ${BUILD_PATH}/tests/ut/ge/ut_libge_kernel_utest ${OUTPUT_PATH}
    cp -rf ${BUILD_PATH}/tests/ut/ge/ut_libge_helper_utest ${OUTPUT_PATH}
    cp -rf ${BUILD_PATH}/tests/ut/ge/ut_libge_label_maker_utest ${OUTPUT_PATH}
    cp -rf ${BUILD_PATH}/tests/ut/ge/ut_libge_common_utest ${OUTPUT_PATH}
    #execute ut testcase
    export LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libasan.so.4
    export ASAN_OPTIONS=detect_leaks=0
    RUN_TEST_CASE=${OUTPUT_PATH}/ut_libgraph && ${RUN_TEST_CASE} &&
    RUN_TEST_CASE=${OUTPUT_PATH}/ut_libge_multiparts_utest && ${RUN_TEST_CASE} &&
    RUN_TEST_CASE=${OUTPUT_PATH}/ut_libge_distinct_load_utest && ${RUN_TEST_CASE} &&
    RUN_TEST_CASE=${OUTPUT_PATH}/ut_libge_others_utest && ${RUN_TEST_CASE} &&
    RUN_TEST_CASE=${OUTPUT_PATH}/ut_libge_kernel_utest && ${RUN_TEST_CASE} &&
    RUN_TEST_CASE=${OUTPUT_PATH}/ut_libge_helper_utest && ${RUN_TEST_CASE} &&
    RUN_TEST_CASE=${OUTPUT_PATH}/ut_libge_common_utest && ${RUN_TEST_CASE} &&
    RUN_TEST_CASE=${OUTPUT_PATH}/ut_libge_label_maker_utest && ${RUN_TEST_CASE}
    if [[ "$?" -ne 0 ]]; then
        echo "!!! UT FAILED, PLEASE CHECK YOUR CHANGES !!!"
        echo -e "\033[31m${RUN_TEST_CASE}\033[0m"
        exit 1;
    fi
    unset LD_PRELOAD
    unset ASAN_OPTIONS
    echo "Generating coverage statistics, please wait..."
    cd ${BASEPATH}
    rm -rf ${BASEPATH}/cov
    mkdir ${BASEPATH}/cov
    lcov -c -d ${BUILD_PATH}/tests/ut/ge -d ${BUILD_PATH}/tests/ut/common/graph/ -o cov/tmp.info
    lcov -r cov/tmp.info '*/output/*' '*/build/opensrc/*' '*/build/proto/*' '*/build/grpc_*' '*/third_party/*' '*/tests/*' '/usr/local/*' '/usr/include/*' '*/metadef/*' '*/parser/*' -o cov/coverage.info
    cd ${BASEPATH}/cov
    genhtml coverage.info

    if [[ "X$ENABLE_GE_COV" = "Xon" ]]; then
      generate_inc_coverage
    fi
fi

if [[ "X$ENABLE_GE_ST" = "Xon" ]]; then
    cp -rf ${BUILD_PATH}/tests/st/testcase/st_run_data ${BUILD_PATH}/
    #execute st testcase
    export LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libasan.so.4
    export ASAN_OPTIONS=detect_leaks=0:new_delete_type_mismatch=0
    RUN_TEST_CASE=${BUILD_PATH}/tests/framework/ge_running_env/tests/ge_running_env_test && ${RUN_TEST_CASE} &&
    RUN_TEST_CASE=${BUILD_PATH}/tests/framework/ge_graph_dsl/tests/ge_graph_dsl_test && ${RUN_TEST_CASE} &&
    RUN_TEST_CASE=${BUILD_PATH}/tests/st/hybrid_model_exec/hybrid_model_async_exec_test && ${RUN_TEST_CASE} &&
    RUN_TEST_CASE=${BUILD_PATH}/tests/st/testcase/graph_engine_test && ${RUN_TEST_CASE} &&
    RUN_TEST_CASE=${BUILD_PATH}/tests/st/testcase/helper_runtime_test && ${RUN_TEST_CASE} &&
    RUN_TEST_CASE=${BUILD_PATH}/tests/ut/ge/ut_libge_kernel_utest && ${RUN_TEST_CASE} &&
    RUN_TEST_CASE=${BUILD_PATH}/tests/ut/ge/ut_libge_common_utest && ${RUN_TEST_CASE}
    if [[ "$?" -ne 0 ]]; then
        echo "!!! ST FAILED, PLEASE CHECK YOUR CHANGES !!!"
        echo -e "\033[31m${RUN_TEST_CASE}\033[0m"
        exit 1;
    fi
    # remove plugin
    rm -rf ${OUTPUT_PATH}/plugin
    unset LD_PRELOAD
    unset ASAN_OPTIONS
    echo "Generating coverage statistics, please wait..."
    cd ${BASEPATH}
    rm -rf ${BASEPATH}/cov
    mkdir ${BASEPATH}/cov
    lcov -c -d ${BUILD_PATH}/tests/framework/CMakeFiles/graphengine.dir \
            -d ${BUILD_PATH}/tests/framework/CMakeFiles/helper_runtime.dir \
            -d ${BUILD_PATH}/tests/ut/ge/CMakeFiles/ut_libge_kernel_utest.dir \
            -d ${BUILD_PATH}/tests/ut/ge/CMakeFiles/ge_ut_common_format.dir \
            -d ${BUILD_PATH}/tests/ut/ge/CMakeFiles/ut_libge_common_utest.dir \
            -o cov/tmp.info
    lcov -r cov/tmp.info '*/deployer/proto/*' '*/output/*' '*/build/opensrc/*' '*/build/proto/*' '*/build/grpc_*' '*/third_party/*' '*/tests/*' '/usr/local/*' '/usr/include/*' '*/metadef/*' '*/parser/*' -o cov/coverage.info
    cd ${BASEPATH}/cov
    genhtml coverage.info

    if [[ "X$ENABLE_GE_COV" = "Xon" ]]; then
      generate_inc_coverage
    fi
fi

# generate output package in tar form for runtime/compiler, including ut/st libraries/executables
generate_package()
{
  cd "${BASEPATH}"

  GRAPHENGINE_LIB_PATH="lib"
  RUNTIME_PATH="runtime/lib64"
  COMPILER_PATH="compiler/lib64"
  COMPILER_BIN_PATH="compiler/bin"
  COMPILER_INCLUDE_PATH="compiler/include"
  NNENGINE_PATH="plugin/nnengine/ge_config"
  OPSKERNEL_PATH="plugin/opskernel"

  RUNTIME_LIB=("libge_common.so" "libgraph.so" "libregister.so" "liberror_manager.so" "libge_executor.so" "libgrpc_client.so" "grpc_server")
  COMPILER_LIB=("libc_sec.so" "liberror_manager.so" "libge_common.so" "libslice.so" "libge_compiler.so" "libge_executor.so" "libge_runner.so" "libgraph.so" "libregister.so")
  PLUGIN_OPSKERNEL=("libge_local_engine.so" "libge_local_opskernel_builder.so" "optimizer_priority.pbtxt")
  PARSER_LIB=("lib_caffe_parser.so" "libfmk_onnx_parser.so" "libfmk_parser.so" "libparser_common.so")

  rm -rf ${OUTPUT_PATH:?}/${RUNTIME_PATH}/
  rm -rf ${OUTPUT_PATH:?}/${COMPILER_PATH}/
  rm -rf ${OUTPUT_PATH:?}/${COMPILER_BIN_PATH}/

  mk_dir "${OUTPUT_PATH}/${RUNTIME_PATH}"
  mk_dir "${OUTPUT_PATH}/${COMPILER_PATH}/${NNENGINE_PATH}"
  mk_dir "${OUTPUT_PATH}/${COMPILER_PATH}/${OPSKERNEL_PATH}"
  mk_dir "${OUTPUT_PATH}/${COMPILER_BIN_PATH}"
  mk_dir "${OUTPUT_PATH}/${COMPILER_INCLUDE_PATH}"

  cd "${OUTPUT_PATH}"

  find ./ -name graphengine_lib.tar -exec rm {} \;

  cp ${OUTPUT_PATH}/${GRAPHENGINE_LIB_PATH}/engine_conf.json ${OUTPUT_PATH}/${COMPILER_PATH}/${NNENGINE_PATH}

  find ${OUTPUT_PATH}/${GRAPHENGINE_LIB_PATH} -maxdepth 1 -name libengine.so -exec cp -f {} ${OUTPUT_PATH}/${COMPILER_PATH}/${NNENGINE_PATH}/../ \;

  MAX_DEPTH=1
  for lib in "${PLUGIN_OPSKERNEL[@]}";
  do
    find ${OUTPUT_PATH}/${GRAPHENGINE_LIB_PATH} -maxdepth ${MAX_DEPTH} -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${COMPILER_PATH}/${OPSKERNEL_PATH} \;
  done

  for lib in "${PARSER_LIB[@]}";
  do
    find ${OUTPUT_PATH}/${GRAPHENGINE_LIB_PATH} -maxdepth 1 -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${COMPILER_PATH} \;
  done

  for lib in "${RUNTIME_LIB[@]}";
  do
    find ${OUTPUT_PATH}/${GRAPHENGINE_LIB_PATH} -maxdepth 1 -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${RUNTIME_PATH} \;
  done

  for lib in "${COMPILER_LIB[@]}";
  do
    find ${OUTPUT_PATH}/${GRAPHENGINE_LIB_PATH} -maxdepth 1 -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${COMPILER_PATH} \;
  done

  find ./lib/fwkacl -name atc.bin -exec cp {} "${OUTPUT_PATH}/${COMPILER_BIN_PATH}" \;

  cp -rf ${BASEPATH}/base/metadef/inc/external/* ${COMPILER_INCLUDE_PATH}
  cp -rf ${BASEPATH}/base/parser/inc/external/* ${COMPILER_INCLUDE_PATH}
  cp -rf ${BASEPATH}/inc/external/* ${COMPILER_INCLUDE_PATH}

  tar -zcf graphengine_lib.tar runtime compiler
}

if [[ "X$ENABLE_GE_UT" = "Xoff" && "X$ENABLE_GE_ST" = "Xoff" && "X$MINDSPORE_MODE" = "Xoff" ]]; then
  generate_package
elif [ "X$MINDSPORE_MODE" = "Xon" ]
then
  cd "${OUTPUT_PATH}"
  find ./ -name graphengine_lib.tar -exec rm {} \;
  tar -zcf graphengine_lib.tar lib
fi
echo "---------------- GraphEngine package archive generated ----------------"
