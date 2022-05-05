#readlink -f $0获取该文件的绝对路径；dirname获取父目录
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..
#存放编译出的msprof.bin和msprof.py
TEMP_OUTPUT=${TOP_DIR}/build/output
MSPROF_TEMP_DIR=${TOP_DIR}/msprof_tmp
mkdir ${MSPROF_TEMP_DIR}

cp -r ${TEMP_OUTPUT}/lib/libmsprofiler.so ${MSPROF_TEMP_DIR}
cp -r ${TEMP_OUTPUT}/stub ${MSPROF_TEMP_DIR}
cp -r ${TEMP_OUTPUT}/bin/msprof ${MSPROF_TEMP_DIR}
cp -r ${TOP_DIR}/analysis ${MSPROF_TEMP_DIR}

#makeself是一款编run包的工具
MAKESELF_DIR=${TOP_DIR}/opensource/makeself
#创建run包的脚本
CREATE_RUN_SCRIPT=${MAKESELF_DIR}/makeself.sh
#makeself的参数控制脚本，用于将--参数传给内置脚本
CONTROL_PARAM_SCRIPT=${MAKESELF_DIR}/makeself-header.sh
#输出run存放的位置
OUTPUT_DIR=${TOP_DIR}/output
mkdir ${OUTPUT_DIR}


#执行run包相关脚本存放的路径
RUN_SCRIPT_DIR=${TOP_DIR}/scripts/run_script
#入参白名单
FILTER_PARAM_SCRIPT=${RUN_SCRIPT_DIR}/help.sh
#run包运行时自动执行的脚本
MAIN_SCRIPT=/main.sh
#run包的名字
MSPROF_RUN_NAME="msprof"
#获取版本
version="unknwon"


function parse_script_args() {
    while true; do
		echo "$1"
		if [ "$1" = "" ]; then
			break
		fi
        case "$1" in
        version_dir=*)
			version=${1#version_dir=}
			shift
			continue
            ;;
        *)
			echo "[ERROR]" "Input option is invalid"
			exit 2
            ;;
        esac
    done
}


#创建发布件的临时目录
function create_temp_dir() {
	local temp_dir=${1}
	cp ${RUN_SCRIPT_DIR}${MAIN_SCRIPT} ${temp_dir}
}

# 以下函数不需要
function version() {
    local _path="${TOP_DIR}/../../MindStudio_CI/Manifest/dependency/config.ini"
    local _version=$(grep "^version=" "${_path}" | cut -d"=" -f2)
    echo "${_version}"
}

function get_package_name() {
    local _product="Ascend"
    local _name="mindstudio-msprof"

    local _version=$(version)
    local _os_arch=$(arch)
    echo "${_product}-${_name}_${_version}_linux-${_os_arch}.run"
}


function create_run_package(){
	local _run_name=${1}
	local _temp_dir=${2}
	local _package_name=$(get_package_name)
	
	${CREATE_RUN_SCRIPT} \
	--header ${CONTROL_PARAM_SCRIPT} \
	--help-header ${FILTER_PARAM_SCRIPT} --pigz \
	--complevel 4 \
	--nomd5 \
	--sha256 \
	--chown \
	${_temp_dir} \
	${OUTPUT_DIR}/${_package_name} \
	${_run_name} \
	.${MAIN_SCRIPT}
}

parse_script_args $*
sed -i "1i VERSION=$version" ${RUN_SCRIPT_DIR}${MAIN_SCRIPT}
create_temp_dir ${MSPROF_TEMP_DIR}
create_run_package ${MSPROF_RUN_NAME} ${MSPROF_TEMP_DIR}

sed -i '1d' ${RUN_SCRIPT_DIR}${MAIN_SCRIPT}
