#/bin/bash

# msprofbin libmsprofiler.so stub/libmsprofiler.so
set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..

rm -rf ${TOP_DIR}/msprof_bin_utest
rm -rf ${TOP_DIR}/opensource/protobuf
rm -rf ${TOP_DIR}/opensource/json
rm -rf ${TOP_DIR}/opensource/makeself
rm -rf ${TOP_DIR}/platform/securec
rm -rf ${TOP_DIR}/test/opensource
rm -rf ${TOP_DIR}/test/build_llt
rm -rf ${TOP_DIR}/test/output
rm -rf ${TOP_DIR}/hwts.*
rm -rf ${TOP_DIR}/llc.*
rm -rf ${TOP_DIR}/TRANSPORT_TRANSPORT_ITRANSPORT_TEST-SendFile-file_existing
rm -rf ${TOP_DIR}/test/msprof_python/ut/testcase/sqlite/ai_cpu.db