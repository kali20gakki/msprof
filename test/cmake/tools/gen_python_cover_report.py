#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
# ----------------------------------------------------------------------------
# Purpose:
# Copyright Huawei Technologies Co., Ltd. 2010-2020. All rights reserved.
# ----------------------------------------------------------------------------
"""
python_llt_run_and_check.sh脚本会在coverage_result目录下生成
各编译目标的覆盖率数据文件`.coverage.[local_module]`
与src_path文件`.src_path.[local_module]`（记录各编译目标对应的SRC_PATH）

本脚本基于python_llt_run_and_check.sh脚本的运行结果，生成覆盖率数据。
"""

from typing import Dict, List
import fnmatch
import logging
import os
import shlex
import subprocess
import sys
import shutil
import json
from lxml import etree
import codecs
THIS_FILE_NAME = __file__
THIS_FILE_PATH = os.path.dirname(os.path.realpath(THIS_FILE_NAME))
work_dir = os.path.abspath(os.path.join(THIS_FILE_PATH, "../../../"))
classify_rule_path = os.path.join(
    work_dir, 'vendor/hisi/classify_rule/classify_rule.yaml')
sys.path.append(os.path.join(work_dir, "vendor/hisi/llt/ci/script/"))
from utils import read_file_content_to_list,run_command
from diff_coverage import ProjectCoverage,DiffCoverage
logging.basicConfig(
    format='[%(asctime)s] [%(filename)s:%(lineno)d] [%(levelname)s] : %(message)s',
    level=logging.DEBUG)

logger = logging.getLogger()


class Path:
    def __init__(self, path, topdir):
        self.path = path
        self.topdir = topdir

    @property
    def relpath(self):
        return os.path.relpath(self.path, start=self.topdir)


class IncCoverageParam:
    def __init__(self, local_module, coverage_result_path):
        self.local_module = local_module
        self.coverage_result_path = coverage_result_path
        # src_path为,分隔的字符串，所以src_paths为一个列表
        self.src_paths = None

        self._topdir = coverage_result_path.topdir

    @property
    def coverage_filepath(self) -> Path:
        coverage_filepath = Path(
            os.path.join(self.coverage_result_path.path,
                         '.coverage.{0}'.format(self.local_module)),
            self._topdir)
        return coverage_filepath

    @property
    def src_path_filepath(self) -> Path:
        src_path_filepath = Path(
            os.path.join(self.coverage_result_path.path,
                         '.src_path.{0}'.format(self.local_module)),
            self._topdir)
        return src_path_filepath

    @property
    def cov_xml_filepath(self) -> Path:
        cov_xml_filepath = Path(
            os.path.join(self.coverage_result_path.path,
                         'coverage_{0}.xml'.format(self.local_module)),
            self._topdir)
        return cov_xml_filepath

    @property
    def cov_report_dirpath(self) -> Path:
        cov_report_dirpath = Path(
            os.path.join(self.coverage_result_path.path, self.local_module),
            self._topdir)
        return cov_report_dirpath

    def load_src_paths(self):
        with open(self.src_path_filepath.path) as file:
            src_paths = file.read().strip().split(',')
            self.src_paths = [
                Path(os.path.join(self._topdir, src_path), self._topdir)
                for src_path in src_paths
            ]


def quote_params(params) -> List:
    return [shlex.quote(param) for param in params]


def convert_to_envs(env: Dict) -> List:
    envs = []
    for key, value in env.items():
        envs.append('{0}={1}'.format(shlex.quote(key), shlex.quote(value)))

    return envs


def convert_to_command_line(params):
    command_line = ' '.join(quote_params(params))
    return command_line


def print_command_line(args):
    command_line = convert_to_command_line(args)
    logging.info('command line is %s', command_line)


def shell_exec(cmds, with_quote=True):
    if with_quote:
        command_line = ' '.join(quote_params(cmds))
    else:
        command_line = ' '.join(cmds)
    logging.info(command_line)
    return subprocess.run(command_line, shell=True, check=False)


def get_classify_rule(classifies_str: str) -> str:
    uniq_classifies = list(set(classifies_str.split(" ")))
    if len(uniq_classifies) <= 0:
        logging.error('Do not have classify_rule')
        sys.exit(1)
    return uniq_classifies


def get_averaged_threshold(thresholds):
    thresholds = thresholds.split()
    thresholds = [int(value) for value in thresholds]
    return round(sum(thresholds) / len(thresholds))


def get_local_modules(coverage_result_path: Path):
    files = os.listdir(coverage_result_path.path)
    src_path_files = fnmatch.filter(files, '.src_path.*')
    local_modules = [filename[10:] for filename in src_path_files]
    logging.info("find local_modules: %s", ",".join(local_modules))
    return sorted(local_modules)


def get_classify_rule_item_list(classify_rule_name):
    from yaml_config_parser import YamlConfig
    component_list = classify_rule_name
    yamlconfig = YamlConfig(classify_rule_path)
    yamlconfig.initial(component_list)
    item_list = yamlconfig.coveragefile
    logging.info('filterfile_name=%s', yamlconfig.filterfile)
    logging.info('classify_rule_name=%s', classify_rule_name)
    logging.info('classify_rule_item=%s', ",".join(item_list))
    if len(item_list) <= 0:
        return []
    return item_list


def generate_full_coverage_data(coverage_result_path: Path, python_version,
                                classify_rule: list) -> bool:
    """
    生成全量覆盖率
    1. coverage_result目录下执行coverage combile，合并覆盖率数据
    2. coverage_result目录下执行coverage html，生成html报告
    """
    try:
        os.chdir(coverage_result_path.path)

        result = shell_exec([python_version, '-m', 'coverage', 'combine'])
        if result.returncode != 0:
            logging.error('combine coverage failed!')
            return False

        # 如果在output/llt/coverage_result目录下执行python -m coverage html，生成的报告全是绝对路径，影响看板
        # 在根目录下执行python -m coverage html，生成的报告才是相对路径，coverage==4.5.4就会如此，如果升级coverage可修复此问题
        coverage_file = os.path.join(coverage_result_path.path, '.coverage')
        os.chdir(coverage_result_path.topdir)
        report_cmd = [
            'COVERAGE_FILE=' + coverage_file, python_version, '-m', 'coverage',
            'html', "-d", coverage_result_path.path
        ]
        classify_rule_item_list = get_classify_rule_item_list(
            classify_rule)
        if len(classify_rule_item_list) > 0:
            # 根据classify_rule中的条目，只统计跟组件相关的代码
            include_str = ",".join(classify_rule_item_list)
            report_cmd.append("--include=" + include_str)
        result = shell_exec(report_cmd)
        if result.returncode != 0:
            logging.error('generate coverage html failed!')
            return False
    finally:
        os.chdir(coverage_result_path.topdir)

    return True


def check_inc_coverage_prerequisite(
        inc_cov_params: List[IncCoverageParam]) -> bool:
    """
    检查增量覆盖率前置条件，.coverage文件与.src_path文件都需要存在
    """
    retcode = True
    for inc_cov_param in inc_cov_params:
        if not os.path.isfile(inc_cov_param.coverage_filepath.path):
            logging.warning('coverage file %s does not exist',
                            inc_cov_param.coverage_filepath.relpath)
            retcode = False

    for inc_cov_param in inc_cov_params:
        if not os.path.isfile(inc_cov_param.src_path_filepath.path):
            logging.warning('src_path file %s does not exist',
                            inc_cov_param.src_path_filepath.relpath)
            retcode = False
    return retcode


def generate_inc_coverage_data(python_version, classify_rule, top_dir,
                               threshold) -> bool:
    """
    生成增量覆盖率
    1. 对于每一个模块，执行coverage xml生成xml覆盖率数据文件
    2. 对于每一个模块，切换到对应的SRC_PATH目录，执行diff-cover，
       根据远端分支与xml文件，生成增量覆盖率报告
    """
    retcode = True
    nowdir = os.getcwd()
    if not os.path.exists(top_dir + '/output/llt/coverage_result/'):
        os.makedirs(top_dir + '/output/llt/coverage_result/')
    try:
        os.chdir(top_dir + '/output/llt/coverage_result/')
        result = shell_exec([python_version, '-m', 'coverage', 'combine'])
        if result.returncode != 0:
            logging.error('combine coverage failed!')
            return False
    finally:
        os.chdir(nowdir)
    coverage_path = top_dir + '/output/llt/coverage_result/'
    coveragefile_path = coverage_path + '.coverage'
    coveragexml_path = coverage_path + 'coverage.xml'
    envs = convert_to_envs(
        {'COVERAGE_FILE': coveragefile_path})
    cmds = quote_params([
        python_version, '-m', 'coverage', 'xml', '-o',
        coveragexml_path
    ])
    classify_rule_item_list = get_classify_rule_item_list(
        classify_rule)
    if len(classify_rule_item_list) > 0:
        # 根据classify_rule中的条目，只统计跟组件相关的代码
        include_str = ",".join(classify_rule_item_list)
        cmds.append("--include=" + include_str)
    result = shell_exec(envs + cmds, with_quote=False)
    if result.returncode != 0:
        logging.error('coverage xml %s to %s failed!',
                      coveragefile_path,
                      coveragexml_path)
        retcode = False

    if not retcode:
        return False
    # 增量覆盖率生成及校验
    invole_project_names = []
    coverage_xml_files = [coveragexml_path]
    language = 'python'
    FILE_COVERAGE_THRESHOLD = 100
    LINE_COVERAGE_THRESHOLD = threshold
    project_names, changed_files = read_file_content_to_list(
        os.path.join(top_dir, 'vendor/hisi/llt/ci/script/changed_files_list'))
    for project_name in project_names:
        path = os.path.join(top_dir, project_name)
        if not os.path.exists(path):
            logger.warning(" %s path is not exists", path)
        else:
            invole_project_names.append(project_name)

    diff_coverage = DiffCoverage(top_dir,
                                 classify_rule, invole_project_names, coverage_xml_files, changed_files, language, FILE_COVERAGE_THRESHOLD, LINE_COVERAGE_THRESHOLD)
    retcode = diff_coverage.process()
    return retcode


def generate_and_check_coverage(params: Dict):
    full_coverage_config = params['full_coverage_config']
    coverage_result_path = params['coverage_result_path']
    python_version = params['python_version']
    classify_rule = params['classify_rule']
    script_dir = params['script_dir']
    threshold = params['threshold']
    top_dir = script_dir.topdir
    classify_rule_str = " ".join(classify_rule)
    if full_coverage_config == 'true':
        retcode = generate_full_coverage_data(coverage_result_path,
                                              python_version, classify_rule)
        if not retcode:
            sys.exit(1)
    else:
        # ignore check result, go ahead
        # 增量检查
        retcode = generate_inc_coverage_data(python_version, classify_rule, top_dir,
                                             threshold)
        if not retcode:
            sys.exit(1)

    if full_coverage_config == 'true':

        result = shell_exec([
            python_version,
            os.path.join(script_dir.path, 'get_python_report.py'),
            classify_rule_str,
            os.path.join(coverage_result_path.path),
            str(threshold), full_coverage_config
        ])
        if result.returncode != 0:
            logging.error('run get_python_report.py failed')
            sys.exit(1)


def generate_coverage_json(coverage_result_path):
    report_file = os.path.join(coverage_result_path.path, 'index.html')
    if not os.path.exists(report_file):
        logging.warning('not found coverage file:%s', report_file)
        coverage = {
            'statement_hit': 0,
            'statement_total': 0,
            'function_hit': 0,
            'function_total': 0,
            "testcase_hit": 0,
            "testcase_total": 0,
            "branch_hit": 0,
            "branch_total": 0,
            "testing_tool": "pytest",
        }
    else:
        f = codecs.open(report_file, "r", "utf-8")
        content = f.read()
        f.close()
        tree = etree.HTML(content)

        coverage_total_element = tree.xpath(
            '//*[@id="index"]/table/tfoot/tr[1]/td[2]')
        coverage_miss_element = tree.xpath(
            '//*[@id="index"]/table/tfoot/tr[1]/td[3]')
        if len(coverage_total_element) == 0:
            coverage_total = 0
        else:
            coverage_total = int(coverage_total_element[0].text)
        if len(coverage_miss_element) == 0:
            coverage_miss = 0
        else:
            coverage_miss = int(coverage_miss_element[0].text)
        coverage = {
            'statement_hit': coverage_total - coverage_miss,
            'statement_total': coverage_total,
            'function_hit': 0,
            'function_total': 0,
            "testcase_hit": 0,
            "testcase_total": 0,
            "branch_hit": 0,
            "branch_total": 0,
            "testing_tool": "pytest",
        }
    result_json = json.dumps(coverage)
    json_file_name = "coverage.json"
    json_file_full_name = os.path.join(coverage_result_path.topdir,
                                       'output/llt', json_file_name)
    with open(json_file_full_name, 'w') as F:
        F.write(result_json)
    return json_file_full_name


def get_manifest_branch(topdir=None):
    if topdir:
        manifest_path = os.path.join(topdir, '.repo', 'manifests')
    else:
        manifest_path = os.path.join('.repo', 'manifests')

    cmd = 'echo $(cd "{0}" && git config branch.default.merge) | cut -d "/" -f 3'.format(
        manifest_path)
    result = subprocess.run(cmd,
                            stdout=subprocess.PIPE,
                            shell=True,
                            check=False)
    return result.stdout.decode().strip()


def main(argv):
    logging.basicConfig(
        format='%(asctime)s:%(levelname)s:%(filename)s:%(lineno)d:%(message)s',
        level=logging.INFO)

    stripped_argv = [arg.strip() for arg in argv]

    print_command_line(stripped_argv)

    outdir = stripped_argv[1]
    classifies = stripped_argv[2]
    thresholds = stripped_argv[3]
    full_coverage_config = stripped_argv[4]
    python_version = stripped_argv[5]

    classify_rule = get_classify_rule(classifies)
    threshold = get_averaged_threshold(thresholds)

    # 默认跑全量覆盖率
    if full_coverage_config != 'false':
        full_coverage_config = 'true'

    topdir = stripped_argv[6]

    coverage_result_path = Path(os.path.join(outdir, 'coverage_result'),
                                topdir)
    script_dir = Path(
        os.path.join(topdir, 'vendor', 'hisi', 'llt', 'ci', 'script',
                     'genaddcov'), topdir)

    params = {
        'full_coverage_config': full_coverage_config,
        'coverage_result_path': coverage_result_path,
        'python_version': python_version,
        'script_dir': script_dir,
        'classify_rule': classify_rule,
        'threshold': threshold,
    }
    generate_and_check_coverage(params)
    generate_coverage_json(coverage_result_path)


if __name__ == '__main__':
    main(sys.argv)
