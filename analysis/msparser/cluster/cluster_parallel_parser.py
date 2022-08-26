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
    TYPE_0_MESSAGE = "The communication time ratio of all NPU devices is good and below the threshold of 10%."
    TYPE_1_MESSAGE = "The communication time ratio of all NPU devices exceeds the threshold of 10%. " \
                     "You can try to set the all_reduce_fusion_config parameter and adjust the gradient " \
                     "allreduce fusion by referring to the MindSpore distributed parallel tutorial."
    TYPE_2_MESSAGE = "The communication time ratio of some NPU devices exceeds the threshold of 10%." \
                     "You need to check whether there are slow nodes or slow links, then you can try to set " \
                     "the all_reduce_fusion_config parameter and adjust the gradient allreduce fusion " \
                     "by referring to the MindSpore distributed parallel tutorial."

    def __init__(self, params: dict) -> None:
        self.collection_path = params["collection_path"]
        self._cluster_info_model = ClusterInfoViewModel(params)
        self._cluster_communication_model = ClusterCommunicationModel(params)
        self.parallel_analysis_result = {}

    def process(self: any) -> None:
        self._parallel_analysis()
        self._storage_parallel_analysis_result()

    def _get_rank_ids(self: any) -> any:
        if not os.path.exists(PathManager.get_db_path(self.collection_path, DBNameConstant.DB_CLUSTER_RANK)):
            error(MsProfCommonConstant.COMMON_FILE_NAME, "Cannot find the cluster_rank.db!")
            return Constant.DEFAULT_INVALID_VALUE
        if not os.path.exists(PathManager.get_db_path(self.collection_path, DBNameConstant.DB_CLUSTER_STEP_TRACE)):
            error(MsProfCommonConstant.COMMON_FILE_NAME, "Cannot find the cluster_step_trace.db!")
            return Constant.DEFAULT_INVALID_VALUE
        with self._cluster_info_model as _model:
            return _model.get_all_rank_id()

    def _get_parallel_data(self: any, rank_ids: set) -> any:
        parallel_data = []
        with self._cluster_communication_model as _model:
            for rank_id in rank_ids:
                communication_time_ratio = _model.get_communication_time_ratio(rank_id)
                if communication_time_ratio == Constant.DEFAULT_INVALID_VALUE or communication_time_ratio is None:
                    error(MsProfCommonConstant.COMMON_FILE_NAME, "Invalid communication_time_ratio!")
                    return Constant.DEFAULT_INVALID_VALUE
                if communication_time_ratio < 0.1:
                    continue
                rank_communication_ratio = {}
                rank_communication_ratio.setdefault("rank_id", rank_id)
                rank_communication_ratio.setdefault("communication_time_ratio", communication_time_ratio)
                parallel_data.append(rank_communication_ratio)
        if parallel_data:
            parallel_data.sort(key=lambda x: x["communication_time_ratio"], reverse=True)
        return parallel_data

    def _parallel_analysis(self: any, ):
        self.parallel_analysis_result["type"] = Constant.DEFAULT_INVALID_VALUE
        self.parallel_analysis_result["value"] = []
        self.parallel_analysis_result["message"] = "Invalid parallel analysis data!"
        rank_ids = self._get_rank_ids()
        if rank_ids == Constant.DEFAULT_INVALID_VALUE:
            return
        if not rank_ids:
            error(MsProfCommonConstant.COMMON_FILE_NAME, "Cannot get the rank_ids, please check the table ClusterRank!")
            return
        parallel_data = self._get_parallel_data(rank_ids)
        if parallel_data == Constant.DEFAULT_INVALID_VALUE:
            return
        elif not parallel_data:
            self.parallel_analysis_result["type"] = 0
            self.parallel_analysis_result["message"] = self.TYPE_0_MESSAGE
            return
        self.parallel_analysis_result["value"] = parallel_data
        if len(parallel_data) == len(rank_ids):
            self.parallel_analysis_result["type"] = 1
            self.parallel_analysis_result["message"] = self.TYPE_1_MESSAGE
        else:
            self.parallel_analysis_result["type"] = 2
            self.parallel_analysis_result["message"] = self.TYPE_2_MESSAGE
        return

    def _storage_parallel_analysis_result(self: any) -> None:
        output_file_name = "cluster_parallel_analysis.json"
        query_path = os.path.join(self.collection_path, PathManager.QUERY_CLUSTER)
        if not os.path.exists(query_path):
            try:
                os.makedirs(query_path)
            except OSError:
                error(MsProfCommonConstant.COMMON_FILE_NAME,
                      "Storing data failed, you may not have the permission to write files in the current path.")
        output_file_path = PathManager.get_query_result_path(self.collection_path, output_file_name)
        with os.fdopen(os.open(output_file_path, Constant.WRITE_FLAGS,
                               Constant.WRITE_MODES), 'w') as file:
            os.chmod(output_file_path, NumberConstant.FILE_AUTHORITY)
            json.dump(self.parallel_analysis_result, file, indent=4)