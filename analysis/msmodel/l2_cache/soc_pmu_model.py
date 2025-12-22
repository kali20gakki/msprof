#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel


class SocPmuModel(ParserModel):
    """
    db operator for soc_pmu parser
    """

    def __init__(self: any, result_dir: str, tale_list: list) -> None:
        super(SocPmuModel, self).__init__(result_dir, DBNameConstant.DB_SOC_PMU, tale_list)

    def flush(self: any, data_list: list, table_name: str = DBNameConstant.TABLE_SOC_PMU) -> None:
        """
        insert data into database
        """
        self.insert_data_to_db(table_name, data_list)


class SocPmuCalculatorModel(ParserModel):
    """
    db operator for soc_pmu calculator
    """

    RAW_DATA_EVENTS_INDEX = -1

    def __init__(self: any, result_dir: str) -> None:
        super(SocPmuCalculatorModel, self).__init__(result_dir, DBNameConstant.DB_SOC_PMU,
                                                    [DBNameConstant.TABLE_SOC_PMU_SUMMARY])

    @staticmethod
    def split_events_data(soc_pmu_ps_data: list) -> list:
        """
        Split events string in soc_pmu_ps_data into a list of events.
        """
        if not soc_pmu_ps_data or len(soc_pmu_ps_data[0]) <= 1:
            return []
        result = []
        for data in soc_pmu_ps_data:
            base = list(data[: SocPmuCalculatorModel.RAW_DATA_EVENTS_INDEX])
            events = data[SocPmuCalculatorModel.RAW_DATA_EVENTS_INDEX].split(',')
            base.extend(events)
            result.append(base)
        return result

    def flush(self: any, data_list: list, table_name: str = DBNameConstant.TABLE_SOC_PMU_SUMMARY) -> None:
        """
        insert data into database
        """
        self.insert_data_to_db(table_name, data_list)


class SocPmuViewerModel(ViewModel):
    """
    soc pmu viewer model class
    """
    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        super().__init__(result_dir, db_name, table_list)

    def get_summary_data(self: any) -> list:
        """
        get soc pmu data
        :return: list
        """
        sql = "SELECT stream_id, task_id, hit_rate, miss_rate FROM {}".format(DBNameConstant.TABLE_SOC_PMU_SUMMARY)
        return DBManager.fetch_all_data(self.cur, sql)
