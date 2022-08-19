#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to operate AI_CPU
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""

from msmodel.interface.base_model import BaseModel
from common_func.db_name_constant import DBNameConstant
from common_func.db_manager import DBManager
from profiling_bean.prof_enum.chip_model import ChipModel
from msmodel.ai_cpu.ai_cpu_from_ts.ai_cpu_source_model_info import RuntimeModelInfo
from msmodel.ai_cpu.ai_cpu_from_ts.ai_cpu_source_model_info import TsTrackModelInfo


class AiCpuFromTsModel(BaseModel):
    CHIP_TO_SOURCE_MODEL = {
        ChipModel.CHIP_V1_1_0: RuntimeModelInfo,
        ChipModel.CHIP_V2_1_0: TsTrackModelInfo
    }

    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_AI_CPU, [DBNameConstant.TABLE_AI_CPU_FROM_TS])

    def flush(self: any, data_list: list) -> None:
        """
        insert data to table
        :param data_list: ai_cpu data
        :param ai_cpu_table_name: ai cpu table name
        :return:
        """
        self.insert_data_to_db(DBNameConstant.TABLE_AI_CPU_FROM_TS, data_list)

    def get_ai_cpu_data(self: any) -> list:
        """
        insert data to table
        :param data_list: ai_cpu data
        :param ai_cpu_table_name: ai cpu table name
        :return:
        """
        ai_cpu_from_ts = []
        if self._source_table is None or self._source_model_class is None:
            return ai_cpu_from_ts

        source_model = self._source_model_class()
        with self._source_model_class:
            sql = "select stream_id, task_id, timestamp, task_state from {0} " \
                  "order by timestamp".format(self._source_table)

            ai_cpu_from_ts = DBManager.fetch_all_data(self.cur, sql)
        return ai_cpu_from_ts