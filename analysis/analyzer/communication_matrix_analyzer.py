#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import os
import logging
import json
from collections import defaultdict

from msmodel.cluster_info.communication_model import CommunicationModel
from common_func.msprof_common import prepare_for_analyze
from common_func.db_name_constant import DBNameConstant
from common_func.msprof_exception import ProfException
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import OpBandWidthType
from common_func.common import error, warn
from common_func.common import print_msg
from common_func.path_manager import PathManager
from common_func.msprof_common import get_path_dir
from common_func.msprof_common import check_path_valid
from common_func.data_check_manager import DataCheckManager
from common_func.ms_constant.str_constant import StrConstant
from common_func.constant import Constant
from common_func.ms_constant.str_constant import CommunicationMatrixInfo
from msparser.cluster.communication_matrix_parser import CommunicationMatrixParser
from common_func.msvp_common import create_json_for_dict
from framework.load_info_manager import LoadInfoManager


class CommunicationMatrixAnalyzer:
    """
    single NPU communication data analyzer
    """
    FILE_NAME = os.path.basename(__file__)
    HOST_PATH = 'host'
    TRANSPORT_TYPE = {
        0: 'HCCS',
        1: 'PCIE',
        2: 'RDMA',
        3: 'LOCAL'
    }

    def __init__(self: any, collection_path: any) -> None:
        self.collection_path = collection_path
        self.hccl_op_data = defaultdict(list)

    def process(self):
        """Analyzing Communication Data"""
        self._process_sub_dirs()

    def _process_output(self, output_data: list) -> dict:
        """Delete unnecessary fields in dict"""
        proc_dict = {}
        for data in output_data:
            proc_dict[data.get(StrConstant.OP_NAME)] = {}
            link_info = data.get(StrConstant.LINK_INFO)
            for info in link_info:
                src_rank = info.get(CommunicationMatrixInfo.SRC_RANK)
                dst_rank = info.get(CommunicationMatrixInfo.DST_RANK)
                link_key = f"{src_rank}-{dst_rank}"
                proc_dict[data.get(StrConstant.OP_NAME)][link_key] = {
                    CommunicationMatrixInfo.TRANSPORT_TYPE:
                        self.TRANSPORT_TYPE.get(info.get(CommunicationMatrixInfo.TRANSPORT_TYPE)),
                    CommunicationMatrixInfo.TRANSIT_SIZE_MB: info.get(CommunicationMatrixInfo.TRANSIT_SIZE_MB),
                    CommunicationMatrixInfo.TRANSIT_TIME_MS: info.get(CommunicationMatrixInfo.TRANSIT_TIME_MS),
                    CommunicationMatrixInfo.BANDWIDTH_GB_S: info.get(CommunicationMatrixInfo.BANDWIDTH_GB_S)
                }
        return proc_dict

    def _get_hccl_data_from_db(self: any, rank_path: str) -> None:
        """
        get op events of all rank by iteration start and end time
        """
        with CommunicationModel(rank_path) as _model:
            if not _model.check_table():
                logging.error("Fail to connect %s, hccl parser is interrupted", DBNameConstant.DB_HCCL_SINGLE_DEVICE)
                raise ProfException(ProfException.PROF_INVALID_CONNECT_ERROR)
            conditions = {
                'iter_start': NumberConstant.DEFAULT_START_TIME,
                'iter_end': NumberConstant.DEFAULT_END_TIME
            }
            events_all = _model.get_all_events_from_db(conditions)
            if not events_all:
                print_msg(f"Fail to get hccl events, "
                          f"please check hccl_single_device.db from {rank_path}")
                raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)
            for event in events_all:
                # get group_name from hccl_single_device.db
                op_name = event.op_name + "@" + event.group_name
                self.hccl_op_data[op_name].append(event)

    def _generate_parser(self: any, rank_path) -> CommunicationMatrixParser:
        self._get_hccl_data_from_db(rank_path)

        if not self.hccl_op_data:
            message = f"fail to get hccl data"
            logging.error("Can't get hccl events!")
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR, message)

        return CommunicationMatrixParser(self.hccl_op_data)

    def _generate_output(self, rank_path: str) -> None:
        """
        Generate output json file
        """
        communication_parser = self._generate_parser(rank_path)
        op_info = communication_parser.run()
        output_result = self._process_output(op_info)
        output_file_name = "communication_matrix.json"
        save_dir = os.path.dirname(os.path.realpath(rank_path))
        output_file_path = PathManager.get_analyze_result_path(save_dir, output_file_name)
        result = create_json_for_dict(output_file_path, output_result)
        result_json = json.loads(result)
        if result_json["status"] == NumberConstant.SUCCESS:
            print_msg(result)
        else:
            print_msg(json.dumps(
                {'status': NumberConstant.ERROR,
                 'info': f'communication matrix data generation failed, '
                         f'maybe you can check the directory({self.collection_path}) permissions.',
                 'data': ''}))

    def _process_sub_dirs(self: any, sub_path: str = '', is_cluster: bool = False) -> None:
        collect_path = self.collection_path
        if sub_path:
            collect_path = os.path.join(self.collection_path, sub_path)
        sub_dirs = sorted(get_path_dir(collect_path), reverse=True)
        for sub_dir in sub_dirs:  # result_dir
            if sub_dir == StrConstant.TIMELINE_PATH or sub_dir == self.HOST_PATH:
                continue

            sub_path = os.path.realpath(
                os.path.join(collect_path, sub_dir))
            check_path_valid(sub_path, False)

            if DataCheckManager.contain_info_json_data(sub_path):
                LoadInfoManager.load_info(sub_path)
                self._communication_matrix_analyze(sub_path)
            elif sub_path and is_cluster:
                warn(self.FILE_NAME, 'Invalid parsing dir("%s"), -dir must be profiling data dir '
                                     'such as PROF_XXX_XXX_XXX' % collect_path)
            else:
                self._process_sub_dirs(sub_dir, is_cluster=True)

    def _communication_matrix_analyze(self, sub_path: str) -> None:
        """ communication matrix analyzer"""
        prepare_for_analyze(os.path.join(sub_path, '..'))
        if os.path.exists(PathManager.get_db_path(sub_path, DBNameConstant.DB_HCCL_SINGLE_DEVICE)):
            self._generate_output(sub_path)
        else:
            logging.warning('There is not hccl_single_device.db in %s', sub_path)
