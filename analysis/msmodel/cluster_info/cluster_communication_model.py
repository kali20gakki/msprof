#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.collective_communication_dto import CollectiveCommunicationDto


class ClusterCommunicationModel(ViewModel):
    """
    operation of collective communication.
    """

    def __init__(self, params):
        self._collection_path = params["collection_path"]
        self._model_id = params["model_id"]
        self._iteration_id = params["iteration_id"]
        super().__init__(self._collection_path, DBNameConstant.DB_CLUSTER_STEP_TRACE, [])

    def get_cluster_communication(self, rank_id):
        sql = "select {rank_id} as rank_id, t0.fp_bp_time - t0.fp_bp_communication_time as compute_time," \
              "t0.communication_time, " \
              "t0.iteration_time - t0.communication_time as stage_time " \
              "from (select " \
              "(case when tt.fp_bp_time = 0 then tt.iteration_time else tt.fp_bp_time end) as fp_bp_time, " \
              "tt.iteration_time, sum(t1.end - t1.start) as communication_time," \
              "sum(case when fp_bp_time>0 and t1.start>tt.bp_end then 0 else t1.end - t1.start end) " \
              "as fp_bp_communication_time " \
              "from {1} t1 inner join {0} tt " \
              "on t1.model_id = tt.model_id and t1.index_id = tt.iteration_id " \
              "and t1.model_id = {2} and t1.index_id = {3} " \
              "group by t1.model_id, t1.index_id) t0".format(DBNameConstant.TABLE_CLUSTER_STEP_TRACE.format(rank_id),
                                                             DBNameConstant.TABLE_CLUSTER_ALL_REDUCE.format(rank_id),
                                                             self._model_id,
                                                             self._iteration_id,
                                                             rank_id=rank_id)
        return DBManager.fetch_all_data(self.cur, sql, dto_class=CollectiveCommunicationDto)
