#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.base_model import BaseModel


class AscendTaskModel(BaseModel):
    """
    class used to operate top-down task
    """

    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super(AscendTaskModel, self).__init__(result_dir, DBNameConstant.DB_ASCEND_TASK, table_list)
