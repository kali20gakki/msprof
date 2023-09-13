#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import logging
import os
from enum import Enum

from common_func.common import error
from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileOpen
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from common_func.msvp_common import is_valid_original_data
from msmodel.nano.nano_exeom_model import NanoExeomModel
from msmodel.nano.nano_exeom_model import NanoGraphAddInfoViewModel
from msmodel.runtime.runtime_host_task_model import RuntimeHostTaskModel
from msparser.interface.data_parser import DataParser
from msparser.nano.nano_dbg_bean import L1OpDescBean
from msparser.nano.nano_dbg_bean import L2InputDescBean
from msparser.nano.nano_dbg_bean import L2OutputDescBean
from msparser.nano.nano_dbg_bean import NameBean
from msparser.nano.nano_dbg_bean import NumBean
from msparser.nano.nano_dbg_bean import ShapeBean
from msparser.nano.nano_dbg_bean import TypeHeadBean, MagicHeadBean
from profiling_bean.prof_enum.data_tag import DataTag


class L1NameType(Enum):
    """
    leve1:name type enum
    """
    MODEL_NAME = 0
    TASK_INFO = 1


class L2NameType(Enum):
    """
    leve2:name type enum
    """
    OP_NAME = 0
    OP_TYPE = 1
    ORI_OP_NAME = 2
    L1_SUB_GRAPH_NO = 3
    INPUT = 4
    OUTPUT = 5


class L3NameType(Enum):
    """
    leve3:name type enum
    """
    SHAPE = 0


class NanoDbgParser(DataParser, MsMultiProcess):
    """
    parse nano_model_exeom, to get dbg name
    """

    INVALID_ID = 4294967295
    INVALID_CONTEXT_ID = 4294967295
    INVALID_VALUE = -1
    INVALID_TENSOR_NUM = -1
    INVALID_INDEX_ID = -1
    INVALID_TIMESTAMP = -1
    INVALID_BLOCK_DIM = 0
    INVALID_DEVICE_ID = 0
    INVALID_REQUEST_ID = 0
    INVALID_BATCH_ID = 0
    INVALID_OP_STATE = "0"
    DBG_NAME_LEN = 4

    # 0x5a5a5a5a is 1515870810
    MAGIC_NUM = 1515870810

    # 1G
    DBG_MAX_SIZE = 1024 * 1024 * 1024

    SEMICOLON_CHAR = ";"

    L2_NAME_DICT = {
        L2NameType.OP_NAME.value: "op_name",
        L2NameType.OP_TYPE.value: "op_type",
        L2NameType.ORI_OP_NAME.value: "original_name",
        L2NameType.L1_SUB_GRAPH_NO.value: "l1_sub_graph_no",
        L2NameType.INPUT.value: "input",
        L2NameType.OUTPUT.value: "output",
    }

    BEAN_DICT = {
        L2NameType.INPUT.value: L2InputDescBean,
        L2NameType.OUTPUT.value: L2OutputDescBean,
    }

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._model_info = []

    def parse(self: any) -> None:
        """
        parse function
        """
        dbg_tag_files = self._file_list.get(DataTag.DBG_FILE, [])
        model_filename_dict = self._get_nano_model_filename_with_id_data()
        for _file in dbg_tag_files:
            if not is_valid_original_data(_file, self._project_path) \
                    or _file[:-self.DBG_NAME_LEN] not in model_filename_dict:
                continue
            _file_path = self.get_file_path_and_check(_file)
            logging.info(
                "start parsing nano_dbg_file data file: %s", _file)
            self._read_data(_file_path, model_filename_dict.get(_file[:-self.DBG_NAME_LEN]))

    def save(self: any) -> None:
        """
        save data to db
        :return:
        """
        if not self._model_info:
            logging.error(
                "Please confirm that the PROF data matches the dbg file!")
            return
        with NanoExeomModel(self._project_path) as exeom_model:
            exeom_model.flush(self._reformat_ge_task_data(self._model_info), DBNameConstant.TABLE_GE_TASK)

        with RuntimeHostTaskModel(self._project_path) as host_task_model:
            host_task_model.flush(self._reformat_host_task_data(self._model_info), DBNameConstant.TABLE_HOST_TASK)

    def ms_run(self: any) -> None:
        """
        entrance for dbg parser
        :return:
        """
        if not (self._file_list.get(DataTag.DBG_FILE, [])):
            logging.error(
                "Please confirm if there are dbg files in the data folder!")
            return
        try:
            self.parse()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        self.save()

    def _check_magic_num(self, data: bytes, data_offset: int) -> int:
        head = MagicHeadBean().decode(data, data_offset)
        data_offset += head.fmt_size
        if head.magic_num != self.MAGIC_NUM:
            logging.error("DBG file verification failed!")
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)
        return data_offset

    def _get_task_info(self, data: bytes, data_offset: int, file_size: int, model_id: int):
        while data_offset < file_size:
            head_data = TypeHeadBean().decode(data, data_offset)
            data_offset += head_data.fmt_size
            if head_data.datatype == L1NameType.TASK_INFO.value:
                num = NumBean().decode(data, data_offset)
                data_offset += num.fmt_size
                for _ in range(num.num):
                    dbg_dict = {}
                    bean_data = L1OpDescBean().decode(data, data_offset)
                    dbg_dict.update({"task_id": bean_data.task_id, "stream_id": bean_data.stream_id,
                                     "task_type": bean_data.task_type, "block_dim": bean_data.block_dim})
                    data_offset += bean_data.fmt_size
                    data_offset = self._get_op_info(data, dbg_dict, data_offset, data_offset + bean_data.length)
                    dbg_dict.setdefault("model_id", model_id)
                    self._model_info.append(dbg_dict)
            else:
                data_offset += head_data.length

    def _get_op_info(self, data: bytes, res: dict, data_offset: int, length: int) -> int:
        while data_offset < length:
            head_data = TypeHeadBean().decode(data, data_offset)
            data_offset += head_data.fmt_size
            if head_data.datatype <= L2NameType.L1_SUB_GRAPH_NO.value:
                bean_data = NameBean().decode(data, data_offset, head_data)
                res.setdefault(self.L2_NAME_DICT.get(head_data.datatype), bean_data.name)
                data_offset += head_data.length
            elif head_data.datatype <= L2NameType.OUTPUT.value:
                data_offset = self._get_format_and_type(data, res, data_offset, head_data.datatype)
            else:
                data_offset += head_data.length
        return data_offset

    def _get_format_and_type(self, data: bytes, res: dict, data_offset: int, name_type: int) -> int:
        num = NumBean().decode(data, data_offset)
        data_offset += num.fmt_size
        shape_type = self.L2_NAME_DICT.get(name_type)
        bean = self.BEAN_DICT.get(name_type)()
        for _ in range(num.num):
            bean_data = bean.decode(data, data_offset)
            data_type = self._add_semicolon_char(res.get(shape_type + "_data_type", ""))
            data_format = self._add_semicolon_char(res.get(shape_type + "_format", ""))
            res.update(
                {shape_type + "_data_type": data_type + str(bean_data.data_type),
                 shape_type + "_format": data_format + str(bean_data.format)})
            data_offset += bean_data.fmt_size
            self._get_shape(data, res, data_offset, bean_data.length, shape_type)
            data_offset += bean_data.length
        return data_offset

    def _get_shape(self, data: bytes, res: dict, *base_info: any):
        data_offset, length, shape_type = base_info
        offset = 0
        while offset < length:
            head_data = TypeHeadBean().decode(
                data, offset + data_offset)
            offset += head_data.fmt_size
            if head_data.datatype == L3NameType.SHAPE.value:
                bean_data = ShapeBean().decode(
                    data, offset + data_offset, head_data)
                shape = self._add_semicolon_char(res.get(shape_type + "_shape", ""))
                res.update({shape_type + "_shape": shape + bean_data.shape})
            offset += head_data.length

    def _add_semicolon_char(self, s: str) -> str:
        return s + self.SEMICOLON_CHAR if s else s

    def _read_data(self: any, file_path: str, model_id: int) -> None:
        file_size = os.path.getsize(file_path)
        data_offset = 0
        if not file_size or file_size > self.DBG_MAX_SIZE:
            logging.error("The dbg file's size is invalid, is %d", file_size)
            error("The dbg file's size is invalid!")
            return
        with FileOpen(file_path, 'rb') as _open_file:
            _all_data = _open_file.file_reader.read(file_size)
            data_offset = self._check_magic_num(_all_data, data_offset)
            # parse dbg by struct
            self._get_task_info(_all_data, data_offset, file_size, model_id)

    def _get_nano_model_filename_with_id_data(self) -> dict:
        with NanoGraphAddInfoViewModel(self._project_path) as _model:
            return _model.get_nano_model_filename_with_model_id_data()

    def _get_op_name(self: any, dbg_dict: dict) -> str:
        # if exist l1_sub_graph_no, op_name should be original_name
        if "l1_sub_graph_no" in dbg_dict:
            return dbg_dict.get("original_name", Constant.NA)
        return dbg_dict.get("op_name", Constant.NA)

    def _reformat_ge_task_data(self: any, data_list: list) -> list:
        with NanoGraphAddInfoViewModel(self._project_path) as _model:
            thread_id_dict = _model.get_nano_thread_id_with_model_id_data()
        return [
            [
                data.get("model_id", self.INVALID_ID), self._get_op_name(data),
                data.get("stream_id", self.INVALID_VALUE), data.get("task_id", self.INVALID_ID),
                data.get("block_dim", self.INVALID_BLOCK_DIM), self.INVALID_BLOCK_DIM,
                self.INVALID_OP_STATE, data.get("task_type", Constant.NA), data.get("op_type", Constant.NA),
                self.INVALID_INDEX_ID, thread_id_dict.get(data.get("model_id", self.INVALID_ID), self.INVALID_ID),
                self.INVALID_TIMESTAMP, self.INVALID_BATCH_ID, self.INVALID_TENSOR_NUM,
                data.get("input_format", Constant.NA), data.get("input_data_type", Constant.NA),
                data.get("input_shape", Constant.NA), data.get("output_format", Constant.NA),
                data.get("output_data_type", Constant.NA), data.get("output_shape", Constant.NA),
                self.INVALID_DEVICE_ID, self.INVALID_CONTEXT_ID
            ] for data in data_list
        ]

    def _reformat_host_task_data(self: any, data_list: list) -> list:
        return [
            [
                data.get("model_id", self.INVALID_ID), self.INVALID_REQUEST_ID,
                data.get("stream_id", self.INVALID_VALUE), data.get("task_id", self.INVALID_ID),
                str(self.INVALID_CONTEXT_ID), self.INVALID_BATCH_ID, data.get("task_type", Constant.NA),
                self.INVALID_DEVICE_ID, self.INVALID_TIMESTAMP
            ] for data in data_list
        ]
