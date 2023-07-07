#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
from abc import abstractmethod

import logging
from typing import List

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.platform.chip_manager import ChipManager
from mscalculate.cann.additional_record import AdditionalRecord
from mscalculate.cann.cann_database import ApiDataDatabase
from mscalculate.cann.event import Event
from msmodel.acl.acl_model import AclModel
from msmodel.ge.ge_host_parser_model import GeHostParserModel
from msmodel.ge.ge_info_model import GeModel
from msmodel.ge.ge_model_load_model import GeFusionModel
from msmodel.ge.ge_model_time_load import GeModelTimeModel
from msmodel.hccl.hccl_host_model import HCCLHostModel
from msmodel.runtime.runtime_api_model import RuntimeApiModel
from profiling_bean.db_dto.api_data_dto import ApiDataDto
from profiling_bean.db_dto.ctx_id_dto import CtxIdDto
from profiling_bean.db_dto.fusion_op_info_dto import FusionOpInfoDto
from profiling_bean.db_dto.ge_time_dto import GeTimeDto
from profiling_bean.db_dto.graph_id_map_dto import GraphIdMapDto
from profiling_bean.db_dto.hccl_info_dto import HCCLInfoDto
from profiling_bean.db_dto.node_basic_info_dto import NodeBasicInfoDto
from profiling_bean.db_dto.mem_copy_info_dto import MemCopyInfoDto
from profiling_bean.db_dto.task_track_dto import TaskTrackDto
from profiling_bean.db_dto.tensor_info_dto import TensorInfoDto


class CANNGear:
    """
    Gears, the basic components in the analysis chain of CANN software stacks,
    each instance processing data in one layer of CANN.
    1. Associate upper-layer and same-layer data.
    2. Generate the table structure.
    """
    INVALID_LEVEL = -1

    def __init__(self, project_path):
        self._project_path = project_path
        self.cann_level = self.INVALID_LEVEL

    @abstractmethod
    def run(self, event: Event, call_stack: dict):
        raise Exception("To be implement")

    @abstractmethod
    def flush_data(self):
        raise Exception("To be implement")


class RootGear(CANNGear):
    def __init__(self, project_path):
        super().__init__(project_path)
        self.cann_level = Constant.ROOT_LEVEL

    def run(self, event: Event, call_stack: dict):
        pass

    def flush_data(self):
        pass


class ACLGear(CANNGear):
    def __init__(self, project_path):
        super().__init__(project_path)
        self.cann_level = Constant.ACL_LEVEL
        self.acl_data = []
        self.process_id = InfoConfReader().get_json_pid_data()

    def run(self, event: Event, call_stack: dict):
        dto: ApiDataDto = ApiDataDatabase().get(event)

        self.acl_data.append([dto.id, dto.struct_type, dto.start, dto.end, self.process_id, dto.thread_id])

    def flush_data(self):
        if not self.acl_data:
            return
        model = AclModel({StrConstant.PARAM_RESULT_DIR: self._project_path})
        model.init()
        model.drop_table(DBNameConstant.TABLE_GE_MODEL_LOAD)
        model.create_table()
        model.insert_data_to_db(DBNameConstant.TABLE_ACL_DATA, self.acl_data)
        model.finalize()


class ModelGear(CANNGear):
    INVALID_MODEL_NAME = "NA"
    INVALID_REQUEST_ID = "NA"
    MODEL_TIME_STAGE = 3

    def __init__(self, project_path):
        super().__init__(project_path)
        self.cann_level = Constant.MODEL_LEVEL
        self.model_id_name_table = {}
        self.model_load_data = []
        self.model_time_data_table = {}
        self.model_time_data = []

    @classmethod
    def get_graph_id_map_dto(cls, event: Event) -> GraphIdMapDto:
        for record in event.additional_record:
            if isinstance(record.dto, GraphIdMapDto):
                return record.dto
        return GraphIdMapDto()

    def add_model_load(self, data: ApiDataDto, event: Event):
        graph_id_map_dto: GraphIdMapDto = self.get_graph_id_map_dto(event)
        model_name = graph_id_map_dto.model_name if graph_id_map_dto.struct_type else ""

        item = [data.item_id, model_name, data.start, data.end]
        self.model_load_data.append(item)

    def find_model_name(self, model_id) -> str:
        for mdl_id, model_name, *_ in self.model_load_data:
            if mdl_id == model_id:
                return model_name
        return ""

    def add_model_time_item_from_table(self, model_time: GeTimeDto) -> None:
        # In helper scene, there is no ModelExecute, so return
        if model_time.model_id == 0 and model_time.thread_id == 0:
            return

        if model_time.model_name is None:
            model_id = model_time.model_id
            model_time.model_name = self.find_model_name(model_id)

        self.model_time_data.append(
            [model_time.model_name, model_time.model_id, model_time.request_id, model_time.thread_id,
             model_time.input_start, model_time.input_end, model_time.infer_start, model_time.infer_end,
             model_time.output_start, model_time.output_end])
        self.model_time_data_table.pop((int(model_time.model_id), model_time.thread_id))

    def add_model_time(self, data: ApiDataDto, event):
        model_time = self.model_time_data_table.setdefault((int(data.item_id), data.thread_id), GeTimeDto())
        if model_time.model_id and data.struct_type == "ModelExecute":
            # dynamic scene do not have input and output stage
            self.add_model_time_item_from_table(model_time)
            model_time = self.model_time_data_table.setdefault((int(data.item_id), data.thread_id), GeTimeDto())
        if data.struct_type == "InputCopy":
            model_time.input_start = data.start
            model_time.input_end = data.end
            model_time.stage_num += 1
        elif data.struct_type == "ModelExecute":
            model_time.model_id = data.item_id
            model_time.thread_id = data.thread_id
            model_time.infer_start = data.start
            model_time.infer_end = data.end
            model_time.request_id = data.request_id
            graph_id_map_dto: GraphIdMapDto = self.get_graph_id_map_dto(event)
            model_name = graph_id_map_dto.model_name if graph_id_map_dto.struct_type else ""
            model_time.model_name = model_name
            model_time.stage_num += 1
        elif data.struct_type == "OutputCopy":
            model_time.output_start = data.start
            model_time.output_end = data.end
            model_time.stage_num += 1
        else:
            logging.error("Found illegal api: %s", data.struct_type)
            return

        if model_time.stage_num == self.MODEL_TIME_STAGE:
            self.add_model_time_item_from_table(model_time)

    def run(self, event: Event, call_stack: dict):
        dto: ApiDataDto = ApiDataDatabase().get(event)
        # these two table can be merged in the feature
        if dto.struct_type == "ModelLoad":
            self.add_model_load(dto, event)
        elif dto.struct_type:
            self.add_model_time(dto, event)
        else:
            # lose some data
            for record in event.additional_record:
                if isinstance(record, GraphIdMapDto):
                    logging.warning("Lose model info for graph_id: %s, model_name: %s",
                                    record.graph_id, record.model_name)

    def save_model_load(self: any) -> None:
        if not self.model_load_data:
            return
        model = GeFusionModel(self._project_path, [DBNameConstant.TABLE_GE_MODEL_LOAD])
        model.init()
        model.drop_table(DBNameConstant.TABLE_GE_MODEL_LOAD)
        model.create_table()
        model.flush_all({DBNameConstant.TABLE_GE_MODEL_LOAD: self.model_load_data})
        model.finalize()

    def save_model_time(self: any) -> None:
        if not self.model_time_data and not self.model_time_data_table:
            return
        if self.model_time_data_table:
            for model_time_data in list(self.model_time_data_table.values()):
                self.add_model_time_item_from_table(model_time_data)

        model = GeModelTimeModel(self._project_path, [DBNameConstant.TABLE_GE_MODEL_TIME])
        model.init()
        model.drop_table(DBNameConstant.TABLE_GE_MODEL_TIME)
        model.create_table()
        model.flush(self.model_time_data)
        model.finalize()

    def flush_data(self):
        self.save_model_load()
        self.save_model_time()


class NodeGear(CANNGear):
    GE_STEP_INFO_API_TYPE = "step_info"
    GE_HCCL_OP_TYPE = "HCCL"

    def __init__(self, project_path):
        super().__init__(project_path)
        self.cann_level = Constant.NODE_LEVEL
        self.node_name_type_table = {}
        self.ge_host_info = []
        self.step_info = []
        self.hccl_op_info = []
        self.fusion_op_info = []

    @staticmethod
    def get_op_type_by_addition(additional_records: List[AdditionalRecord]):
        if not additional_records:
            return ""

        # static shape or dynamic shape
        for record in additional_records:
            if isinstance(record.dto, NodeBasicInfoDto):
                return record.dto.op_type
        return ""

    def update_hccl_op_name(self: any) -> None:
        op_name_index = 3  # 3 is op_name index
        hccl_op_counter = 0
        for hccl_op in self.hccl_op_info:
            hccl_op[op_name_index] = hccl_op[op_name_index] + "_" + str(hccl_op_counter)
            hccl_op_counter += 1

    def record_fusion_op_info(self, dto: FusionOpInfoDto, call_stack: dict):
        model_event: Event = call_stack.get(Constant.MODEL_LEVEL)
        model_dto: ApiDataDto = ApiDataDatabase().get(model_event)
        self.fusion_op_info.append([model_dto.item_id, dto.op_name, dto.fusion_op_num, dto.fusion_op_names,
                                    dto.memory_input, dto.memory_output, dto.memory_weight, dto.memory_workspace,
                                    dto.memory_total])

    def run(self, event: Event, call_stack: dict):
        dto: ApiDataDto = ApiDataDatabase().get(event)

        if not dto.struct_type:
            for record in event.additional_record:
                if isinstance(record.dto, FusionOpInfoDto):
                    self.record_fusion_op_info(record.dto, call_stack)
            return

        if dto.struct_type == self.GE_STEP_INFO_API_TYPE:
            # add step_info
            model_event: Event = call_stack.get(Constant.MODEL_LEVEL)
            model_dto: ApiDataDto = ApiDataDatabase().get(model_event)
            # tag 0 represents iter begin, tag 1 represents iter end
            timestamp = dto.end if int(dto.item_id) == 1 else dto.start
            # notice: different from old struct, delete stream_id task_id batch_id in step info
            request_id = model_dto.request_id if model_dto.request_id is not None else -1

            item_id = 1 if int(dto.item_id) == 1 else 0
            self.step_info.append([model_dto.item_id, dto.thread_id, timestamp, request_id, item_id])
        else:
            # add ge host info
            op_type = self.get_op_type_by_addition(event.additional_record)
            if op_type:
                # record
                self.node_name_type_table[dto.item_id] = op_type
            self.ge_host_info.append([dto.thread_id, op_type, dto.struct_type,
                                      dto.start, dto.end, dto.item_id])
            for record in event.additional_record:
                if isinstance(record.dto, NodeBasicInfoDto) and record.dto.task_type == self.GE_HCCL_OP_TYPE:
                    model_event: Event = call_stack.get(Constant.MODEL_LEVEL)
                    model_dto: ApiDataDto = ApiDataDatabase().get(model_event)
                    request_id = model_dto.request_id if model_dto.request_id is not None else -1
                    model_id = NumberConstant.INVALID_MODEL_ID if model_dto.item_id is None else model_dto.item_id
                    self.hccl_op_info.append([model_id, request_id, dto.thread_id, dto.item_id,
                                              record.dto.task_type, record.dto.op_type, dto.start, dto.end,
                                              record.dto.is_dynamic])

    def update_node_name(self):
        op_type_index = 1
        op_name_index = -1
        for ge_host_data in self.ge_host_info:
            ge_host_data[op_type_index] = self.node_name_type_table.get(ge_host_data[op_name_index], "")

    def save_ge_host_info(self):
        if not self.ge_host_info:
            return
        self.update_node_name()
        model = GeHostParserModel(self._project_path, DBNameConstant.DB_GE_HOST_INFO,
                                  [DBNameConstant.TABLE_GE_HOST])
        model.init()
        model.drop_table(DBNameConstant.TABLE_GE_HOST)
        model.create_table()

        model.insert_data_to_db(DBNameConstant.TABLE_GE_HOST, self.ge_host_info)
        model.finalize()

    def save_step_info(self):
        if not self.ge_host_info:
            return
        model = GeModel(self._project_path, [DBNameConstant.TABLE_GE_STEP])
        model.init()
        model.drop_table(DBNameConstant.TABLE_GE_STEP)
        model.create_table()
        model.insert_data_to_db(DBNameConstant.TABLE_GE_STEP, self.step_info)
        model.finalize()

    def save_hccl_op_info(self):
        if not self.hccl_op_info:
            return
        self.update_hccl_op_name()
        model = HCCLHostModel(self._project_path)
        model.init()
        model.drop_table(DBNameConstant.TABLE_HCCL_OP)
        model.create_table()

        model.insert_data_to_db(DBNameConstant.TABLE_HCCL_OP, self.hccl_op_info)
        model.finalize()

    def save_fusion_op_info(self):
        if not self.fusion_op_info:
            return
        model = GeFusionModel(self._project_path, [DBNameConstant.TABLE_GE_FUSION_OP_INFO])
        model.init()
        model.drop_table(DBNameConstant.TABLE_GE_FUSION_OP_INFO)
        model.create_table()

        model.insert_data_to_db(DBNameConstant.TABLE_GE_FUSION_OP_INFO, self.fusion_op_info)
        model.finalize()

    def flush_data(self):
        self.save_ge_host_info()
        self.save_step_info()
        self.save_hccl_op_info()
        self.save_fusion_op_info()


class TaskGear(CANNGear):
    INVALID_STREAM = 65535  # 翻转最大值65535做无效值
    INVALID_ID = -1
    INVALID_DIRECT = -1
    INVALID_CONTEXT_ID = 4294967295
    INVALID_MODEL_ID = 4294967295
    KERNEL_TASK_PREFIX = "KERNEL"
    AICORE_TASK_TYPE = "AI_CORE"
    HCCL_TASK_TYPE = "HCCL"
    FFTS_PLUS_TASK_TYPE = "FFTS_PLUS"
    KERNEL_FFTS_PLUS_TASK_TYPE = "FFTS_PLUS"

    class RuntimeApi:
        def __init__(self, start, end, struct_type, thread_id):
            self.start = start
            self.end = end
            self.api = struct_type
            self.thread = thread_id
            self.stream_id = TaskGear.INVALID_STREAM
            self.task_id = TaskGear.INVALID_ID
            self.batch_id = TaskGear.INVALID_ID
            self.data_size = 0
            self.memcpy_direction = TaskGear.INVALID_DIRECT

        def to_list(self):
            return [self.start, self.end, self.api, self.thread, self.stream_id,
                    self.task_id, self.batch_id, self.data_size, self.memcpy_direction]

    class NodeDesc:
        def __init__(self, node_basic_info=NodeBasicInfoDto(), tensor_info=TensorInfoDto(), ctx_info=CtxIdDto()):
            self.node_basic_info = node_basic_info
            self.tensor_info = tensor_info
            self.ctx_info = ctx_info

        @staticmethod
        def get_hash(dto: any):
            return dto.op_name + "-" + str(int(dto.timestamp))

        def is_invalid(self):
            return self.node_basic_info is not None and self.tensor_info is not None

    def __init__(self, project_path):
        super().__init__(project_path)
        self.cann_level = Constant.TASK_LEVEL
        self.api_call_info = []
        self.task_info = []
        self.tensor_info = []
        self.hccl_task_info = []
        self.mismatch_hccl = 0

    @staticmethod
    def get_task_level_additional_dto(event: Event) -> tuple:
        mem_cpy_info_dto = MemCopyInfoDto()
        task_track_dto = TaskTrackDto()
        for record in event.additional_record:
            if isinstance(record.dto, MemCopyInfoDto):
                mem_cpy_info_dto = record.dto
            elif isinstance(record.dto, TaskTrackDto):
                task_track_dto = record.dto
        return mem_cpy_info_dto, task_track_dto

    def get_hccl_info_dto(self, event: Event) -> HCCLInfoDto:
        for record in event.additional_record:
            if isinstance(record.dto, HCCLInfoDto):
                return record.dto
        self.mismatch_hccl += 1
        return HCCLInfoDto()

    def get_node_descs(self, event: Event) -> dict:
        node_descs = dict()
        for record in event.additional_record:
            if isinstance(record.dto, NodeBasicInfoDto):
                node_desc = node_descs.setdefault(self.NodeDesc.get_hash(record.dto), self.NodeDesc())
                node_desc.node_basic_info = record.dto
            elif isinstance(record.dto, TensorInfoDto):
                node_desc = node_descs.setdefault(self.NodeDesc.get_hash(record.dto), self.NodeDesc())
                node_desc.tensor_info = record.dto
            elif isinstance(record.dto, CtxIdDto):
                node_desc = node_descs.setdefault(self.NodeDesc.get_hash(record.dto), self.NodeDesc())
                node_desc.ctx_info = record.dto
        return node_descs

    def is_hccl_task(self, hccl_event: Event):
        return not hccl_event.is_invalid()

    def add_hccl_task(self, hccl_event: Event, task_track_dto: TaskTrackDto):
        hccl_dto: ApiDataDto = ApiDataDatabase().get(hccl_event)
        hccl_info_dto = self.get_hccl_info_dto(hccl_event)
        self.hccl_task_info.append(
            [hccl_dto.item_id, hccl_info_dto.plane_id, hccl_info_dto.timestamp,
             hccl_info_dto.duration_estimated, task_track_dto.stream_id, task_track_dto.task_id,
             task_track_dto.batch_id, hccl_info_dto.to_args_json(task_track_dto.stream_id, task_track_dto.task_id)])

    def is_kernel_task(self, task_track_dto: TaskTrackDto):
        if task_track_dto.struct_type is None:
            return False
        if task_track_dto.task_type.startswith(self.KERNEL_TASK_PREFIX) or \
                task_track_dto.task_type == self.KERNEL_FFTS_PLUS_TASK_TYPE:
            return True
        return False

    def add_kernel_task(self, call_stack: dict, add_dto: TaskTrackDto):
        model_event: Event = call_stack.get(Constant.MODEL_LEVEL)
        model_dto: ApiDataDto = ApiDataDatabase().get(model_event)
        node_event: Event = call_stack.get(Constant.NODE_LEVEL)

        node_descs = self.get_node_descs(node_event)
        if not node_descs:
            # this happen when runtime task is not respond to a op
            return

        model_id = model_dto.item_id if model_dto.item_id is not None else self.INVALID_MODEL_ID
        request_id = model_dto.request_id if model_dto.request_id is not None else -1
        for node_desc in node_descs.values():
            node_basic_info_dto: NodeBasicInfoDto = node_desc.node_basic_info
            tensor_info_dto: TensorInfoDto = node_desc.tensor_info
            ctx_id_dto = node_desc.ctx_info

            if node_basic_info_dto.task_type is None or node_basic_info_dto.task_type == self.FFTS_PLUS_TASK_TYPE:
                continue
            task_type = node_basic_info_dto.task_type
            if node_basic_info_dto.task_type == self.HCCL_TASK_TYPE:
                # notice: reduce TBE op
                task_type = self.AICORE_TASK_TYPE

            cxt_ids = str(ctx_id_dto.ctx_id).split(',')
            for cxt_id in cxt_ids:
                self.task_info.append([model_id, node_basic_info_dto.op_name, add_dto.stream_id, add_dto.task_id,
                                       node_basic_info_dto.block_dim, node_basic_info_dto.mix_block_dim,
                                       node_basic_info_dto.is_dynamic, task_type,
                                       node_basic_info_dto.op_type, request_id, add_dto.thread_id,
                                       node_basic_info_dto.timestamp, add_dto.batch_id, int(cxt_id)])

            if tensor_info_dto.struct_type is None:
                continue
            for cxt_id in cxt_ids:
                self.tensor_info.append([model_id, add_dto.stream_id, add_dto.task_id, tensor_info_dto.tensor_num,
                                         tensor_info_dto.input_formats, tensor_info_dto.input_data_types,
                                         tensor_info_dto.input_shapes, tensor_info_dto.output_formats,
                                         tensor_info_dto.output_data_types, tensor_info_dto.output_shapes,
                                         request_id, tensor_info_dto.timestamp, add_dto.batch_id, int(cxt_id)])

    def run(self, event: Event, call_stack: dict):
        dto: ApiDataDto = ApiDataDatabase().get(event)

        # pure runtime api
        if not event.is_invalid() and not event.additional_record:
            self.api_call_info.append([dto.start, dto.end, dto.struct_type, dto.thread_id, self.INVALID_STREAM,
                                       self.INVALID_ID, self.INVALID_ID, 0, self.INVALID_DIRECT])
            return

        mem_cpy_dto, task_track_dto = self.get_task_level_additional_dto(event)
        if not event.is_invalid():
            api = self.RuntimeApi(dto.start, dto.end, dto.struct_type, dto.thread_id)

            if mem_cpy_dto.struct_type is not None:
                api.data_size = mem_cpy_dto.data_size
                api.memcpy_direction = mem_cpy_dto.memcpy_direction
            if task_track_dto.struct_type is not None:
                api.stream_id = task_track_dto.stream_id
                api.task_id = task_track_dto.task_id
                api.batch_id = task_track_dto.batch_id
            self.api_call_info.append(api.to_list())

        hccl_event = call_stack.get(Constant.HCCL_LEVEL)
        if self.is_hccl_task(hccl_event):
            self.add_hccl_task(hccl_event, task_track_dto)
        if self.is_kernel_task(task_track_dto):
            self.add_kernel_task(call_stack, task_track_dto)

    def save_api_call_info(self):
        if not self.api_call_info:
            return

        model = RuntimeApiModel(self._project_path, DBNameConstant.DB_RUNTIME, [DBNameConstant.TABLE_API_CALL])
        model.init()
        model.drop_table(DBNameConstant.TABLE_API_CALL)
        model.create_table()
        model.insert_data_to_db(DBNameConstant.TABLE_API_CALL, self.api_call_info)
        model.finalize()

    def save_task_info(self):
        if not self.task_info:
            return

        model = GeModel(self._project_path, [DBNameConstant.TABLE_GE_TASK])
        model.init()
        model.drop_table(DBNameConstant.TABLE_GE_TASK)
        model.create_table()
        model.insert_data_to_db(DBNameConstant.TABLE_GE_TASK, self.task_info)
        model.finalize()

    def save_tensor_info(self):
        if not self.tensor_info:
            return

        model = GeModel(self._project_path, [DBNameConstant.TABLE_GE_TENSOR])
        model.init()
        model.drop_table(DBNameConstant.TABLE_GE_TENSOR)
        model.create_table()
        model.insert_data_to_db(DBNameConstant.TABLE_GE_TENSOR, self.tensor_info)
        model.finalize()

    def save_hccl_task_info(self):
        if not self.hccl_task_info:
            return
        if self.mismatch_hccl > 0:
            logging.warning("There is %d hccl info lost", self.mismatch_hccl)
        model = HCCLHostModel(self._project_path)
        model.init()
        model.drop_table(DBNameConstant.TABLE_HCCL_TASK)
        model.create_table()
        model.insert_data_to_db(DBNameConstant.TABLE_HCCL_TASK, self.hccl_task_info)
        model.finalize()

    def flush_data(self):
        self.save_api_call_info()
        self.save_task_info()
        self.save_tensor_info()
        self.save_hccl_task_info()


class HCCLGear(CANNGear):
    """
    hccl op contains several threads, link will represent in hccl_calculator.py
    """

    def __init__(self, project_path):
        super().__init__(project_path)
        self.cann_level = Constant.HCCL_LEVEL

    def run(self, event: Event, call_stack: dict):
        pass

    def flush_data(self):
        pass
