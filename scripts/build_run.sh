#readlink -f $0获取该文件的绝对路径；dirname获取父目录
CUR_DIR=$(dirname $(readlink -f $0))
#获取代码仓根目录
TOP_DIR=${CUR_DIR}/..
#存放编译出的libprofiler.so和stub
PROFILER_TEMP_DIR=${TOP_DIR}/profiler_temp
#存放编译出的msprof.bin和msprof.py
MSPROF_TEMP_DIR=${TOP_DIR}/msprof_temp

cp -r /home/mdf/decoupling_test/profiler_tmp ${PROFILER_TEMP_DIR}
cp -r /home/mdf/decoupling_test/msprof_temp ${MSPROF_TEMP_DIR}

#*********************************
# 凯达的编译代码
#*********************************


#makeself是一款编run包的工具
MAKESELF_DIR=${TOP_DIR}/opensource/makeself
#创建run包的脚本
CREATE_RUN_SCRIPT=${MAKESELF_DIR}/makeself.sh
#makeself的参数控制脚本，用于将--参数传给内置脚本
CONTROL_PARAM_SCRIPT=${MAKESELF_DIR}/makeself-header.sh
#输出run存放的位置
OUTPUT_DIR=${TOP_DIR}/output


#执行run包相关脚本存放的路径
RUN_SCRIPT_DIR=${TOP_DIR}/scripts/run_script
#入参白名单
FILTER_PARAM_SCRIPT=${RUN_SCRIPT_DIR}/help.sh
#run包运行时自动执行的脚本
MAIN_SCRIPT=/main.sh
#run包的名字
PROFILER_RUN_NAME="profiler"
MSPROF_RUN_NAME="msprof"
#版本还需要改动
SOFT_VERSION=1.2
ARCH="arch"

function create_temp_dir() {
	local temp_dir=${1}
	cp ${RUN_SCRIPT_DIR}${MAIN_SCRIPT} ${temp_dir}
}

function create_run_package(){
	local RUN_NAME=${1}
	local TEMP_DIR=${2}
	${CREATE_RUN_SCRIPT} \
	--header ${CONTROL_PARAM_SCRIPT} \
	--help-header ${FILTER_PARAM_SCRIPT} --pigz \
	--complevel 4 \
	--nomd5 \
	--sha256 \
	--chown \
	${TEMP_DIR} \
	${OUTPUT_DIR}/Ascend-${RUN_NAME}_${SOFT_VERSION}_linux-${ARCH}.run \
	${RUN_NAME} \
	.${MAIN_SCRIPT}
}

create_temp_dir ${PROFILER_TEMP_DIR}
create_temp_dir ${MSPROF_TEMP_DIR}
create_run_package ${PROFILER_RUN_NAME} ${PROFILER_TEMP_DIR}
create_run_package ${MSPROF_RUN_NAME} ${MSPROF_TEMP_DIR}

rm -rf ${PROFILER_TEMP_DIR}
rm -rf ${MSPROF_TEMP_DIR}
