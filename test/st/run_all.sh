#!/bin/bash

# Define Color Codes
RED='\e[0;31m'
GREEN='\e[0;32m'
WHITE='\e[0;37m'
NC='\e[0m'

RUN_ALL_PATH="$(readlink -f "$0")"
PARENT_RUN_ALL_PATH="$(dirname "$RUN_ALL_PATH")"
export PYTHONPATH=$PYTHONPATH:${PARENT_RUN_ALL_PATH}
result_file=${PARENT_RUN_ALL_PATH}/run_all_result.txt
output_path=${PARENT_RUN_ALL_PATH}

rm -rf $output_path

test_list=`ls |grep ".sh" | grep -E "test_ascend_msprof"`
num_of_cases=$(echo "$test_list" | wc -l)

if [ ! -z "$test_list" ]; then
    echo -e "${WHITE}========================================${NC}"
    echo -e "${GREEN}[DEBUG] There are $num_of_cases test cases:${NC}"
    echo -e "${WHITE}----------------------------------------${NC}"
    echo -e "$test_list" | sed 's/^/    /'
    echo -e "${WHITE}========================================${NC}"

    for i in $test_list
    do
        echo -e "${WHITE}====================${i%.sh} Test Case ====================${NC}"
        bash $i $output_path
        if [ 0 -ne $? ]; then
            echo "$i fail" >> $result_file
            echo -e "--------------------------------------${WHITE}${i%.sh}${NC}------${RED}FAIL${NC}"
        else
            echo "$i pass" >> $result_file
            echo -e "--------------------------------------${WHITE}${i%.sh}${NC}------${GREEN}PASS${NC}"
        fi
    done
    echo -e "${GREEN}[DEBUG] End msprof_smoke_test${NC}"
else
    echo -e "${RED}[DEBUG] No test cases: $test_list${NC}"
fi
