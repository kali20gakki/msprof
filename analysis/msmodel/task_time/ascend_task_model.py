#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

from typing import List

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from mscalculate.ascend_task.ascend_task import TopDownTask
from msmodel.interface.base_model import BaseModel


class AscendTaskModel(BaseModel):
    """
    class used to operate top-down task
    """

    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super(AscendTaskModel, self).__init__(result_dir, DBNameConstant.DB_ASCEND_TASK, table_list)

    def get_ascend_task_data_without_unknown(self: any) -> List[TopDownTask]:
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_ASCEND_TASK):
            return []
        param = (
            Constant.TASK_TYPE_UNKNOWN,
        )
        data_sql = "select * from {} " \
                   "where device_task_type != ?".format(DBNameConstant.TABLE_ASCEND_TASK)
        ascend_task_data = DBManager.fetch_all_data(self.cur, data_sql, param=param)
        return [TopDownTask(*data) for data in ascend_task_data]
