from msmodel.ge.ge_info_calculate_model import GeInfoModel
from common_func.msvp_common import path_check
from common_func.path_manager import PathManager
from common_func.db_name_constant import DBNameConstant


class IterInfo:
    """
    class used to record iter info.
    """

    def __init__(self: any, model_id: int,
                             index_id: int,
                             iter_id: int,
                             start_time: int,
                             end_time: int) -> None:
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

    def is_aicore(self: any) -> str:
        """
        file name
        """
        return "iter_rec_parser"


class IterationManager:
    def __init__(self: any, project_path: str) -> None:
        # todo project path需要优化
        self.project_path = project_path
        self._iter_to_iter_info = {}
        self.is_parallel_scene = False

    def initial_iter_to_info(self: any) -> None:
        if not path_check(PathManager.get_db_path(self.project_path, DBNameConstant.DB_GE_INFO)):
            return
        with GeInfoModel(self.project_path) as ge_info_model:
            step_trace_data = ge_info_model.get_step_trace_data()

        for index, step_trace_datum in enumerate(step_trace_data):
            iter_info = self._iter_to_iter_info.setdefault(step_trace_datum.iter_id,
                                                           IterInfo(step_trace_datum.model_id,
                                                                   step_trace_datum.index_id,
                                                                   step_trace_datum.iter_id,
                                                                   step_trace_datum.step_start,
                                                                   step_trace_datum.step_end))
            for behind_datum in step_trace_data[index + 1:]:
                behind_iter_info = self._iter_to_iter_info.setdefault(behind_datum.iter_id,
                                                                      IterInfo(behind_datum.model_id,
                                                                   behind_datum.index_id,
                                                                   behind_datum.iter_id,
                                                                   behind_datum.step_start,
                                                                   behind_datum.step_end))
                if behind_iter_info.start_time < iter_info.end_time <= behind_iter_info.end_time:
                    iter_info.behind_parallel_iter.add(behind_iter_info)

    def get_iter_info(self: any, iter_id: int) -> any:
        return self._iter_to_iter_info.get(iter_id)

    def get_behind_parallel_iter_info(self: any, iter_id: int) -> set:
        return self._iter_to_iter_info.get(iter_id).behind_parallel_iter


class IterationUpdater:
    def __init__(self: any, project_path) -> None:
        self.active_parallel_iter_set = set([])
        self.iteration_manager = IterationManager(project_path)
        self.current_iter = -1
        self.min_same_end_iter = -1

    def update_parallel_iteration_pool(self: any) -> None:


    def add_new_iteration(self: any) -> None:
        pass