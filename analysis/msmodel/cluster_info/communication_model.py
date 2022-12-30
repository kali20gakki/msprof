#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
from profiling_bean.db_dto.hccl_dto import HcclDto
from msmodel.interface.view_model import ViewModel
from common_func.db_name_constant import DBNameConstant
from common_func.db_manager import DBManager


class CommunicationModel(ViewModel):
    """
    get hccl operators data from db
    """

    def __init__(self, collection_path):
        super().__init__(collection_path, DBNameConstant.DB_HCCL, [])

    def get_hccl_data_by_conditions(self: any, conditions: dict) -> list:
        """
        get hccl data by conditions[op_name, iteration]
        :params conditions: dict contains op_name, iter_start, iter_end
        :return: communication operators event list
        """
        sql = "select * from {0} where first_timestamp < ? and first_timestamp >= ? and op_name = ?". \
            format(DBNameConstant.TABLE_HCCL_ALL_REDUCE)
        data = DBManager.fetch_all_data(self.cur, sql,
                                        (conditions.get('iter_end', 0), conditions.get('iter_start', float('inf')),
                                         conditions.get('op_name', '')), dto_class=HcclDto)
        return data

    def get_distinct_op_name(self: any) -> list:
        """
        get hccl op names
        :return:
        """
        sql = "select distinct op_name from {}".format(DBNameConstant.TABLE_HCCL_ALL_REDUCE)
        data = DBManager.fetch_all_data(self.cur, sql, dto_class=HcclDto)
        return data
