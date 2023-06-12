#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2022. All rights reserved.
import logging
from collections import namedtuple

from common_func.db_manager import DBManager
from common_func.ms_constant.number_constant import NumberConstant
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.hash_dict_constant import HashDictData
from msmodel.npu_mem.npu_op_mem_model import NpuOpMemModel
from mscalculate.interface.icalculator import ICalculator


class NpuOpMemCalculator(ICalculator, MsMultiProcess):
    """
    calculate npu op memory data
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)

        self._conn = None
        self._curs = None

        self._op_data = []
        self._model_raw = NpuOpMemModel(self._project_path, [DBNameConstant.TABLE_NPU_OP_MEM_RAW])
        self._model_mem = NpuOpMemModel(self._project_path, [DBNameConstant.TABLE_NPU_OP_MEM])
        self._model_rec = NpuOpMemModel(self._project_path, [DBNameConstant.TABLE_NPU_OP_MEM_REC])
        self._memory_record = []
        self._opeartor_memory = []


    def calculate(self: any) -> None:
        self._op_data = self._model_raw.get_table_data(DBNameConstant.TABLE_NPU_OP_MEM_RAW)
        self._calc_memory_record()
        self._calc_operator_memory()

    def save(self: any) -> None:
        """
        save data
        """
        if self._memory_record:
            self._model_rec.clear(DBNameConstant.TABLE_NPU_OP_MEM_REC)
            self._model_rec.init()
            self._model_rec.flush(DBNameConstant.TABLE_NPU_OP_MEM_REC, self._memory_record)
            self._model_rec.finalize()
        if self._opeartor_memory:
            self._model_mem.clear(DBNameConstant.TABLE_NPU_OP_MEM)
            self._model_mem.init()
            self._model_mem.flush(DBNameConstant.TABLE_NPU_OP_MEM, self._opeartor_memory)
            self._model_mem.finalize()

    def ms_run(self: any) -> None:
        """
        calculate for task scheduler
        :return:
        """
        self._connect_db()
        self.calculate()
        self.save()

    def _connect_db(self: any) -> None:
        self._model_raw.init()
        self._model_mem.init()
        self._model_rec.init()

    def _calc_operator_memory(self: any) -> None:
        allocated_data = {}
        allocated_size = 0
        OperatorKey = namedtuple('OperatorKey', ['operator', 'addr', 'device_type'])
        OperatorValue = namedtuple('OperatorValue',
                                   ['size', 'timestamp', 'total_allocate_memory',
                                    'total_reserve_memory', 'allocated_size'])
        for item in self._op_data:
            allocated_size += item.size
            if item.size > 0:
                item_key = OperatorKey(operator=item.operator, addr=item.addr, device_type=item.device_type)
                item_value = OperatorValue(size=item.size, timestamp=item.timestamp,
                                           total_allocate_memory=item.total_allocate_memory,
                                           total_reserve_memory=item.total_reserve_memory,
                                           allocated_size=allocated_size)
                allocated_data[item_key] = item_value
            elif item.size < 0:
                item_key = OperatorKey(operator=item.operator, addr=item.addr, device_type=item.device_type)
                item_value = OperatorValue(size=item.size, timestamp=item.timestamp,
                                           total_allocate_memory=item.total_allocate_memory,
                                           total_reserve_memory=item.total_reserve_memory,
                                           allocated_size=allocated_size)
                if item_key in allocated_data:
                    allocated_value = allocated_data[item_key]
                    op_mem = [
                        item.operator, allocated_value.size, allocated_value.timestamp,
                        item_value.timestamp, int(item_value.timestamp) - int(allocated_value.timestamp),
                        allocated_value.allocated_size, allocated_value.total_reserve_memory,
                        item_value.allocated_size, item_value.total_reserve_memory,
                        item_key.device_type
                    ]
                    self._opeartor_memory.append(op_mem)
                    allocated_data.pop(item_key)
        if len(allocated_data) > 0:
            for key, value in allocated_data.items():
                self._opeartor_memory.append([key.operator, value.size, value.timestamp,
                                              NumberConstant.EXCEPTION, NumberConstant.EXCEPTION,
                                              value.allocated_size, value.total_reserve_memory,
                                              NumberConstant.EXCEPTION, NumberConstant.EXCEPTION, key.device_type])
        self._reformat_data()

    def _calc_memory_record(self: any) -> None:
        self._memory_record = [['GE', item.timestamp, item.total_reserve_memory,
                                item.total_allocate_memory, item.device_type] for item in self._op_data]

    def _reformat_data(self: any) -> list:
        hash_dict_data = HashDictData(self._project_path)
        ge_dict = hash_dict_data.get_ge_hash_dict()
        for item in self._opeartor_memory:
            name = ''
            if item[0] in ge_dict:
                name = ge_dict[item[0]]
            item.append(name)