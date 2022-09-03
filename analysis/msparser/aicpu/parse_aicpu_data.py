#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

import logging
import os
import re

from common_func.ai_stack_data_check_manager import AiStackDataCheckManager
from common_func.common import get_data_dir_sorted_files
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.file_name_manager import get_data_preprocess_compiles
from common_func.file_name_manager import get_file_name_pattern_match
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from common_func.msvp_common import MsvpCommonConst
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from common_func.batch_counter import BatchCounter
from common_func.iter_recorder import IterRecorder
from analyzer.scene_base.profiling_scene import ProfilingScene


class ParseAiCpuData(MsMultiProcess):
    """
    parse ai cpu data by dp channel
    """
    TAG_AICPU = "AICPU"
    LIMIT_AI_CPU_LEN = 5000
    AICPU_MARKS = "Run start"
    DISPATCH_MARKS = "dispatchTime"
    CONVERSION_TIME = 1000
    TABLES_PATH = os.path.join(MsvpCommonConst.CONFIG_PATH, 'Tables.ini')
    AI_CPU_DATA_MAP = "AiCpuDataMap"
    LEN_TABLE = 10
    LEN_TIME_NAME = 5

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        MsMultiProcess.__init__(self, sample_config)
        self._file_list = file_list
        self.sample_config = sample_config
        self.project_path = self.sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self.ai_cpu_datas = []
        self._batch_counter = BatchCounter(self.project_path)
        self._batch_counter.init(Constant.TASK_TYPE_AI_CPU)
        self._iter_recorder = IterRecorder(self.project_path)

    @staticmethod
    def check_sign_exist(data: str, marks_sign: list) -> bool:
        """
        check for sign in data
        """
        for sign in marks_sign:
            if sign not in data:
                return False
        return True

    @staticmethod
    def __read_lines(file_path: str) -> list:
        """
        read lines from files
        """
        lines = []
        with open(file_path, "rb") as file_reader:
            # replace \n and \x00 in lines
            line = str(file_reader.read().replace(b'\n\x00', b' ___ ').replace(b'\x00', b' ___ '),
                       encoding='utf-8')
            lines += list(filter(None, line.split(" ___ ")))
        return lines

    def check_stage_time(self: any, timers: list, time_names: list) -> bool:
        """
        timers: Run start,compute start,memcpy start,memcpy end,Run end
        if time_names missing: Run start,Run end
        """
        if len(time_names) == self.LEN_TIME_NAME:
            return timers[4] >= timers[0] \
                   and timers[3] >= timers[2] >= timers[1]
        if len(time_names) < self.LEN_TIME_NAME and ' Run end' in time_names:
            return timers[-1] > timers[0]
        return False

    def parse_ai_cpu(self: any) -> None:
        """
        parse ai cpu
        """
        if not AiStackDataCheckManager.contain_dp_aicpu_data(self.project_path):
            return

        self.__create_db()
        data_dir = PathManager.get_data_dir(self.project_path)
        for _file in get_data_dir_sorted_files(data_dir):
            res = get_file_name_pattern_match(_file, *get_data_preprocess_compiles(self.TAG_AICPU))
            if res and is_valid_original_data(_file, self.project_path):
                self.__read_and_analysis_ai_cpu(os.path.join(data_dir, _file))
                FileManager.add_complete_file(self.project_path, _file)

        # flush all data to db
        self.__insert_data()

    def ms_run(self: any) -> None:
        """
        main entry for parsing ai cpu data
        :return: None
        """
        try:
            self.parse_ai_cpu()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ProfException) as task_rec_err:
            logging.error(str(task_rec_err), exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            pass

    def _calculate_ai_cpu_data(self: any, ai_cpu_datas: tuple, timers: list) -> list:
        """
        calculate ai cpu marks
        data format: timestamp, sys_start, sys_end, node_name, compute_time,
        memcpy_time, task_time, dispatch_time, total_time
        """
        dispatch_marks = ai_cpu_datas[1]
        info_split = ai_cpu_datas[0].split(",")
        marks_sign = [",", ":", " ", "dispatchTime=", "totalTime="]
        if not self.check_sign_exist(dispatch_marks, marks_sign):
            return []
        dispatch_time = dispatch_marks.split("dispatchTime=")[-1]
        total_time = dispatch_marks.split("totalTime=")[-1]

        info = InfoConfReader()
        ai_cpu_data = [0] * self.LEN_TABLE
        if re.search("streamId=", dispatch_marks):
            ai_cpu_data[0] = dispatch_marks.split("streamId=")[-1].split(", ")[0]
        else:
            ai_cpu_data[0] = NumberConstant.DEFAULT_STREAM_ID
        if re.search("taskId=", dispatch_marks):
            ai_cpu_data[1] = dispatch_marks.split("taskId=")[-1].split(".")[0]
        else:
            ai_cpu_data[1] = NumberConstant.DEFAULT_TASK_ID
        ai_cpu_data[2] = info.time_from_syscnt(timers[0], NumberConstant.MILLI_SECOND)
        ai_cpu_data[3] = info.time_from_syscnt(timers[-1], NumberConstant.MILLI_SECOND)
        ai_cpu_data[4] = info_split[0].split("Node:")[1].strip("")
        if re.search("compute", ai_cpu_datas[0]):
            ai_cpu_data[5] = (timers[2] - timers[1]) / NumberConstant.MS_TO_US
        else:
            ai_cpu_data[5] = Constant.NA
        if re.search("memcpy", ai_cpu_datas[0]):
            ai_cpu_data[6] = (timers[3] - timers[2]) / NumberConstant.MS_TO_US
        else:
            ai_cpu_data[6] = Constant.NA
        ai_cpu_data[7] = ai_cpu_data[3] - ai_cpu_data[2]
        ai_cpu_data[8] = float(dispatch_time.split(" ")[0]) / NumberConstant.MS_TO_US
        ai_cpu_data[9] = float(total_time.split(" ")[0]) / NumberConstant.MS_TO_US

        # timers -1 is end timestamp syscnt; ai cpu data 0 is stream; ai cpu data 1 is task
        ai_cpu_data.append(self.__calculate_batch_id(ai_cpu_data[0], ai_cpu_data[1], timers[-1]))

        return ai_cpu_data

    def __insert_data(self: any) -> None:
        if not self.ai_cpu_datas:
            return
        ai_cpu_conn, ai_cpu_curs = \
            DBManager.create_connect_db(PathManager.get_db_path(self.project_path, DBNameConstant.DB_AI_CPU))
        ai_cpu_inst_sql = "insert into {0} values ({1})".format(DBNameConstant.TABLE_AI_CPU,
                                                                ','.join(('?' for _ in self.ai_cpu_datas[0])))

        DBManager.executemany_sql(ai_cpu_conn, ai_cpu_inst_sql, self.ai_cpu_datas)
        DBManager.destroy_db_connect(ai_cpu_conn, ai_cpu_curs)
        self.ai_cpu_datas.clear()

    def __read_and_analysis_ai_cpu(self: any, file_path: str) -> None:
        infos = {self.AICPU_MARKS: [], self.DISPATCH_MARKS: []}
        # analysis run start/end time and dispatch time every loop.
        lines = self.__read_lines(file_path)
        while len(lines) >= 2:
            node_line = lines.pop(0)
            dispatch_line = lines.pop(0)
            if node_line.find(self.AICPU_MARKS) != -1:
                infos.get(self.AICPU_MARKS).append(node_line)
                infos.get(self.DISPATCH_MARKS).append(dispatch_line)

        for every_ai_cpu in zip(infos.get(self.AICPU_MARKS), infos.get(self.DISPATCH_MARKS)):
            # analysis compute, memory copy, task time dispatch time and total time for each aicpu.
            device_id = InfoConfReader().get_device_list()
            if not device_id:
                logging.error("No device list found in info.json when analysis ai cpu data.")
                return
            self.__analysis_ai_cpu(every_ai_cpu)

    def __create_db(self: any) -> bool:
        ai_cpu_conn, ai_cpu_curs = \
            DBManager.create_connect_db(PathManager.get_db_path(self.project_path, DBNameConstant.DB_AI_CPU))

        ai_cpu_create_sql = DBManager.sql_create_general_table(self.AI_CPU_DATA_MAP,
                                                               DBNameConstant.TABLE_AI_CPU,
                                                               self.TABLES_PATH)

        DBManager.execute_sql(ai_cpu_conn, ai_cpu_create_sql)
        DBManager.destroy_db_connect(ai_cpu_conn, ai_cpu_curs)
        return True

    def __analysis_ai_cpu(self: any, ai_cpu_datas: tuple) -> None:
        """
        analysis ai cpu marks
        data format: timestamp, sys_start, sys_end, node_name, compute_time,
        memcpy_time, task_time, dispatch_time, total_time
        """
        # info is in the following format:
        # [13136354159] Node:IteratorV2201910091232111, Run start:13136354161 1313635416102,
        # compute start:13136354167, memcpy start:13136354357, memcpy end:13136355132,
        #  Run end:13136355160 1313635516002
        #  streamId:int, taskId:int
        ai_cpu_marks = ai_cpu_datas[0]
        marks_sign = [",", ":", " "]
        if not self.check_sign_exist(ai_cpu_marks, marks_sign):
            logging.warning("Unable to get aicpu data, "
                            "maybe aicpu data is not collected .")
            return
        info_split = ai_cpu_marks.split(",")
        timers = []
        stage_time_names = []
        for iter_name in info_split[1:]:
            stage_time_names.append(iter_name.split(":")[0])
            node_time_value = iter_name.split(":")[-1].split(" ")[-1]
            if node_time_value and node_time_value.isdigit():
                timers.append(int(node_time_value))

        # time sequence should to be correct and ai cpu name needed.
        if not self.check_stage_time(timers, stage_time_names):
            logging.warning("Unable to get aicpu data, "
                            "maybe aicpu data is not collected .")
            return

        ai_cpu_data = self._calculate_ai_cpu_data(ai_cpu_datas, timers)
        self.ai_cpu_datas.append(ai_cpu_data)

    def __calculate_batch_id(self: any, stream_id: int, task_id: int, syscnt: int) -> int:
        if not ProfilingScene().is_operator():
            self._iter_recorder.set_current_iter_id(syscnt)
            batch_id = self._batch_counter.calculate_batch(stream_id, task_id, self._iter_recorder.current_iter_id)
        else:
            batch_id = self._batch_counter.calculate_batch(stream_id, task_id)
        return batch_id
