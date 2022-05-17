import os
import sys
from argparse import ArgumentParser
import time
import logging

THIS_FILE_NAME = __file__
THIS_FILE_PATH = os.path.dirname(os.path.realpath(THIS_FILE_NAME))
work_dir = os.path.abspath(os.path.join(THIS_FILE_PATH, "../../../"))
sys.path.append(os.path.join(work_dir, "vendor/hisi/llt/ci/script/"))
from vcast.vcast_full_coverage_report import VcastFullCoverageReport
from utils import read_file_content_to_list
from diff_coverage import DiffCoverage

logging.basicConfig(
    format='[%(asctime)s] [%(filename)s:%(lineno)d] [%(levelname)s] : %(message)s',
    level=logging.DEBUG)

logger = logging.getLogger()


def _define_parser() -> ArgumentParser:
    parser = ArgumentParser()
    parser.description = main.__doc__
    parser.add_argument(
        '--code-root',
        required=True,
        help='code-root',
    )
    parser.add_argument(
        '--classify_rule_name',
        required=True,
        help='classify_rule_name',
    )
    parser.add_argument('--full_coverage', required=True, help='full coverage')
    return parser


def is_no_coverage_repo(name):
    """
    工程的脚本仓，不需要算增量覆盖率
    """
    white_list = [
        'cmake',
        'vendor/hisi/llt/ci',
        'vendor/hisi/farmland',
        'build/release/config',
        'build/product/cloud',
        'build/product/mini',
        'build/product/mdc',
    ]
    if name in white_list:
        return True
    return False


def inc_coverage(top_dir, classify_rule_list):
    # 增量覆盖率生成及校验

    invole_project_names = []
    coverage_xml_files = []
    coverage_xml_file_path = os.path.join(
        top_dir, 'output/llt/coverage_result/cobertura/')
    for f in os.listdir(coverage_xml_file_path):
        if f.endswith('.xml'):
            coverage_xml_files.append(os.path.join(coverage_xml_file_path, f))
    logger.info("xml files %s", ",".join(coverage_xml_files))
    language = 'c++'
    FILE_COVERAGE_THRESHOLD = 100
    LINE_COVERAGE_THRESHOLD = 80
    project_names, changed_files = read_file_content_to_list(
        os.path.join(top_dir, 'vendor/hisi/llt/ci/script/changed_files_list'))
    for project_name in project_names:
        path = os.path.join(top_dir, project_name)
        if not os.path.exists(path):
            logger.warning(" %s path is not exists", path)
            continue
        if is_no_coverage_repo(project_name):
            logger.info("%s path is no need coverage", path)
            continue
        invole_project_names.append(project_name)

    diff_coverage = DiffCoverage(top_dir,
                                 classify_rule_list, invole_project_names, coverage_xml_files, changed_files, language, FILE_COVERAGE_THRESHOLD, LINE_COVERAGE_THRESHOLD)
    retcode = diff_coverage.process()
    return retcode


def main(argv=None):
    parser = _define_parser()
    args = parser.parse_args(argv)
    code_root = args.code_root
    classify_rule_name = args.classify_rule_name
    classify_rule_list = list(set(classify_rule_name.split(' ')))
    full_coverage = args.full_coverage
    is_full_coverage = full_coverage != 'false'
    test_type = 'ut'
    template_file = os.path.join(code_root,
                                 "cmake/function/tools/template.html")

    if is_full_coverage:
        report = VcastFullCoverageReport(top_dir=code_root,
                                         template=template_file,
                                         test_type=test_type,
                                         classify_rule_name=classify_rule_name,
                                         is_full_coverage=is_full_coverage)
        ret = report.process()
        if not ret:
            sys.exit(1)
    else:
        ret = inc_coverage(top_dir=code_root,
                           classify_rule_list=classify_rule_list)
        if not ret:
            sys.exit(1)


if __name__ == '__main__':
    logger.info(str(sys.argv))
    main()
