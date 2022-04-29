# 非法选项校验已由makeself的help.sh脚本完成
# 参数重复，参数非法组合的情况已由check_args完成
# 安装路径有效校验已在copy file中完成
function parse_script_args() {
    while true; do
		if [ "$3" = "" ]; then
			break
		fi
        case "$3" in
        --install-path=*)
			let "install_path_num+=1"
			
			#路径校验在copy file函数中完成
			install_path=${3#--install-path=}/${VERSION}
			shift
			continue
            ;;
        --uninstall)
            uninstall_flag=1
			let "install_args_num+=1"
            shift
			continue
            ;;
        --devel)
            let "install_args_num+=1"
            shift
			continue
            ;;
        --upgrade)
            let "install_args_num+=1"
            shift
			continue
            ;;
        --quiet)
            let "install_args_num+=1"
            shift
			continue
            ;;
        --run)
            let "install_args_num+=1"
            shift
			continue
            ;;
        --full)
            let "install_args_num+=1"
            shift
			continue
            ;;
        --install-for-all)
			#直接拷贝原有权限，不用特别设置
            shift
			continue
            ;;
        *)
			echo "[ERROR]" "Input option is invalid. Please try --help."
			exit 2
            ;;
        esac
    done
}


function check_args() {
	if [ ${install_args_num} -ne 0 ] && [ ${uninstall_flag} = 1 ]; then
		echo "[ERROR]" "Input option is invalid. Please try --help."
		echo uuuuuuuuuuu
		echo ${install_args_num}
		echo ${uninstall_flag}
		exit 2
	fi

	if [ ${install_path_num} -gt 1 ]; then
		echo "[ERROR]" "Do not input --install-path many times. Please try --help."
		echo yyyyyyyyyyyyyyyyyy
		echo ${install_path_num}
		exit 2
	fi
}

function execute_run() {
	if [ ${uninstall_flag} = 1 ]; then
		exit 0
	fi
	get_cann_package_name
	implement_install
}

function implement_install() {
	echo "$cann_package_name"
	if [ "$cann_package_name" = "ascend-toolkit" ]; then
		copy_file ${ANALYSIS} ${install_path}${ANALYSIS_PATH}${ANALYSIS}
		copy_file ${MSPROF} ${install_path}${MSPROF_PATH}${MSPROF}
	fi

	#if [ "$array"=="ascend-tookit" ] || [ "$array"=="nnae" ]; then
	#	copy_file ${GE_PROF} ${install_path}${GE_PROF_PATH}${GE_PROF}
	#fi

	#copy_file ${ACL_PROF} ${install_path}${ACL_PROF_PATH}${ACL_PROF}
	copy_file ${LIBMSPROFILER} ${install_path}${LIBMSPROFILER_PATH}${LIBMSPROFILER}
	copy_file ${LIBMSPROFILER_STUB} ${install_path}${LIBMSPROFILER_PATH}${LIBMSPROFILER_STUB}
}

function get_cann_package_name() {
	local name_list=(${install_path//// })
	cann_package_name=${name_list[-2]}
	echo "$cann_package_name"
	if [ "$cann_package_name" = "ascend-toolkit" ] || [ "$cann_package_name" = "nnrt" ] || [ "$cann_package_name" = "nnae" ]; then
		return
	fi
	echo "[ERROR]" "There is no ascend-toolkit, nnrt or nnae."
	exit 2
}

function copy_file() {
	local filename=${1}
	local target_file=$(readlink -f ${2})
	
	echo ${target_file}
	if [ -f "$target_file" ] || [ -d "$target_file" ]; then
		right=$(stat -c '%a' ${target_file})
		# target file的权限受父目录影响
		chmod u+w $(dirname ${target_file})
		travFolder ${target_file} u+w
		rm -r ${target_file}
		
		cp -r ${filename} ${target_file}
		travFolder ${target_file} $right
		chmod u-w $(dirname ${target_file})
		return
	fi
	echo "[WARNING]" "$2 is non-existent."
}

function travFolder(){
	local file_name=${1}
	local right=${2}
	chmod $right ${file_name}
	
	if [ -f "$file_name" ]; then
		return
	fi
	
	if [ -d "$file_name" ]; then
		file_list=`ls $file_name`
		
		for f in $file_list
		do
			travFolder ${file_name}/${f} ${right}
		done
	fi
}


if [ "$UID" = "0" ]; then
	# root用户安装的默认路径
	DEFAULT_INSTALL_PATH="/usr/local/Ascend/ascend-toolkit/latest"
else
	DEFAULT_INSTALL_PATH="${HOME}/Ascend/ascend-toolkit/latest"
fi
	
LIBMSPROFILER="libmsprofiler.so"
LIBMSPROFILER_STUB="stub/libmsprofiler.so"
ANALYSIS="analysis"
MSPROF="msprof"
#GE_PROF="ge_prof.h"
#ACL_PROF="acl_prof.h"

LIBMSPROFILER_PATH="/runtime/lib64/"
ANALYSIS_PATH="/tools/profiler/profiler_tool/"
MSPROF_PATH="/tools/profiler/bin/"
#GE_PROF_PATH="/compiler/include/ge/"
#ACL_PROF_PATH="/runtime/include/acl/"

install_path=${DEFAULT_INSTALL_PATH}

#以下参数用于校验
install_args_num=0
install_path_num=0

#以下参数决定是否卸载
uninstall_flag=0

#$*包括至少三个参数:0, 该脚本路径;1, 执行run包的路径;2, run包父目录;3, run包参数
parse_script_args $*
check_args
execute_run