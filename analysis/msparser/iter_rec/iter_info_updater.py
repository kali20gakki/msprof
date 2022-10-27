from msmodel.ge.ge_info_calculate_model import GeInfoModel
from common_func.msvp_common import path_check
from common_func.path_manager import PathManager
from common_func.db_name_constant import DBNameConstant
from common_func.constant import Constant


class IterInfo:
    """
    class used to record iter info.
    """

    def __init__(self: any, model_id: int = -1,
                             index_id: int = -1,
                             iter_id: int = -1,
                             start_time: int = -1,
                             end_time: int = -1) -> None:
        self.model_id = model_id
        self.index_id = index_id
        self.iter_id = iter_id
        self.start_time = start_time
        self.end_time = end_time
        self.behind_parallel_iter = set([])
        self.aic_count = 0
        self.hwts_count = 0
        self.aic_offset = 0
        self.hwts_offset = 0
        self.static_aic_task_set = set([])
        self.dynamic_aic_task_set = set([])

    @staticmethod
    def class_name() -> str:
        """
        class name
        """
        return "IterInfo"

    @staticmethod
    def file_name() -> str:
        """
        file name
        """
        return "iter_rec_parser"

    def is_aicore(self: any, task: any) -> bool:
        """
        judge is aicore
        """
        stream_task_batch = GeInfoModel.STREAM_TASK_BATCH_KEY_FMT.format(task.stream_id, task.task_id, task.batch_id)
        return stream_task_batch in self.static_aic_task_set or stream_task_batch in self.dynamic_aic_task_set


class IterationManager:
    def __init__(self: any, project_path: str) -> None:
        self.project_path = project_path
        self.iter_to_iter_info = {}
        self.is_parallel_scene = False
        self._initial_iter_to_info()

    def _initial_iter_to_info(self: any) -> None:
        if not path_check(PathManager.get_db_path(self.project_path, DBNameConstant.DB_GE_INFO)):
            return
        with GeInfoModel(self.project_path) as ge_info_model:
            step_trace_data = ge_info_model.get_step_trace_data()
            static_task_dict = ge_info_model.get_ge_task_data(Constant.TASK_TYPE_AI_CORE, Constant.GE_STATIC_SHAPE)
            dynamic_task_dict = ge_info_model.get_ge_task_data(Constant.TASK_TYPE_AI_CORE, Constant.GE_NON_STATIC_SHAPE)

        self._regist_paralle_set(step_trace_data)
        self._regist_aicore_set(static_task_dict, dynamic_task_dict)

    def _regist_paralle_set(self: any, step_trace_data: list) -> None:
        for index, step_trace_datum in enumerate(step_trace_data):
            iter_info = self.iter_to_iter_info.setdefault(step_trace_datum.iter_id,
                                                          IterInfo(step_trace_datum.model_id,
                                                                    step_trace_datum.index_id,
                                                                    step_trace_datum.iter_id,
                                                                    step_trace_datum.step_start,
                                                                    step_trace_datum.step_end))
            for behind_datum in step_trace_data[index:]:
                behind_iter_info = self.iter_to_iter_info.setdefault(behind_datum.iter_id,
                                                                     IterInfo(behind_datum.model_id,
                                                                               behind_datum.index_id,
                                                                               behind_datum.iter_id,
                                                                               behind_datum.step_start,
                                                                               behind_datum.step_end))
                if behind_iter_info.start_time < iter_info.end_time <= behind_iter_info.end_time:
                    iter_info.behind_parallel_iter.add(behind_datum.iter_id)

    def _regist_aicore_set(self: any, static_task_dict: dict, dynamic_task_dict: dict) -> None:
        for iter_info_bean in self.iter_to_iter_info.values():
            iter_info_bean.static_aic_task_set = static_task_dict.get(iter_info_bean.model_id, set([]))
            iter_info_bean.dynamic_aic_task_set = dynamic_task_dict.get(
                GeInfoModel.MODEL_INDEX_KEY_FMT.format(
                    iter_info_bean.model_id,
                    iter_info_bean.index_id),
                set([]))


class IterInfoUpdater:
    HWTS_TASK_END = 1

    def __init__(self: any, project_path) -> None:
        self.current_iter = -1
        self.active_parallel_iter_set = set([])
        self.iteration_manager = IterationManager(project_path)

    def update_parallel_iter_info_pool(self: any, iter_id: int) -> None:
        if iter_id <= self.current_iter:
            return

        new_iter_info = self.iteration_manager.iter_to_iter_info.get(iter_id, IterInfo())
        new_add_parallel_id = (self.iteration_manager.iter_to_iter_info.get(it_id) for it_id in
                               new_iter_info.behind_parallel_iter - self.active_parallel_iter_set)
        self._update_new_add_iter_info(new_add_parallel_id)

        self.current_iter = iter_id
        self.active_parallel_iter_set = new_iter_info.behind_parallel_iter

    def _update_new_add_iter_info(self: any, new_add_parallel_id: any) -> None:
        current_iter_info = self.iteration_manager.iter_to_iter_info.get(self.current_iter, IterInfo())

        for new_add_parallel_iter_info in new_add_parallel_id:
            new_add_parallel_iter_info.hwts_offset = current_iter_info.hwts_offset + current_iter_info.hwts_count
            new_add_parallel_iter_info.aic_offset = current_iter_info.aic_offset + current_iter_info.aic_count

    def update_count_and_offset(self: any, task: any, ai_core_task: set) -> None:
        iter_info_list = [self.iteration_manager.iter_to_iter_info.get(iter_id)
                          for iter_id in self.active_parallel_iter_set]

        self._update_hwts(iter_info_list)

        if task.sys_tag == self.HWTS_TASK_END and \
                self._judge_ai_core(task, iter_info_list, ai_core_task):
            self._update_aicore(iter_info_list)

    @staticmethod
    def _judge_ai_core(task: any, iter_info_list: list, ai_core_task: set) -> bool:
        # if there are ge data, ai_core_task is empty
        if not ai_core_task:
            return any([iter_info_bean.is_aicore(task) for iter_info_bean in iter_info_list])
        return GeInfoModel.STREAM_TASK_KEY_FMT.format(task.stream_id, task.task_id) in ai_core_task

    @staticmethod
    def _update_hwts(iter_info_list: list) -> None:
        for iter_info_bean in iter_info_list:
            iter_info_bean.hwts_count += 1

    @staticmethod
    def _update_aicore(iter_info_list: list) -> None:
        for iter_info_bean in iter_info_list:
            iter_info_bean.aic_count += 1
