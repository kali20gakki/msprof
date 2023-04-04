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

    def get_all_events_from_db(self: any, conditions: dict) -> list:
        """
        get hccl op names
        :return:
        """
        sql = "select * from {0} where first_timestamp < ? and first_timestamp >= ?".\
            format(DBNameConstant.TABLE_HCCL_ALL_REDUCE)
        data = DBManager.fetch_all_data(self.cur, sql, (conditions.get('iter_end', 0),
                                                        conditions.get('iter_start', float('inf'))), dto_class=HcclDto)
        return data
