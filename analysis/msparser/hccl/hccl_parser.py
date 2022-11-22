#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import json
import logging
import os
import shutil
import sqlite3

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileOpen
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.os_manager import check_dir_writable
from common_func.path_manager import PathManager
from msmodel.hccl.hccl_model import HCCLModel
from msparser.hccl.hccl_data import HCCLData
from profiling_bean.prof_enum.data_tag import DataTag


class HCCLParser(MsMultiProcess):
    """
    parsing HCCL data class
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.sample_config = sample_config
        self.device_id = self.sample_config.get("device_id", "0")
        self.project_path = self.sample_config.get("result_dir", "")
        self._file_list = file_list.get(DataTag.HCCL, [])
        self._hccl_dir = os.path.realpath(PathManager.get_hccl_path(self.project_path))
        self._model = HCCLModel(self.project_path, [DBNameConstant.TABLE_HCCL_ALL_REDUCE])
        self._hccl_data = []
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))

    def parse(self: any) -> None:
        """
        parse hccl data
        :return:
        """
        self._prepare_for_parse()
        self._parse_hccl_data()
        self._hccl_trace_data()

    def save(self: any) -> None:
        """
        save hccl data to db
        :return:
        """
        if self._hccl_data:
            self._model.init()
            self._model.flush(self._hccl_data)
            self._model.finalize()

    def ms_run(self: any) -> None:
        """
        Entry for hccl parse.
        """
        if not self._file_list:
            return
        try:
            self.parse()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        else:
            try:
                self.save()
            except sqlite3.Error as err:
                logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            # if os.path.exists(self._hccl_dir):
            #     shutil.rmtree(self._hccl_dir)
            pass

    def _prepare_for_parse(self: any) -> None:
        if os.path.exists(self._hccl_dir):
            check_dir_writable(self._hccl_dir)
            shutil.rmtree(self._hccl_dir)
        os.makedirs(self._hccl_dir, mode=NumberConstant.DIR_AUTHORITY)

    def _parse_hccl_data(self: any) -> None:
        try:
            from hccl_parser.entry import hccl_parse_op
        except (ImportError,) as import_error:
            logging.warning("Unable to import module: %s, please install.", str(import_error))
        else:
            hccl_parse_op(self.device_id, PathManager.get_data_dir(self.project_path), self._hccl_dir)

    def _hccl_trace_data(self: any) -> None:
        for _reduce_dir in os.listdir(self._hccl_dir):
            trace_path = os.path.realpath(os.path.join(self._hccl_dir, _reduce_dir))
            for _trace_file in os.listdir(trace_path):
                _trace_file = os.path.realpath(os.path.join(trace_path, _trace_file))
                self._append_hccl_data(_trace_file, _reduce_dir)

    def _append_hccl_data(self: any, trace_path: str,  op_name: str) -> None:
        with FileOpen(trace_path, "r") as hccl_reader:
            json_data = hccl_reader.file_reader.readline(Constant.MAX_READ_LINE_BYTES)
            json_data = json.loads(json_data)
        iteration = json_data.get("iteration", 0)
        first_timestamp = 0
        for hccl_data in json_data.get("traceEvents", []):
            hccl = HCCLData(hccl_data)
            if first_timestamp == 0:
                first_timestamp = hccl.timestamp
            self._hccl_data.append([op_name, iteration, hccl.hccl_name, first_timestamp, hccl.plane_id,
                                    hccl.timestamp, hccl.duration, hccl.notify_id, hccl.stage, hccl.step,
                                    hccl.bandwidth, hccl.stream_id, hccl.task_id, hccl.task_type,
                                    hccl.src_rank, hccl.dst_rank, hccl.transport_type, hccl.size])

