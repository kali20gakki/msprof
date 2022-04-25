#!/bin/bash

set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..
COV_DIR=${TOP_DIR}/llt/output
cd ${COV_DIR}
mkdir coverage

#
#
#
#

echo "***************Generate Coverage*****************"
lcov -c -d ./build/CMakeFiles/testcode.dir -o ./coverage/lcov_arm1.info -b ./coverage --rc lcov_branch_coverage=1
lcov -r ./coverage/lcov_arm1.info '*c++*' -o ./coverage/lcov_arm1.info --rc lcov_branch_coverage=1
lcov -r ./coverage/lcov_arm1.info '*opensource*' -o ./coverage/lcov_arm1.info --rc lcov_branch_coverage=1
lcov -r ./coverage/lcov_arm1.info '*pb_out*' -o ./coverage/lcov_arm1.info --rc lcov_branch_coverage=1
lcov -r ./coverage/lcov_arm1.info '*testcode*' -o ./coverage/lcov_arm1.info --rc lcov_branch_coverage=1
lcov -r ./coverage/lcov_arm1.info '*platform*' -o ./coverage/lcov_arm1.info --rc lcov_branch_coverage=1
genhtml ./coverage/lcov_arm1.info -o ./coverage/html1 --branch-coverage

tar -zcvf ./coverage/html1.tar.gz ./coverage/html1
cp ./build/test_detail.xml ./coverage/test_detail.xml