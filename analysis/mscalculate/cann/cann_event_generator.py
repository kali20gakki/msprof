#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

from common_func.db_name_constant import DBNameConstant
from mscalculate.cann.additional_record import AdditionalRecord
from mscalculate.cann.cann_database import AdditionalRecordDatabase
from mscalculate.cann.cann_database import ApiDataDatabase
from mscalculate.cann.event_queue import EventQueue
from msmodel.add_info.ctx_id_model import CtxIdModel
from msmodel.add_info.fusion_add_info_model import FusionAddInfoModel
from msmodel.add_info.graph_add_info_model import GraphAddInfoModel
from msmodel.add_info.hccl_info_model import HcclInfoModel
from msmodel.compact_info.node_basic_info_model import NodeBasicInfoModel
from msmodel.compact_info.memcpy_info_model import MemcpyInfoModel
from msmodel.compact_info.task_track_model import TaskTrackModel
from msmodel.add_info.tensor_add_info_model import TensorAddInfoModel
from msmodel.api.api_data_model import ApiDataModel
from msmodel.event.event_data_model import EventDataModel
from profiling_bean.db_dto.api_data_dto import ApiDataDto
from profiling_bean.db_dto.ctx_id_dto import CtxIdDto
from profiling_bean.db_dto.event_data_dto import EventDataDto
from profiling_bean.db_dto.fusion_op_info_dto import FusionOpInfoDto
from profiling_bean.db_dto.graph_id_map_dto import GraphIdMapDto
from profiling_bean.db_dto.hccl_info_dto import HCCLInfoDto
from profiling_bean.db_dto.mem_copy_info_dto import MemCopyInfoDto
from profiling_bean.db_dto.node_basic_info_dto import NodeBasicInfoDto
from profiling_bean.db_dto.task_track_dto import TaskTrackDto
from profiling_bean.db_dto.tensor_info_dto import TensorInfoDto


class CANNEventGenerator:
    """
    generate event for cann analysis
    """

    def __init__(self, project_path, thread_set: set):
        self._project_path = project_path
        self.event_queue = EventQueue()
        self.thread_set = thread_set

        self.api_data_model = ApiDataModel(self._project_path)
        self.event_data_model = EventDataModel(self._project_path)
        self.node_basic_info_model = NodeBasicInfoModel(self._project_path)
        self.tensor_info_model = TensorAddInfoModel(self._project_path)
        self.task_track_model = TaskTrackModel(self._project_path)
        self.mem_copy_model = MemcpyInfoModel(self._project_path)
        self.ctx_id_model = CtxIdModel(self._project_path)
        self.hccl_info_model = HcclInfoModel(self._project_path)
        self.graph_id_map_model = GraphAddInfoModel(self._project_path)
        self.fusion_op_info_model = FusionAddInfoModel(self._project_path)

    def record_additional_info(self, infos):
        for info in infos:
            record = AdditionalRecord(info, info.timestamp, info.struct_type)
            event = AdditionalRecordDatabase().put(record)
            self.event_queue.add(event)
            self.thread_set.add(event.thread_id)

    def collect_time_period_data(self):
        api_data, event_data = [], []
        if not self.api_data_model.check_db():
            return api_data, event_data
        with self.api_data_model as api_model:
            api_data = api_model.get_all_data(DBNameConstant.TABLE_API_DATA, ApiDataDto)

        if not self.event_data_model.check_db():
            return api_data, event_data
        with self.event_data_model as event_model:
            event_data = event_model.get_all_data(DBNameConstant.TABLE_EVENT_DATA, EventDataDto)
        return api_data, event_data

    def generate_api_event(self):
        api_data, event_data = self.collect_time_period_data()
        for api_data_dto in api_data:
            # special api start equal end, abandon
            if api_data_dto.struct_type == "StreamSyncTaskFinish" \
                    or api_data_dto.start <= 0 or api_data_dto.start == api_data_dto.end:
                continue
            event = ApiDataDatabase().put(api_data_dto)
            self.event_queue.add(event)
            self.thread_set.add(event.thread_id)

        event_logger = dict()
        for event_data_dto in event_data:
            # 先匹配，再生成event
            friend_timestamp = event_logger.get(
                (event_data_dto.level, event_data_dto.thread_id, event_data_dto.struct_type, event_data_dto.request_id,
                 event_data_dto.item_id),
                None
            )
            if not friend_timestamp:
                event_logger[
                    (event_data_dto.level, event_data_dto.thread_id, event_data_dto.struct_type,
                     event_data_dto.request_id, event_data_dto.item_id)
                ] = event_data_dto.timestamp
                continue
            else:
                # represent 2 event with an api
                equal_api = ApiDataDto(friend_timestamp, event_data_dto)
                event = ApiDataDatabase().put(equal_api)
                self.event_queue.add(event)
                self.thread_set.add(event.thread_id)
                # for sub graph scene
                event_logger.pop((event_data_dto.level, event_data_dto.thread_id, event_data_dto.struct_type,
                                  event_data_dto.request_id, event_data_dto.item_id))

    def generate_node_basic_info_event(self):
        if not self.node_basic_info_model.check_db():
            return
        with self.node_basic_info_model as node_basic_model:
            node_basic_infos = node_basic_model.get_all_data(DBNameConstant.TABLE_NODE_BASIC_INFO, NodeBasicInfoDto)
            self.record_additional_info(node_basic_infos)

    def generate_tensor_info_event(self):
        if not self.tensor_info_model.check_db():
            return
        with self.tensor_info_model as tensor_info_model:
            tensor_infos = tensor_info_model.get_all_data(DBNameConstant.TABLE_TENSOR_ADD_INFO, TensorInfoDto)
            self.record_additional_info(tensor_infos)

    def generate_task_track_event(self):
        if not self.task_track_model.check_db():
            return
        with self.task_track_model as task_track_model:
            task_tracks = task_track_model.get_all_data(DBNameConstant.TABLE_TASK_TRACK, TaskTrackDto)
            self.record_additional_info(task_tracks)

    def generate_mem_copy_info_event(self):
        if not self.mem_copy_model.check_db():
            return
        with self.mem_copy_model as mem_copy_model:
            mem_copy_infos = mem_copy_model.get_all_data(DBNameConstant.TABLE_MEMCPY_INFO, MemCopyInfoDto)
            self.record_additional_info(mem_copy_infos)

    def generate_ctx_id_event(self):
        if not self.ctx_id_model.check_db():
            return
        with self.ctx_id_model as ctx_id_model:
            ctx_ids = ctx_id_model.get_all_data(DBNameConstant.TABLE_CTX_ID_ADD_INFO, CtxIdDto)
            self.record_additional_info(ctx_ids)

    def generate_hccl_info_event(self):
        if not self.hccl_info_model.check_db():
            return
        with self.hccl_info_model as hccl_info_model:
            hccl_infos = hccl_info_model.get_all_data(DBNameConstant.TABLE_HCCL_INFO_DATA, HCCLInfoDto)
            self.record_additional_info(hccl_infos)

    def generate_graph_id_map_event(self):
        if not self.graph_id_map_model.check_db():
            return
        with self.graph_id_map_model as graph_id_model:
            graph_id_map_infos = graph_id_model.get_all_data(DBNameConstant.TABLE_GRAPH_ADD_INFO, GraphIdMapDto)
            self.record_additional_info(graph_id_map_infos)

    def generate_fusion_op_info_event(self):
        if not self.fusion_op_info_model.check_db():
            return
        with self.fusion_op_info_model as fusion_op_model:
            fusion_op_infos = fusion_op_model.get_all_data(DBNameConstant.TABLE_FUSION_ADD_INFO, FusionOpInfoDto)
            self.record_additional_info(fusion_op_infos)

    def run(self) -> EventQueue:
        self.generate_api_event()

        self.generate_node_basic_info_event()
        self.generate_tensor_info_event()
        self.generate_task_track_event()
        self.generate_mem_copy_info_event()
        self.generate_graph_id_map_event()
        self.generate_fusion_op_info_event()
        self.generate_ctx_id_event()
        self.generate_hccl_info_event()

        return self.event_queue
