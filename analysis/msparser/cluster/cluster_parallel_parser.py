#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import json
import os

from common_func.common import error
from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_common import MsProfCommonConstant
from common_func.path_manager import PathManager
from msmodel.cluster_info.cluster_communication_model import ClusterCommunicationModel
from msmodel.cluster_info.cluster_info_model import ClusterInfoViewModel


class ClusterParallelParser:
    """
        cluster parallel data analysis
    """
    THRESHOLD_VALUE = 0.1
    TYPE_0_MESSAGE = "The communication time ratio of all NPU cards is good and " \
                     "below the threshold of %s." % format(THRESHOLD_VALUE, '.0%')
    TYPE_1_MESSAGE = "The communication time ratio of all NPU cards exceeds " \
                     "the threshold of %s." % format(THRESHOLD_VALUE, '.0%')
    TYPE_2_MESSAGE = "The communication time ratio of some NPU cards exceeds the threshold of %s. " \
                     "You need to check whether there are slow nodes or slow links." % format(THRESHOLD_VALUE, '.0%')

    def __init__(self, params: dict) -> None:
        self.collection_path = params["collection_path"]
        self._cluster_info_model = ClusterInfoViewModel(params["collection_path"])
        self._cluster_communication_model = ClusterCommunicationModel(params)
        self.parallel_analysis_result = {}

    def process(self: any) -> None:
        self._parallel_analysis()
        self._storage_parallel_analysis_result()

    def _get_device_and_rank_ids(self: any) -> any:
        if not os.path.exists(PathManager.get_db_path(self.collection_path, DBNameConstant.DB_CLUSTER_RANK)):
            error(MsProfCommonConstant.COMMON_FILE_NAME, "Cannot find the cluster_rank.db or Permission denied!")
            return Constant.DEFAULT_INVALID_VALUE
        if not os.path.exists(PathManager.get_db_path(self.collection_path, DBNameConstant.DB_CLUSTER_STEP_TRACE)):
            error(MsProfCommonConstant.COMMON_FILE_NAME, "Cannot find the cluster_step_trace.db or Permission denied!")
            return Constant.DEFAULT_INVALID_VALUE
        with self._cluster_info_model as _model:
            return _model.get_device_and_rank_ids()

    def _get_parallel_data(self: any, device_and_rank_ids: list) -> any:
        parallel_data = []
        with self._cluster_communication_model as _model:
            for device_and_rank in device_and_rank_ids:
                if device_and_rank[1] == Constant.NA:
                    id_name = 'device_id'
                    id_value = device_and_rank[0]
                else:
                    id_name = 'rank_id'
                    id_value = device_and_rank[1]
                communication_time_ratio = _model.get_communication_time_ratio(id_value)
                if communication_time_ratio == Constant.DEFAULT_INVALID_VALUE or communication_time_ratio is None:
                    error(MsProfCommonConstant.COMMON_FILE_NAME, "Invalid communication_time_ratio!")
                    return Constant.DEFAULT_INVALID_VALUE
                if communication_time_ratio < self.THRESHOLD_VALUE:
                    continue
                parallel_analysis_value = {}
                parallel_analysis_value.setdefault(id_name, id_value)
                parallel_analysis_value.setdefault("communication_time_ratio", communication_time_ratio)
                parallel_data.append(parallel_analysis_value)
        if parallel_data:
            parallel_data.sort(key=lambda x: x["communication_time_ratio"], reverse=True)
        return parallel_data

    def _parallel_analysis(self: any) -> None:
        self.parallel_analysis_result["type"] = Constant.DEFAULT_INVALID_VALUE
        self.parallel_analysis_result["value"] = []
        self.parallel_analysis_result["message"] = "Invalid parallel analysis data!"
        device_and_rank_ids = self._get_device_and_rank_ids()
        if device_and_rank_ids == Constant.DEFAULT_INVALID_VALUE:
            return
        if not device_and_rank_ids:
            error(MsProfCommonConstant.COMMON_FILE_NAME, "Cannot get the device or rank ids, "
                                                         "please check the table ClusterRank!")
            return
        parallel_data = self._get_parallel_data(device_and_rank_ids)
        if parallel_data == Constant.DEFAULT_INVALID_VALUE:
            return
        elif not parallel_data:
            self.parallel_analysis_result["type"] = 0
            self.parallel_analysis_result["message"] = self.TYPE_0_MESSAGE
            return
        self.parallel_analysis_result["value"] = parallel_data
        if len(parallel_data) == len(device_and_rank_ids):
            self.parallel_analysis_result["type"] = 1
            self.parallel_analysis_result["message"] = self.TYPE_1_MESSAGE
        else:
            self.parallel_analysis_result["type"] = 2
            self.parallel_analysis_result["message"] = self.TYPE_2_MESSAGE

    def _storage_parallel_analysis_result(self: any) -> None:
        output_file_name = "cluster_parallel_analysis.json"
        query_path = os.path.join(self.collection_path, PathManager.QUERY_CLUSTER)
        if not os.path.exists(query_path):
            try:
                os.makedirs(query_path)
            except OSError:
                error(MsProfCommonConstant.COMMON_FILE_NAME,
                      "Storing data failed, you may not have the permission to write files in the current path.")
            os.chmod(query_path, NumberConstant.DIR_AUTHORITY)
        output_file_path = PathManager.get_query_result_path(self.collection_path, output_file_name)
        try:
            with os.fdopen(os.open(output_file_path, Constant.WRITE_FLAGS,
                                   Constant.WRITE_MODES), 'w') as file:
                os.chmod(output_file_path, NumberConstant.FILE_AUTHORITY)
                json.dump(self.parallel_analysis_result, file, indent=4)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            error(MsProfCommonConstant.COMMON_FILE_NAME, err)
