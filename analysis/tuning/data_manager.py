"""
Copyright Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
"""

import logging
import os
import sqlite3

from common_func.common import CommonConstant
from common_func.common import generate_config
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.get_export_data_config import GetExportDataConfigs
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_constant import MsvpConstant
from common_func.path_manager import PathManager
from viewer.ai_core_op_report import AiCoreOpReport
from viewer.ai_core_report import get_core_sample_data
from viewer.runtime_report import get_task_based_core_data


class DataManager:
    """
    get operator data by infer id
    """
    @staticmethod
    def get_memory_workspace(memory_workspaces: list, operator_dict: dict) -> None:
        """
        get data for memory workspace
        """
        if memory_workspaces:
            for memory_workspace in memory_workspaces:
                if operator_dict.get("stream_id") == memory_workspace[0] \
                        and str(operator_dict.get("task_id")) == memory_workspace[1]:
                    operator_dict["memory_workspace"] = memory_workspace[2]
                    return
        operator_dict["memory_workspace"] = 0

    @staticmethod
    def is_network(project: str, device_id: any) -> bool:
        """
        check the scene of network
        """
        run_for_network = True
        conn, cur = DBManager.check_connect_db(project, DBNameConstant.DB_ACL_MODULE)
        try:
            if conn and cur:
                acl_op_data = cur.execute("select count(api_name) from acl_data where api_name=? "
                                          "and device_id=?",
                                          ("aclopExecute", device_id)).fetchone()
                if acl_op_data[0]:
                    run_for_network = False
            return run_for_network
        except sqlite3.Error as error:
            logging.error(error)
            return False
        finally:
            DBManager.destroy_db_connect(conn, cur)

    @staticmethod
    def _process_headers(headers: list) -> list:
        result_headers = []
        for header in headers:
            result_headers.append(header.replace(" ", "_").split("(")[0].lower())
        return result_headers

    @classmethod
    def get_data_by_infer_id(cls: any, project_path: str, device_id: any, infer_id: any) -> list:
        """
        get data by iter id.
        """
        op_data = []
        memory_workspaces = cls.select_memory_workspace(project_path, device_id)
        headers, datas = cls._get_base_data(device_id, infer_id, project_path)
        headers = cls._process_headers(headers)
        for data in datas:
            operator_dict = {}
            for key, value in zip(headers, data):
                operator_dict[key] = value
            cls._get_extend_data(operator_dict)
            cls.get_memory_workspace(memory_workspaces, operator_dict)
            op_data.append(operator_dict)
        return op_data

    @classmethod
    def _get_base_data(cls: any, device_id: any, infer_id: any, project_path: str) -> tuple:
        if cls.is_network(project_path, device_id):
            configs = {
                StrConstant.CONFIG_HEADERS: GetExportDataConfigs.get_data_headers(StrConstant.AI_CORE_OP_SUMMARY)}
            db_path = PathManager.get_db_path(project_path, DBNameConstant.DB_AICORE_OP_SUMMARY)
            headers, data, _ = AiCoreOpReport.get_ai_core_op_summary_data(project_path, db_path,
                                                                          infer_id, configs)
        else:
            param = {}
            headers, data, _ = MsvpConstant.MSVP_EMPTY_DATA
            sample_config = generate_config(os.path.join(project_path, CommonConstant.SAMPLE_JSON))

            param[StrConstant.DATA_TYPE] = StrConstant.AI_CORE_PMU_EVENTS
            if sample_config.get(StrConstant.AICORE_PROFILING_MODE) == MsvpConstant.AI_CORE_PROFILING_MODE_LIST[0]:
                headers, data, _ = get_task_based_core_data(project_path, DBNameConstant.DB_RUNTIME,
                                                            param)
            elif sample_config.get(StrConstant.AICORE_PROFILING_MODE) == MsvpConstant.AI_CORE_PROFILING_MODE_LIST[1]:
                param[StrConstant.CORE_DATA_TYPE] = StrConstant.AI_CORE_PMU_EVENTS
                headers, data, _ = get_core_sample_data(project_path, "aicore_{}.db", device_id,
                                                        param)

            if not headers or not data:
                param[StrConstant.DATA_TYPE] = StrConstant.AI_VECTOR_CORE_PMU_EVENTS
                if sample_config.get(StrConstant.AIV_PROFILING_MODE) == MsvpConstant.AI_CORE_PROFILING_MODE_LIST[0]:
                    headers, data, _ = get_task_based_core_data(project_path, DBNameConstant.DB_RUNTIME,
                                                                param)
                elif sample_config.get(StrConstant.AIV_PROFILING_MODE) == MsvpConstant.AI_CORE_PROFILING_MODE_LIST[1]:
                    headers, data, _ = get_core_sample_data(project_path, "ai_vector_core_{}.db",
                                                            device_id, param)
        return headers, data

    @classmethod
    def _get_extend_data(cls: any, operator_dict: dict) -> dict:
        extend_data_dict = {}
        cls.get_core_number(extend_data_dict)
        cls.get_vector_bound(extend_data_dict, operator_dict)
        operator_dict.update(extend_data_dict)
        return extend_data_dict

    @classmethod
    def get_vector_bound(cls: any, extend_data_dict: dict, operator_dict: dict) -> None:
        """
        get data for vector bound
        """
        if operator_dict.get("vec_ratio") and operator_dict.get("mte2_ratio") \
                and operator_dict.get("mac_ratio"):
            extend_data_dict["vector_bound"] = 0
            if max(operator_dict.get("mte2_ratio"), operator_dict.get("mac_ratio")):
                extend_data_dict["vector_bound"] = \
                    StrConstant.ACCURACY % float(operator_dict.get("vec_ratio") /
                    max(operator_dict.get("mte2_ratio"), operator_dict.get("mac_ratio")))

    @classmethod
    def get_core_number(cls: any, extend_data_dict: dict) -> None:
        """
        get core number
        """
        extend_data_dict["core_num"] = InfoConfReader().get_data_under_device("ai_core_num")

    @classmethod
    def select_memory_workspace(cls: any, project: str, device_id: any) -> list:
        """
        query memory workspace from db
        """
        memory_workspaces = []
        if not cls.is_network(project, device_id):
            return memory_workspaces
        conn, cur = DBManager.check_connect_db(project, DBNameConstant.DB_GE_MODEL_INFO)
        if conn and cur and DBManager.judge_table_exist(cur, DBNameConstant.TABLE_GE_LOAD_TABLE):
            sql = "select stream_id, task_ids, memory_workspace " \
                  "from GELoad where memory_workspace>0 " \
                  "and device_id=?"
            memory_workspaces = DBManager.fetch_all_data(cur, sql, (device_id,))
        DBManager.destroy_db_connect(conn, cur)
        return memory_workspaces
