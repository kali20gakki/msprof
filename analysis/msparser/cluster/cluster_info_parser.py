"""
This script is used to parse cluster rank_id data to db.
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
import json
import logging
import os
import sqlite3

from common_func.constant import Constant
from common_func.file_manager import check_path_valid
from common_func.file_name_manager import get_info_json_compiles
from common_func.info_conf_reader import InfoConfReader
from common_func.msprof_common import get_path_dir
from msmodel.cluster_info.cluster_info_model import ClusterInfoModel
from msparser.interface.iparser import IParser


class ClusterInfoParser(IParser):
    """
    parser of rank_id data
    """

    def __init__(self: any, collect_path: str) -> None:
        self.collect_path = collect_path
        self.cluster_info_list = []

    def ms_run(self: any):
        self.parse()
        self.save()

    def parse(self: any) -> None:
        first_sub_dirs = get_path_dir(self.collect_path)
        for first_sub_dir in first_sub_dirs:
            if first_sub_dir == 'sqlite':
                continue
            first_sub_path = os.path.realpath(
                os.path.join(self.collect_path, first_sub_dir))
            second_sub_dirs = get_path_dir(first_sub_path)
            for second_sub_dir in second_sub_dirs:
                if second_sub_dir.find('device') != -1:
                    second_sub_path = os.path.realpath(
                            os.path.join(first_sub_path, second_sub_dir))
                    info_json_path = InfoConfReader().get_conf_file_path(second_sub_path, get_info_json_compiles())
                    if not info_json_path or not os.path.exists(info_json_path) or not os.path.isfile(
                            info_json_path):
                        continue
                    check_path_valid(info_json_path, is_file=True)
                    try:
                        with open(info_json_path, "r") as json_reader:
                            json_data = json_reader.readline(Constant.MAX_READ_LINE_BYTES)
                            json_data = json.loads(json_data)
                            _rank_id = json_data.get("rankID")
                            _device_id = json_data.get("devices")
                            cluster_info = [_rank_id, _device_id, first_sub_dir]
                            self.cluster_info_list.append(cluster_info)
                    except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
                        logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
                        logging.error("json data decode fail")
                        continue


    def save(self: any) -> None:
        try:
            if self.cluster_info_list:
                cluster_model = ClusterInfoModel(self.collect_path)
                if cluster_model.init():
                    cluster_model.flush(self.cluster_info_list)
                    cluster_model.finalize()
        except sqlite3.Error as trace_err:
            logging.error("Save time failed, "
                          "%s", str(trace_err), exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            pass
