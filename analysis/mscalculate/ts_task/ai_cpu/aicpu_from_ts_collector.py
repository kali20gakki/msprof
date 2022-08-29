"""
function: analysis the data of ai cpu by iteration.
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
from msmodel.ai_cpu.ai_cpu_model import AiCpuModel
from msmodel.ge.ge_info_calculate_model import GeInfoModel
from common_func.db_name_constant import DBNameConstant
from common_func.constant import Constant


class AICpuFromTsCollector:
    STREAM_TASK_KEY_FMT = "{0}-{1}"
    STREAM_TASK_BATCH_KEY_FMT = "{0}-{1}-{2}"

    def __init__(self: any, project_path: str) -> None:
        self._project_path = project_path
        self._aicpu_model = AiCpuModel(self._project_path, [DBNameConstant.TABLE_AI_CPU_FROM_TS])
        self._check_map = self._get_check_map()
        self.aicpu_list = []

    def _get_check_map(self: any) -> dict:
        ge_info_model = GeInfoModel(self._project_path)
        ge_op_iter_dict = {}
        if ge_info_model.check_db() and ge_info_model.check_table():
            ge_op_iter_dict = ge_info_model.get_ge_data(Constant.TASK_TYPE_AI_CPU)
        ge_info_model.finalize()
        return ge_op_iter_dict

    def filter_aicpu(self: any, aicpu_feature: list) -> None:
        stream_id, task_id, start, end, batch_id, iter_id = aicpu_feature
        stream_task_value = self.STREAM_TASK_KEY_FMT.format(stream_id, task_id)
        stream_task_batch_value = self.STREAM_TASK_BATCH_KEY_FMT.format(stream_id, task_id, batch_id)
        if stream_task_value in self._check_map.get(str(iter_id), set()) or \
                stream_task_batch_value in self._check_map.get(str(iter_id), set()):
            self.aicpu_list.append([stream_id, task_id, start, end, batch_id])

    def save_aicpu(self: any) -> None:
        """
        save ai cpu data
        """
        with self._aicpu_model:
            self._aicpu_model.flush(self.aicpu_list, DBNameConstant.TABLE_AI_CPU_FROM_TS)
