#!/usr/bin/python3
# coding=utf-8
"""
function: calculator for acl.
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import logging

from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from common_func.utils import Utils
from mscalculate.acl.acl_sql_calculator import AclSqlCalculator
from mscalculate.interface.icalculator import ICalculator
from profiling_bean.prof_enum.data_tag import AclApiTag


class AclCalculator(ICalculator, MsMultiProcess):
    """
    calculator for acl
    """

    def __init__(self: any, _: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._acl_data = []

    def _get_acl_data(self: any) -> list:
        acl_data = AclSqlCalculator.select_acl_data(self._get_acl_db_path(), DBNameConstant.TABLE_ACL_DATA)
        return acl_data if acl_data else []

    def _get_acl_db_path(self: any) -> str:
        return PathManager.get_db_path(self._project_path, DBNameConstant.DB_ACL_MODULE)

    def _get_acl_hash_dict(self: any) -> dict:
        acl_hash_dict = {}
        acl_hash_data = AclSqlCalculator.select_acl_hash_data(self._get_acl_hash_db_path(),
                                                              DBNameConstant.TABLE_HASH_ACL)
        for _acl_hash_data in acl_hash_data:
            acl_hash_dict.setdefault(*_acl_hash_data)
        return acl_hash_dict

    def _get_acl_hash_db_path(self: any) -> str:
        return PathManager.get_db_path(self._project_path, DBNameConstant.DB_HASH)

    def _refresh_acl_api_name(self: any) -> None:
        """
        refresh the acl api name
        :return: NA
        """
        logging.info("start to refresh the acl api name.")
        acl_hash_dict = self._get_acl_hash_dict()
        self._acl_data = Utils.generator_to_list((acl_hash_dict.get(_data[0], _data[0]),) + _data[1:]
                                                 for _data in self._acl_data)

    def _refresh_acl_api_type(self: any) -> None:
        """
        refresh the acl api type
        :return: NA
        """
        logging.info("start to refresh the acl api type.")
        self._acl_data = Utils.generator_to_list((_data[0], self._update_api_type(_data[1])) + _data[2:]
                                                 for _data in self._acl_data)

    @staticmethod
    def _update_api_type(acl_api_value: str) -> str:
        """
        transfer the acl api value to enum
        :param acl_api_value: 0,1,2,3
        :return: acl api tag
        """
        return AclApiTag(int(acl_api_value)).name if acl_api_value.isdigit() else acl_api_value

    def calculate(self: any) -> None:
        """
        calculate the acl data
        :return: None
        """
        logging.info("start to calculate the data of acl")
        self._acl_data = self._get_acl_data()
        self._refresh_acl_api_name()
        self._refresh_acl_api_type()
        self.save()

    def save(self: any) -> None:
        """
        save the data of acl
        :return: None
        """
        logging.info("calculating acl data finished, and starting to update the acl data.")
        AclSqlCalculator.delete_acl_data(self._get_acl_db_path(), DBNameConstant.TABLE_ACL_DATA)
        AclSqlCalculator.insert_data_to_db(self._get_acl_db_path(), DBNameConstant.TABLE_ACL_DATA, self._acl_data)

    def ms_run(self: any) -> None:
        """
        entrance for acl calculator
        :return: None
        """
        self.calculate()
