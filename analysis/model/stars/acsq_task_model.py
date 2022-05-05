# coding=utf-8
"""
acsq task model
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from model.interface.parser_model import ParserModel
from model.sqe_type_map import SqeType


class AcsqTaskModel(ParserModel):
    """
    acsq task model class
    """

    def flush(self: any, data_list: list) -> None:
        """
        flush acsq task data to db
        :param data_list:acsq task data list
        :return: None
        """
        self.insert_data_to_db(DBNameConstant.TABLE_ACSQ_TASK, data_list)

    def get_summary_data(self: any) -> list:
        """
        get op_summary data from table
        :return: op_summary data list
        """
        return self.get_all_data(DBNameConstant.TABLE_ACSQ_TASK)

    def get_timeline_data(self: any) -> list:
        """
        get timeline data from table
        :return: timeline data list
        """
        return self.get_all_data(DBNameConstant.TABLE_ACSQ_TASK)

    def get_ffts_type_data(self: any) -> list:
        """
        get all ffts type data
        :param step_start:
        :param step_end:
        :return: list
        """
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_ACSQ_TASK):
            return []
        sql = "select 0, task_id, stream_id, start_time, task_time " \
              "from {0} " \
              "where task_type={task_type}".format(DBNameConstant.TABLE_ACSQ_TASK,
                                                   task_type=SqeType.FFTS_SQE.name)
        return DBManager.fetch_all_data(self.cur, sql)
