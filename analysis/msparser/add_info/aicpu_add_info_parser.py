#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import logging

from common_func.db_name_constant import DBNameConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.ms_constant.str_constant import StrConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.hash_dict_constant import HashDictData
from common_func.hccl_info_common import trans_enum_name
from common_func.hccl_info_common import RoleType
from common_func.hccl_info_common import OpType
from common_func.hccl_info_common import DataType
from common_func.hccl_info_common import LinkType
from common_func.hccl_info_common import TransPortType
from common_func.hccl_info_common import RdmaType
from common_func.hccl_info_common import DeviceHcclOpSource
from msmodel.ai_cpu.ai_cpu_model import AiCpuModel
from msmodel.step_trace.ts_track_model import TsTrackModel
from msmodel.ai_cpu.data_preparation_model import DataPreparationModel
from msmodel.add_info.kfc_info_model import KfcInfoModel
from msparser.add_info.aicpu_add_info_bean import AicpuAddInfoBean
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from profiling_bean.prof_enum.data_tag import DataTag


class AicpuAddInfoParser(DataParser, MsMultiProcess):
    """
    aicpu data parser
    """
    NONE_NODE_NAME = ''

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)
        self._file_list = file_list
        self.project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self.hash_data = {}
        self._aicpu_data = {
            AicpuAddInfoBean.AICPU_NODE: [],
            AicpuAddInfoBean.AICPU_DP: [],
            AicpuAddInfoBean.AICPU_MODEL: [],
            AicpuAddInfoBean.AICPU_MI: [],
            AicpuAddInfoBean.KFC_COMM_TURN: [],
            AicpuAddInfoBean.KFC_COMPUTE_TURN: [],
            AicpuAddInfoBean.KFC_HCCL_INFO: [],
            AicpuAddInfoBean.HCCL_OP_INFO: [],
        }
        self._get_data_func = {
            AicpuAddInfoBean.AICPU_NODE: self.get_aicpu_node_data,
            AicpuAddInfoBean.AICPU_DP: self.get_aicpu_dp_data,
            AicpuAddInfoBean.AICPU_MODEL: self.get_aicpu_model_data,
            AicpuAddInfoBean.AICPU_MI: self.get_aicpu_mi_data,
            AicpuAddInfoBean.KFC_COMM_TURN: self.get_kfc_comm_turn_data,
            AicpuAddInfoBean.KFC_COMPUTE_TURN: self.get_kfc_compute_turn_data,
            AicpuAddInfoBean.KFC_HCCL_INFO: self.get_kfc_hccl_info_data,
            AicpuAddInfoBean.HCCL_OP_INFO: self.get_hccl_op_info_data,
        }

    @staticmethod
    def get_aicpu_node_data(aicpu_info: AicpuAddInfoBean) -> list:
        return [
            aicpu_info.data.stream_id,
            aicpu_info.data.task_id,
            aicpu_info.data.ai_cpu_task_start_time,
            aicpu_info.data.ai_cpu_task_end_time,
            AicpuAddInfoParser.NONE_NODE_NAME,
            aicpu_info.data.compute_time,
            aicpu_info.data.memory_copy_time,
            aicpu_info.data.ai_cpu_task_time,
            aicpu_info.data.dispatch_time,
            aicpu_info.data.total_time
        ]

    @staticmethod
    def get_aicpu_dp_data(aicpu_info: AicpuAddInfoBean) -> list:
        return [
            InfoConfReader().trans_into_local_time(float(aicpu_info.timestamp)),
            aicpu_info.data.action,
            aicpu_info.data.source,
            aicpu_info.data.buffer_size,
        ]

    @staticmethod
    def get_aicpu_model_data(aicpu_info: AicpuAddInfoBean) -> list:
        return [
            aicpu_info.data.index_id,
            aicpu_info.data.model_id,
            aicpu_info.timestamp,
            aicpu_info.data.tag_id,
            aicpu_info.data.event_id,
        ]

    @staticmethod
    def get_aicpu_mi_data(aicpu_info: AicpuAddInfoBean) -> list:
        return [
            aicpu_info.data.node_name,
            aicpu_info.data.queue_size,
            aicpu_info.data.start_time,
            aicpu_info.data.end_time,
            aicpu_info.data.duration,
        ]

    @staticmethod
    def get_kfc_comm_turn_data(aicpu_info: AicpuAddInfoBean) -> list:
        return [
            aicpu_info.data.device_id,
            aicpu_info.data.stream_id,
            aicpu_info.data.task_id,
            aicpu_info.data.comm_turn,
            aicpu_info.data.current_turn,
            aicpu_info.data.wait_notify_start_time,
            aicpu_info.data.kfc_alg_exe_start_time,
            aicpu_info.data.send_task_start_time,
            aicpu_info.data.wait_active_start_time,
            aicpu_info.data.active_start_time,
            aicpu_info.data.wait_exe_end_start_time,
            aicpu_info.data.rtsq_exe_end_time,
        ]

    @staticmethod
    def get_kfc_compute_turn_data(aicpu_info: AicpuAddInfoBean) -> list:
        return [
            aicpu_info.data.device_id,
            aicpu_info.data.stream_id,
            aicpu_info.data.task_id,
            aicpu_info.data.compute_turn,
            aicpu_info.data.current_turn,
            aicpu_info.data.wait_compute_start_time,
            aicpu_info.data.compute_start_time,
            aicpu_info.data.compute_exe_end_time,
        ]

    @staticmethod
    def get_hccl_op_info_data(aicpu_info: AicpuAddInfoBean) -> list:
        source = DeviceHcclOpSource.INVALID
        if aicpu_info.struct_type == str(AicpuAddInfoBean.HCCL_OP_INFO):
            source = DeviceHcclOpSource.HCCL
        return [
            InfoConfReader().time_from_syscnt(aicpu_info.timestamp),
            aicpu_info.data.relay,
            aicpu_info.data.retry,
            trans_enum_name(DataType, aicpu_info.data.data_type),
            aicpu_info.data.alg_type,
            aicpu_info.data.count,
            aicpu_info.data.group_name,
            aicpu_info.data.stream_id,
            aicpu_info.data.task_id,
            aicpu_info.data.rank_size,
            source.value,
        ]

    def get_kfc_hccl_info_data(self: any, aicpu_info: AicpuAddInfoBean) -> list:
        role = trans_enum_name(RoleType, aicpu_info.data.role)
        op_type = trans_enum_name(OpType, aicpu_info.data.op_type)
        data_type = trans_enum_name(DataType, aicpu_info.data.data_type)
        link_type = trans_enum_name(LinkType, aicpu_info.data.link_type)
        transport_type = trans_enum_name(TransPortType, aicpu_info.data.transport_type)
        rdma_type = trans_enum_name(RdmaType, aicpu_info.data.rdma_type)
        return [
            InfoConfReader().time_from_syscnt(aicpu_info.timestamp),
            self.hash_data.get(aicpu_info.data.item_id, aicpu_info.data.item_id),
            aicpu_info.data.ccl_tag,
            aicpu_info.data.group_name,
            aicpu_info.data.local_rank,
            aicpu_info.data.remote_rank,
            aicpu_info.data.rank_size,
            aicpu_info.data.work_flow_mode,
            aicpu_info.data.plane_id,
            aicpu_info.data.context_id,
            aicpu_info.data.notify_id,
            aicpu_info.data.stage,
            role,
            aicpu_info.data.duration_estimated,
            aicpu_info.data.src_addr,
            aicpu_info.data.dst_addr,
            aicpu_info.data.data_size,
            op_type,
            data_type,
            link_type,
            transport_type,
            rdma_type,
            aicpu_info.data.stream_id,
            aicpu_info.data.task_id,
        ]

    def parse(self: any) -> None:
        """
        parse ai cpu
        """
        aicpu_files = self._file_list.get(DataTag.AICPU_ADD_INFO, [])
        aicpu_info = self.parse_bean_data(
            aicpu_files,
            StructFmt.AI_CPU_ADD_FMT_SIZE,
            AicpuAddInfoBean,
            check_func=self.check_magic_num
        )
        self.hash_data = HashDictData(self._project_path).get_ge_hash_dict()
        self.set_aicpu_data(aicpu_info)

    def save(self: any) -> None:
        """
        save data to db
        :return:
        """
        aicpu_node_data = self._aicpu_data.get(AicpuAddInfoBean.AICPU_NODE, [])
        aicpu_dp_data = self._aicpu_data.get(AicpuAddInfoBean.AICPU_DP, [])
        aicpu_model_data = self._aicpu_data.get(AicpuAddInfoBean.AICPU_MODEL, [])
        aicpu_mi_data = self._aicpu_data.get(AicpuAddInfoBean.AICPU_MI, [])
        kfc_comm_turn = self._aicpu_data.get(AicpuAddInfoBean.KFC_COMM_TURN, [])
        kfc_compute_turn = self._aicpu_data.get(AicpuAddInfoBean.KFC_COMPUTE_TURN, [])
        kfc_hccl_info = self._aicpu_data.get(AicpuAddInfoBean.KFC_HCCL_INFO, [])
        hccl_op_info = self._aicpu_data.get(AicpuAddInfoBean.HCCL_OP_INFO, [])
        if aicpu_node_data:
            with AiCpuModel(self.project_path, [DBNameConstant.TABLE_AI_CPU]) as model:
                model.flush(aicpu_node_data, DBNameConstant.TABLE_AI_CPU)
        if aicpu_dp_data:
            with AiCpuModel(self.project_path, [DBNameConstant.TABLE_AI_CPU_DP]) as model:
                model.flush(aicpu_dp_data, DBNameConstant.TABLE_AI_CPU_DP)
        if aicpu_model_data:
            with TsTrackModel(self.project_path,
                              DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_MODEL_WITH_Q]) as model:
                model.create_table(DBNameConstant.TABLE_MODEL_WITH_Q)
                model.flush(DBNameConstant.TABLE_MODEL_WITH_Q, aicpu_model_data)
        if aicpu_mi_data:
            with DataPreparationModel(self.project_path, [DBNameConstant.TABLE_DATA_QUEUE]) as model:
                model.flush(aicpu_mi_data)
        if kfc_hccl_info:
            with KfcInfoModel(self.project_path, [DBNameConstant.TABLE_KFC_INFO]) as model:
                model.flush(kfc_hccl_info, DBNameConstant.TABLE_KFC_INFO)
        if kfc_comm_turn:
            with KfcInfoModel(self.project_path, [DBNameConstant.TABLE_KFC_COMM_TURN]) as model:
                model.flush(kfc_comm_turn, DBNameConstant.TABLE_KFC_COMM_TURN)
        if kfc_compute_turn:
            with KfcInfoModel(self.project_path, [DBNameConstant.TABLE_KFC_COMPUTE_TURN]) as model:
                model.flush(kfc_compute_turn, DBNameConstant.TABLE_KFC_COMPUTE_TURN)
        if hccl_op_info:
            with KfcInfoModel(self.project_path, [DBNameConstant.TABLE_DEVICE_HCCL_OP_INFO]) as model:
                model.flush(hccl_op_info, DBNameConstant.TABLE_DEVICE_HCCL_OP_INFO)

    def ms_run(self: any) -> None:
        """
        parse and save ge fusion data
        :return:
        """
        if not self._file_list.get(DataTag.AICPU_ADD_INFO, []):
            return
        logging.info("start parsing aicpu data, files: %s", str(self._file_list.get(DataTag.AICPU_ADD_INFO)))
        self.parse()
        self.save()

    def set_aicpu_data(self: any, aicpu_data: list) -> None:
        for aicpu_info in aicpu_data:
            get_data_func = self._get_data_func.get(int(aicpu_info.struct_type))
            if not get_data_func:
                logging.error("The aicpu type %d is invalid.", aicpu_info.struct_type)
                continue
            if int(aicpu_info.struct_type) == AicpuAddInfoBean.AICPU_NODE and \
                    (aicpu_info.data.ai_cpu_task_start_time == 0 or aicpu_info.data.ai_cpu_task_end_time == 0):
                continue
            self._aicpu_data.get(int(aicpu_info.struct_type)).append(get_data_func(aicpu_info))
