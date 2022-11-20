#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.

from common_func.db_name_constant import DBNameConstant
from msmodel.interface.view_model import ViewModel


class DataManager:
    """
    class to manage or modify data
    """

    ACCURACY = "%.6f"
    AI_CORE_TASK_TYPE = 0
    AI_CPU_TASK_TYPE = 1
    AI_VECTOR_CORE_TASK_TYPE = 66

    @staticmethod
    def get_op_dict(project_path: str) -> dict:
        """
        get operator dictionary with task id and stream id
        """
        model_view = ViewModel(project_path, DBNameConstant.DB_GE_INFO, [DBNameConstant.TABLE_GE_TASK])
        model_view.check_table()
        used_cols = "op_name, task_id, stream_id"
        search_data_sql = "select {1} from {0} order by rowid" \
            .format(DBNameConstant.TABLE_GE_TASK, used_cols)
        data = model_view.get_sql_data(search_data_sql)
        task_op_dict = {}
        for sub in data:
            key = "{}-{}".format(sub[1], sub[2])  # key is task_id-stream_id
            task_op_dict[key] = sub[0]  # value is op_name
        return task_op_dict

    @classmethod
    def add_memory_bound(cls: any, headers: list, data: list) -> None:
        """
        add memory bound data
        """
        if "mac_ratio" in headers and "vec_ratio" in headers and "mte2_ratio" in headers:
            mte2_index = headers.index("mte2_ratio")
            vec_index = headers.index("vec_ratio")
            mac_index = headers.index("mac_ratio")
            headers.append("memory_bound")
            for index, row in enumerate(data):
                memory_bound = 0
                if max(row[vec_index], row[mac_index]):
                    memory_bound = cls.ACCURACY % float(row[mte2_index] / max(row[vec_index], row[mac_index]))
                data[index] = list(row) + [memory_bound]

    @classmethod
    def add_iter_id(cls: any, *args: any, task_type_index: int = None, iter_id: int = 1) -> bool:
        """
        add iteration id
        """
        data, task_id_index, stream_id_index = args
        iter_id_record = {}
        for row_num, row in enumerate(data):
            if len(row) < task_id_index or len(row) < stream_id_index:
                return False
            if task_type_index is not None and str(task_type_index).isdigit() \
                    and row[int(task_type_index)] not in (cls.AI_CORE_TASK_TYPE,
                                                          cls.AI_CPU_TASK_TYPE,
                                                          cls.AI_VECTOR_CORE_TASK_TYPE):
                data[row_num] = list(row)
                data[row_num].append(0)
                continue
            iter_id_tag = "{}-{}".format(row[task_id_index], row[stream_id_index])
            iter_id_record[iter_id_tag] = iter_id_record.get(iter_id_tag, 0) + 1
            if iter_id_record.get(iter_id_tag) > iter_id:
                iter_id = iter_id_record.get(iter_id_tag)
            data[row_num] = list(row)
            data[row_num].append(iter_id)
        return True
