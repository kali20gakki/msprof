#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to operate systerm memory
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""
from abc import ABC

from common_func.db_name_constant import DBNameConstant
from model.interface.base_model import BaseModel


class SysMemModel(BaseModel, ABC):
    """
    acsq task model class
    """

    @staticmethod
    def class_name() -> None:
        """
        class name
        """
        return "SysMemModel"

    def flush(self: any, data_list: dict) -> None:
        """
        flush acsq task data to db
        :param data_list:acsq task data list
        :return: None
        """
        if data_list.get('sys_data_list'):
            self.insert_data_to_db(DBNameConstant.TABLE_SYS_MEM, data_list.get('sys_data_list'))
        if data_list.get('pid_data_list'):
            self.insert_data_to_db(DBNameConstant.TABLE_PID_MEM, data_list.get('pid_data_list'))
