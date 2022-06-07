#!usr/bin/env python
# coding:utf-8
"""
This script is used to parse ai core by hwts.
Copyright Huawei Technologies Co., Ltd. 2018-2020. All rights reserved.
"""

import logging
import os
import sqlite3
import struct
from collections import OrderedDict

from common_func.common import get_col_index
from common_func.constant import Constant
from common_func.data_manager import DataManager
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileOpen
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_exception import ProfException
from common_func.msvp_common import read_cpu_cfg
from common_func.utils import Utils
from mscalculate.calculate_ai_core_data import CalculateAiCoreData
from viewer.calculate_rts_data import insert_metric_value


class BaseParseDataByHwts:
    """
    base parse data by hwts
    """

    # Tags for differnt aicore log type.
    AICORE_LOG_SIZE = 128
    # Struct of aicore logs.
    RUNTIME_COMMON = "BBHHHIIqqqqqqqqqqIIIIIIII"
    # base number
    HEX = 16
    PMU_COUNT = 8

    def __init__(self: any, file_name: str, sample_config: dict, conn: any, freq: float) -> None:
        self.ai_core_file_name = file_name
        self.conn = conn
        self.aicore_data = {
            "ai_data": []
        }
        self.sample_config = sample_config
        self.events_name_list = []
        self.ai_core_profiling_events = OrderedDict()  # ai core pmu event
        self.freq = freq

    @classmethod
    def get_pmu_event_name(cls: any, pmu_event: str) -> str:
        """
        return pmu event name by pmu_event
        :param pmu_event: pmu event
        :return: pmu name
        """
        aicore_events_map = read_cpu_cfg("ai_core", "event2metric")
        pmu_name = aicore_events_map.get(int(pmu_event, cls.HEX), "")
        if not pmu_name:
            pmu_name = str(pmu_event)
        return pmu_name

    def read_binary_data(self: any) -> None:
        """
        Parsing ai core binary files.
        """
        logging.info("Start parsing aicore data ...")
        if not os.path.exists(self.ai_core_file_name):
            logging.error("ai core file is not found.")
            return
        sum_pmu = [0] * self.PMU_COUNT
        cycle = 0
        device_id = self.__get_device()
        if not device_id:
            logging.error("unable to get device_id from file name.")
            raise ProfException(ProfException.PROF_SYSTEM_EXIT)
        with FileOpen(self.ai_core_file_name, 'rb') as ai_core_log_file:
            self.__read_binary_data_helper(ai_core_log_file.file_reader, device_id, cycle, sum_pmu)
        self.aicore_data.get("ai_data").insert(0, ["", "", cycle, "", device_id, -1, ""])
        self.events_name_list, self.ai_core_profiling_events = \
            CalculateAiCoreData(self.sample_config.get("result_dir")).compute_ai_core_data(
                self.events_name_list, self.ai_core_profiling_events, cycle, sum_pmu)
        self.__move_total_forward()

    def remove_redundant(self: any) -> None:
        """
        remove redundant events
        :return: None
        """
        if self.ai_core_profiling_events.get("icache_req_ratio"):
            del self.ai_core_profiling_events["icache_req_ratio"]
        if self.ai_core_profiling_events.get("vec_fp16_128lane_ratio"):
            del self.ai_core_profiling_events["vec_fp16_128lane_ratio"]
        if self.ai_core_profiling_events.get("vec_fp16_64lane_ratio"):
            del self.ai_core_profiling_events["vec_fp16_64lane_ratio"]

    def save_db(self: any, table_name: str) -> None:
        """
        save ai core data to db
        """
        try:
            self.__save_db_helper(table_name)
        except sqlite3.Error as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def __move_total_forward(self: any) -> None:
        logging.info("move HWTS aicore total time data forward")
        for item in list(self.ai_core_profiling_events.keys()):
            self.ai_core_profiling_events[item].insert(0, self.ai_core_profiling_events[item][-1])
            self.ai_core_profiling_events[item].pop(-1)

    def __get_device(self: any) -> str:
        """
        get device_id from file name
        :return: device id
        """
        file_split = self.ai_core_file_name.split(".")
        # file name should be in the format aicore.data.43.0.0
        if file_split and len(file_split) > 2:
            return file_split[-2]
        return ""

    def __read_binary_data_helper(self: any, ai_core_log_file: any, *args: any) -> None:
        device_id, cycle, sum_pmu = args
        while True:
            log_struct = ai_core_log_file.read(self.AICORE_LOG_SIZE)
            if log_struct:
                if not log_struct.strip():
                    continue
            else:
                break
            _result = Utils.generator_to_list(hex(i) for i in struct.unpack(self.RUNTIME_COMMON, log_struct))
            _byte01 = bin(int(_result[0].replace('0x', ''), self.HEX))
            _byte01 = _byte01.replace('0b', '').zfill(8)
            total_cyc = int(_result[7].replace('0x', ''), self.HEX)
            task_id = int(_result[4].replace('0x', ''), self.HEX)
            stream_id = int(_result[17].replace('0x', ''), self.HEX)
            aicore_data = [_byte01[-4], _byte01[0:4], total_cyc,
                           int(_result[8].replace('0x', ''), self.HEX), device_id,
                           task_id,
                           stream_id]  # format: [overflow, cnt, total_cyc, ov_cyc, device_id, task_id, stream_id]
            self.aicore_data.get("ai_data").append(aicore_data)
            pmu_cnt = Utils.generator_to_list(int(i.replace('0x', ''), self.HEX) for i in _result[9:17])
            cycle += total_cyc
            for i in range(self.PMU_COUNT):
                sum_pmu[i] += pmu_cnt[i]

            self.events_name_list, self.ai_core_profiling_events = \
                CalculateAiCoreData(
                    self.sample_config.get("result_dir")).compute_ai_core_data(
                    self.events_name_list, self.ai_core_profiling_events,
                    total_cyc, pmu_cnt)

    def __get_current_iter_id(self: any, table_name: str) -> int:
        cur_iter_id = None
        get_iter_id_sql = "select max(iter_id) from {}".format(table_name)
        cur = self.conn.cursor()
        if cur:
            cur_iter_id = cur.execute(get_iter_id_sql).fetchone()
        return cur_iter_id[0] if cur_iter_id else NumberConstant.DEFAULT_ITER_ID

    def __before_save_process_ai_core_pmu(self: any, ai_core_lst: list, *args: any) -> tuple:
        device_id, cur_idx, table_name = args

        for ai_core, pmu in zip(self.aicore_data.get("ai_data"),
                                list(zip(*list(self.ai_core_profiling_events.values())))):
            ai_core_new = [ai_core[2]] + list(pmu) + [device_id] + [ai_core[5]] + [ai_core[6]]
            ai_core_lst[cur_idx] = tuple(ai_core_new)
            cur_idx += 1

        pmu_str = ", ?" * len(self.ai_core_profiling_events.keys())
        task_id_index = get_col_index(self.conn.cursor(), table_name, 'task_id')
        stream_id_index = get_col_index(self.conn.cursor(), table_name, 'stream_id')
        return pmu_str, task_id_index, stream_id_index

    def __save_db_helper(self: any, table_name: str) -> None:
        self.remove_redundant()
        current_iter_id = 1
        ai_core_metrics = ["total_cycles"] + list(self.ai_core_profiling_events.keys())
        if not DBManager.judge_table_exist(self.conn.cursor(), table_name):
            #  create metric summary table
            status = insert_metric_value(self.conn, ai_core_metrics, table_name)
            if not status:
                logging.error("create ai core %s table failed", table_name)
                return
        else:
            current_iter_id = self.__get_current_iter_id(table_name)

        cur_idx = 0
        ai_core_lst = [0] * len(self.aicore_data.get("ai_data"))
        device_id = self.__get_device()
        pmu_str, task_id_index, stream_id_index = \
            self.__before_save_process_ai_core_pmu(ai_core_lst, device_id, cur_idx, table_name)

        if not DataManager.add_iter_id(ai_core_lst, task_id_index, stream_id_index,
                                       iter_id=current_iter_id):
            logging.error("Failed to add iteration id for %s", table_name)
            return
        self.conn.executemany("insert into {0} values (?{1}, ?, ?, ?, ?)".format(
            table_name, pmu_str), ai_core_lst)  # insert ai core data to db
        self.conn.commit()


class ParseAiVectorCoreByHwts(BaseParseDataByHwts):
    """
    parse ai vector core data by hwts
    """

    def __init__(self: any, file_name: str, sample_config: dict, conn: any, freq: float) -> None:
        BaseParseDataByHwts.__init__(self, file_name, sample_config, conn, freq)

    def process(self: any) -> None:
        """
        process ai_core
        :return: None
        """

        self._parse_ai_vector_core_pmu_event()
        try:
            self.read_binary_data()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ProfException) as err:
            logging.error("%s: %s", os.path.basename(self.ai_core_file_name), err, exc_info=Constant.TRACE_BACK_SWITCH)
        self.save_db(DBNameConstant.TABLE_AIV_METRIC_SUMMARY)

    def _parse_ai_vector_core_pmu_event(self: any) -> None:
        """
        transformed ai_core_profiling_events
        :return: None
        """
        ai_core_profiling_events = \
            self.sample_config.get('aiv_profiling_events', '')
        if ai_core_profiling_events:
            self.events_name_list = Utils.generator_to_list(BaseParseDataByHwts.get_pmu_event_name(pmu_event)
                                                            for pmu_event in ai_core_profiling_events.split(","))
