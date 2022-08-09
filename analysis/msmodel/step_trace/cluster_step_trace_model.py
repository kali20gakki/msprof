# !/usr/bin/python
# coding=utf-8
"""
This script is used to create step trace database model for cluster.
Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
"""

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.base_model import BaseModel


class ClusterStepTraceModel(BaseModel):
    """
    Step trace model for cluster scene.
    """
    FORMAT_TABLE_NAME = "ClusterStepTrace"

    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super().__init__(result_dir, DBNameConstant.DB_CLUSTER_STEP_TRACE, table_list)

    def create_table(self: any) -> None:
        table_map = "{0}Map".format(ClusterStepTraceModel.FORMAT_TABLE_NAME)
        for table_name in self.table_list:
            if DBManager.judge_table_exist(self.cur, table_name):
                DBManager.drop_table(self.conn, table_name)
            sql = DBManager.sql_create_general_table(table_map, table_name, self.TABLES_PATH)
            DBManager.execute_sql(self.conn, sql)

    def get_model_info(self: any, table_name: str) -> list:
        if not DBManager.judge_table_exist(self.cur, table_name):
            return []
        sql = 'select t0.model_id, t0.max_index, group_concat(t0.iteration_id ) from (select t.* from' \
              '(select iteration_id, model_id, ge_tag, (select count( * ) + 1 from {0} as t2 where ' \
              't2.model_id = t1.model_id and t2.iteration_time > t1.iteration_time ) as top,' \
              '(select count(0) from {0} as t3 where t3.model_id = t1.model_id ) as max_index ' \
              'from {0} as t1 ) as t where top <= 5 and ge_tag = 1 order by model_id, top)t0 ' \
              'group by model_id, max_index'.format(table_name)
        return DBManager.fetch_all_data(self.cur, sql)
