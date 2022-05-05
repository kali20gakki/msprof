# coding=utf-8
"""
function: script used to parse acsq task data and save it to db
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import logging
import sqlite3

from model.stars.op_summary_model import OpSummaryModel


class OpSummaryViewer:
    """
    class used to export stars op summary csv
    """

    def __init__(self: any, configs: dict, params: dict) -> None:
        self.configs = configs
        self.params = params

    def get_summary_data(self: any) -> any:
        """

        """
        try:
            with OpSummaryModel(self.configs) as op_model:
                return op_model.get_summary_data()
        except sqlite3.Error as err:
            logging.error("Create ge summary tables failed! %s", err)
            return []
        finally:
            pass
