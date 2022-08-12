#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import json

from common_func.common import print_msg
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_common import create_json
from common_func.path_manager import PathManager
from msmodel.cluster_info.cluster_communication_model import ClusterCommunicationModel
from msmodel.cluster_info.cluster_info_model import ClusterInfoViewModel


class ClusterCommunicationParser:
    """
    collective communication data parser
    """
    HEADERS = ['Rank ID', 'Compute Time', 'Communication Time', 'Stage Time']
    CLUSTER_ALL_DEVICE_SCENE = -1

    def __init__(self: any, params: dict) -> None:
        self._collection_path = params["collection_path"]
        self._is_cluster_scene = params["is_cluster"]
        self._npu_id = params["npu_id"]
        self._model_id = params["model_id"]
        self._iteration_id = params["iteration_id"]
        self._data_collection = []
        self._communication_model = ClusterCommunicationModel(params)
        self._cluster_info_model = ClusterInfoViewModel(params)

    def process(self: any) -> None:
        self._get_communication_data()
        self._storage_summary_data()

    def _get_communication_data(self: any) -> None:
        """
        communication contains: rank id, compute_time, communication_time, stage_time
        """
        with self._communication_model as _model:
            if not _model.check_db():
                return
            if not self._is_cluster_all_device_scene():
                return

            with self._cluster_info_model as _c_model:
                if _c_model.check_db() and _c_model.check_table():
                    rank_ids = _c_model.get_all_rank_id()
            for rank_id in rank_ids:
                communication_data = _model.get_cluster_communication(rank_id)
                if not communication_data:
                    continue
                self._data_collection.extend(communication_data)

    def _storage_summary_data(self: any) -> None:
        if not self._data_collection:
            return

        communication_data = []
        for data in self._data_collection:
            communication_data.append([data.rank_id, data.compute_time, data.communication_time, data.stage_time])
        output_file_name = "collective_communication_{}_{}_{}.json".format(self._npu_id, self._model_id,
                                                                           self._iteration_id)
        output_file_path = PathManager.get_query_result_path(self._collection_path, output_file_name)
        result = create_json(output_file_path, self.HEADERS, communication_data, save_old_file=False)
        result_json = json.loads(result)
        if result_json["status"] == NumberConstant.SUCCESS:
            print_msg(result)
        else:
            print_msg(json.dumps(
                {'status': NumberConstant.ERROR, 'info': 'query collective communication data failed', 'data': ''}))

    def _is_cluster_all_device_scene(self: any) -> bool:
        return self._npu_id == self.CLUSTER_ALL_DEVICE_SCENE
