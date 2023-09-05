#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
import json
import os
import glob
import logging
from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.view_model import ViewModel
from viewer.runtime_report import add_mem_bound, add_cube_usage


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
    def add_cube_usage(cls: any, project_path: str, headers: list, data: list) -> None:
        """
        add cube usage data
        """
        info_json_path = None
        config_dict = dict()
        for file in glob.glob(os.path.join(project_path, 'info.json.*[0-9]')):
            info_json_path = file
        try:
            with open(info_json_path, "r") as json_reader:
                json_data = json_reader.readline(Constant.MAX_READ_LINE_BYTES)
                json_data = json.loads(json_data)
                device_info = json_data.get('DeviceInfo')[0]
                config_dict['ai_core_num'] = device_info.get('ai_core_num')
                config_dict['aic_frequency'] = float(device_info.get('aic_frequency'))
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            logging.error("Cube utilization: info_json data read fail")
            return

        try:
            config_dict.setdefault('mac_ratio_index', None)
            config_dict.setdefault('ratio_index_int8', None)
            config_dict.setdefault('ratio_index_fp16', None)
            for item in headers:
                if item in ("mac_ratio", "aic_mac_ration"):
                    config_dict['mac_ratio_index'] = headers.index("mac_ratio") if "mac_ratio" in headers else \
                        headers.index("aic_mac_ratio")
                elif item == "mac_int8_ratio":
                    config_dict['ratio_index_int8'] = headers.index("mac_int8_ratio")
                elif item == "mac_fp16_ratio":
                    config_dict['ratio_index_fp16'] = headers.index("mac_fp16_ratio")

            config_dict['total_cycles_index'] = headers.index("total_cycles") if "total_cycles" in headers else None
            config_dict['task_duration_index'] = headers.index("Task Duration(us)") if "Task Duration(us)" \
                                                                                       in headers else None
            if config_dict.get('task_duration_index') and config_dict.get('total_cycles_index') and \
                    (config_dict.get("mac_ratio_index") or config_dict.get('ratio_index_int8') or
                     config_dict.get('ratio_index_fp16')):
                headers.append("cube_utilization(%)")
            else:
                return
            for index, row in enumerate(data):
                data[index] = add_cube_usage(config_dict, list(row))
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            logging.error("Cube utilization: cube info not found")
            return

    @classmethod
    def add_memory_bound(cls: any, headers: list, data: list) -> None:
        """
        add memory bound data
        """
        if all(header in headers for header in ("mac_ratio", "vec_ratio", "mte2_ratio")):
            mte2_index = headers.index("mte2_ratio")
            vec_index = headers.index("vec_ratio")
            mac_index = headers.index("mac_ratio")
            headers.append("memory_bound")
            for index, row in enumerate(data):
                data[index] = add_mem_bound(list(row), vec_index, mac_index, mte2_index)

        elif all(header in headers for header in ("mac_exe_ratio", "vec_exe_ratio", "mte2_exe_ratio")):
            mte2_index = headers.index("mte2_exe_ratio")
            vec_index = headers.index("vec_exe_ratio")
            mac_index = headers.index("mac_exe_ratio")
            headers.append("memory_bound")
            for index, row in enumerate(data):
                data[index] = add_mem_bound(list(row), vec_index, mac_index, mte2_index)

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
