#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
# ----------------------------------------------------------------------------
# Purpose:
# Copyright Huawei Technologies Co., Ltd. 2010-2020. All rights reserved.
# ----------------------------------------------------------------------------

import os
import queue
import shlex
import logging
import subprocess
import threading
import re
from argparse import ArgumentParser
import traceback
from multiprocessing import cpu_count
import xml.etree.ElementTree as ET

__version = "1.0"

logging.basicConfig(
    format=
    '[%(asctime)s] [%(filename)s:%(lineno)d] [%(levelname)s] : %(message)s',
    level=logging.DEBUG)
logger = logging.getLogger()

LLT_RUN_MOD_SINGLE = "single"
LLT_RUN_MOD_TESTSUITE = "testsuite"
LLT_RUN_MOD_TESTCASE = "testcase"
LLT_RUN_MOD_MIND = "mind"

LLT_RUN_MOD_CHOICES = (
    LLT_RUN_MOD_SINGLE,
    LLT_RUN_MOD_TESTSUITE,
    LLT_RUN_MOD_MIND
)
NUM_CPUS = cpu_count()


class LowLevelTestSplitByTestSuiteMixin(object):
    """
    llt用例执行按照测试套进行切分
    """

    def split(self):
        res = []
        for testsuite_name in self.testsuite_list:
            testsuite_fullname = testsuite_name + ".*"
            res.append(testsuite_fullname)
        return res


class LowLevelTestSplitByTestCaseMixin(object):
    """
    llt用例执行按照测试用例进行切分
    """

    def split(self):
        res = []
        for testcase_name in self.testcase_list:
            res.append(testcase_name)
        return res


class LowLevelTestSplitMindMixin(object):
    """
    llt用例执行按照非切分的执行结果，进行智能切分。
    """
    single_xml_file = None

    def _mind_split(self):
        with open(self.single_xml_file, 'r') as F:
            data = F.read()
        root = ET.fromstring(data)
        total_time = float(root.get("time"))
        print(total_time)
        testsuites = list(root.iterfind("testsuite"))
        testsuites_count = len(testsuites)
        print(testsuites_count)
        avg_time = total_time / self.thread
        print(avg_time)
        container = [[] for i in range(self.thread)]
        index = 0
        capacity = [0] * self.thread
        max_capacity = avg_time
        sort_testsuites = sorted(testsuites, key=lambda x: float(x.get("time")), reverse=True)
        for testsuite in sort_testsuites:
            testsuite_name = testsuite.get("name")
            testsuite_time = float(testsuite.get("time"))
            if testsuite_time > avg_time:
                for testcase in testsuite.iterfind("testcase"):
                    testcase_time = float(testcase.get('time'))
                    cur_container = container[index]
                    cur_capacity = capacity[index]
                    if cur_capacity + testcase_time < max_capacity:
                        cur_capacity += testcase_time
                        capacity[index] = cur_capacity
                        full_name = testcase.get("classname") + "." + testcase.get("name")
                        cur_container.append(full_name)
                    else:
                        index += 1
                        if index >= self.thread:
                            index = 0
                            max_capacity += max_capacity
            else:
                cur_container = container[index]
                cur_capacity = capacity[index]
                if cur_capacity + testsuite_time < max_capacity:
                    cur_capacity += testsuite_time
                    capacity[index] = cur_capacity
                    full_name = testsuite_name + ".*"
                    cur_container.append(full_name)
                else:
                    index += 1
                    if index >= 16:
                        index = 0
                        max_capacity += max_capacity

        return container

    def split(self):
        print("debug")
        res = []
        mind_split_res = self._mind_split()
        for item in mind_split_res:
            if len(item) > 0:
                res.append(":".join(item))

        return res


class LowLevelTestExecuteBase(object):
    def __init__(self, llt_bin, run_mod, tmp, output, clean, thread, print_llt_log):
        self.llt_bin = llt_bin
        self.llt_basename = os.path.basename(llt_bin)
        self.run_mod = run_mod
        self.tmp = tmp
        self.output = output
        self.clean = clean
        self.thread = thread
        self.print_llt_log = print_llt_log
        self._testsuites = None
        self._testsuite_list = None
        self._testcase_list = None

    @property
    def testsuites(self):
        """
        获取所有的测试套(含测试用例)
        :return:
        """
        if self._testsuites is not None:
            return self._testsuites
        cmd = self.llt_bin + " --gtest_list_tests"
        p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, )
        stdout, stderr = p.communicate()
        logger.info(cmd)
        logger.info(stdout)
        if p.returncode != 0:
            logger.error(cmd + " may failed.")
            logger.error(stdout, stderr)
            raise Exception(cmd + " may failed.")
        testsuites = {}
        cur_testsuite_name = None
        for line in stdout.decode().splitlines():
            re_testsuites_str = r"^[\w/]+\.$"
            re_testcase_str = r"^  \w+$"
            if re.match(re_testsuites_str, line):
                cur_testsuite_name = line.strip('.')
                testsuites[cur_testsuite_name] = []
            elif re.match(re_testcase_str, line):
                testcase_name = line.strip(' ')
                testsuites[cur_testsuite_name].append(testcase_name)
            else:
                pass  # 没有匹配到测试用例相关行，一般是执行日志打印
        self._testsuites = testsuites
        return self._testsuites

    @property
    def testsuite_list(self):
        """
        测试套列表
        :return:
        """
        if self._testsuite_list is not None:
            return self._testsuite_list
        testsuite_list = []
        for testsuite_name in self.testsuites.keys():
            testsuite_list.append(testsuite_name)
        self._testsuite_list = testsuite_list
        return self._testsuite_list

    @property
    def testcase_list(self):
        """
        测试用例列表
        :return:
        """
        if self._testcase_list is not None:
            return self._testcase_list
        testcase_list = []
        for testsuite_name, testcases in self.testsuites.items():
            for testcase in testcases:
                testcase_fullname = testsuite_name + "." + testcase
                testcase_list.append(testcase_fullname)
        self._testcase_list = testcase_list
        return self._testcase_list

    def split(self):
        """
        用例切分方法
        :return:
        """
        pass

    def run(self):
        """
        执行llt
        :return: Bool
        """

    def merge(self):
        """
        合并执行结果
        :return: xml file
        """

    def process(self):
        """

        :return:
        """
        pass


class LowLevelTestExecuteMultiBase(LowLevelTestExecuteBase):
    process_index = 0
    output_files = []

    def _run(self, q_tests, q_output, lock):
        while not q_tests.empty():
            item = q_tests.get()
            item_show_name = item if len(item) < 200 else item[0:200] + "..."
            print("progress:", item_show_name)
            lock.acquire()
            self.process_index += 1
            lock.release()
            output_name = "{basename}_{index}.xml".format(basename=self.llt_basename, index=self.process_index)
            output = os.path.join(self.tmp, output_name)
            log_file_name = "{basename}_{index}.log".format(basename=self.llt_basename, index=self.process_index)
            log_file = os.path.join(self.tmp, log_file_name)

            shell_cmd = "{llt_bin} --gtest_filter={item} --gtest_output=xml:{output}".format(llt_bin=self.llt_bin,
                                                                                             item=item,
                                                                                             output=output)
            cmd = shlex.split(shell_cmd)
            p = subprocess.run(cmd, shell=False, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            if self.print_llt_log:
                print(p.stdout.decode())
            with open(log_file, 'w') as F:
                F.write(shell_cmd)
                F.write("\n")
                F.write(p.stdout.decode())
            q_output.put(output)
            if p.returncode == 0:
                print("progress:", item_show_name, "ok")
            else:
                print("progress:", item_show_name, "failed")

    def _indent(self, elem, level=0):
        """格式化xml输出"""
        i = "\n" + level * "\t"
        if len(elem):
            if not elem.text or not elem.text.strip():
                elem.text = i + "\t"
            if not elem.tail or not elem.tail.strip():
                elem.tail = i
            for elem in elem:
                self._indent(elem, level + 1)
            if not elem.tail or not elem.tail.strip():
                elem.tail = i
        else:
            if level and (not elem.tail or not elem.tail.strip()):
                elem.tail = i

    def _save_split_result(self, split_res, output_file=None):
        if not output_file:
            output_file = os.path.join(self.tmp, self.llt_basename + "_split.txt.log")
        with open(output_file, 'w') as F:
            for item in split_res:
                F.write(item)
                F.write("\n")

    def merge(self):
        files = self.output_files
        root = ET.Element("testsuites")
        tests = 0
        failures = 0
        disabled = 0
        errors = 0
        times = 0.0
        for f in files:
            if not f:
                continue
            data = open(f, 'r').read()
            try:
                data = data.encode("GB2312").decode("UTF-8")
            except:
                data = re.sub(r'[^\x00-\x7f]', r'', data).encode("G2312").decode("UTF-8")
            data = data.replace("GB2312", "utf-8")
            et = ET.fromstring(data)
            tests += int(et.get("tests"))
            failures += int(et.get("failures"))
            disabled += int(et.get("disabled"))
            errors += int(et.get("errors"))
            times += float(et.get("time"))
            for testsuite in et.iterfind("testsuite"):
                if len(testsuite.findall('testcase[@status="run"]')) > 0:
                    root.append(testsuite)
        root.set("tests", str(tests))
        root.set("failures", str(failures))
        root.set("disabled", str(disabled))
        root.set("errors", str(errors))
        root.set("time", str(round(times, 3)))
        tree = ET.ElementTree(root)
        root.set("name", "AllTests")
        self._indent(root)
        tree.write(self.output, encoding="utf-8", xml_declaration=True)

    def process(self):
        max_thread = self.thread
        q_tests = queue.Queue()
        q_output = queue.Queue()
        lock = threading.Lock()
        split_items = self.split()
        ts = []
        for item in split_items:
            q_tests.put(item)
        for i in range(max_thread):
            t = threading.Thread(target=self._run, args=(q_tests, q_output, lock))
            t.start()
            ts.append(t)
        for t in ts:
            t.join()
        while not q_output.empty():
            self.output_files.append(q_output.get())
        self._save_split_result(split_res=split_items)  # 把切分结果保存到日志目录中
        self.merge()


class LowLevelTestExecuteSingle(LowLevelTestExecuteBase):

    def process(self):
        shell_cmd = "{llt_bin} --gtest_output=xml:{output}".format(llt_bin=self.llt_bin, output=self.output)
        cmd = shlex.split(shell_cmd)
        p = subprocess.Popen(cmd, shell=False, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        while p.poll() is None:
            try:
                line = p.stdout.readline().decode()
                line = line.strip()
            except:
                traceback.format_exc()
                print(p.stdout.readline())
            if line:
                print(line)
        if p.returncode == 0:
            return True
        else:
            return False


class LowLevelTestExecuteTestSuite(LowLevelTestSplitByTestSuiteMixin, LowLevelTestExecuteMultiBase):
    pass


class LowLevelTestExecuteTestCase(LowLevelTestSplitByTestCaseMixin, LowLevelTestExecuteMultiBase):
    pass


class LowLevelTestExecuteMind(LowLevelTestSplitMindMixin, LowLevelTestExecuteMultiBase):
    pass


def main(argv=None):
    """Analyze build event to a graph, then trigger jobs."""
    parser = _define_parser()
    args = parser.parse_args(argv)
    if not os.path.exists(args.llt_bin):
        logger.error("%s not exist!", args.llt_bin)
    llt_class = None
    if args.run_mod == LLT_RUN_MOD_SINGLE:
        llt_class = LowLevelTestExecuteSingle
    elif args.run_mod == LLT_RUN_MOD_TESTSUITE:
        llt_class = LowLevelTestExecuteTestSuite
    elif args.run_mod == LLT_RUN_MOD_TESTCASE:
        llt_class = LowLevelTestExecuteTestCase
    elif args.run_mod == LLT_RUN_MOD_MIND:
        llt_class = LowLevelTestExecuteMind
    if not llt_class:
        logger.error("no executor init")
    executor = llt_class(llt_bin=args.llt_bin, run_mod=args.run_mod, tmp=args.tmp,
                         output=args.output, clean=args.clean, thread=args.thread,
                         print_llt_log=args.print_llt_log)
    if args.run_mod == LLT_RUN_MOD_MIND:
        if not args.train_xml:
            logger.error("must set train-xml")
            exit(1)
        if not os.path.exists(args.train_xml):
            logger.error("%s not exist!", args.train_xml)
            exit(1)
        executor.single_xml_file = args.train_xml
    executor.process()


def _define_parser() -> ArgumentParser:
    parser = ArgumentParser()
    parser.description = main.__doc__
    parser.add_argument(
        'llt_bin',
        help='llt binary',
    )
    parser.add_argument(
        '--run-mod',
        required=True,
        type=str.lower,
        choices=LLT_RUN_MOD_CHOICES,
        help='llt will be execute by the mode',
    )
    parser.add_argument(
        '--train-xml',
        required=False,
        help='train xml file',
    )
    parser.add_argument(
        '--thread',
        required=False,
        type=int,
        default=NUM_CPUS,
        help='the tmp dir used to store temp file',
    )
    parser.add_argument(
        '--tmp',
        required=False,
        default="./tmp",
        help='the tmp dir used to store temp file',
    )
    parser.add_argument(
        '--output',
        required=False,
        default="output.xml",
        help='train xml file',
    )
    parser.add_argument(
        '--clean',
        action='store_true',
        default=False,
        help='is clean tmp dir after run finish',
    )
    parser.add_argument(
        '--print-llt-log',
        action='store_true',
        default=False,
        help='is print llt run log'
    )
    parser.add_argument('-V', '--version', action='version', version=__version)
    return parser


if __name__ == "__main__":
    main()

