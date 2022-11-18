#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.hccl_operator_dto import HCCLOperatorDto


class ClusterHCCLModel(ParserModel):
    """
        class used to operate cluster_hccl db
    """

    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_CLUSTER_HCCL, [DBNameConstant.TABLE_HCCL_OPERATOR_EXE])

    def flush(self: any, data_list: list, table_name: str) -> None:
        """
        flush data to db
        """
        self.insert_data_to_db(table_name, data_list)


class ClusterHCCLViewModel(ViewModel):
    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_CLUSTER_HCCL, [])

    def get_hccl_op_data(self: any) -> list:
        sql = "select * from HcclOperatorExe where start_time<>-1 and end_time<>-1 order by start_time"
        return DBManager.fetch_all_data(self.cur, sql, dto_class=HCCLOperatorDto)
