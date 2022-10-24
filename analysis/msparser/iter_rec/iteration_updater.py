class IterInfo:
    """
    class used to record iter info.
    """

    def __init__(self: any, model_id: int,
                             index_id: int,
                             iter_id: int,
                             start_time: int,
                             end_time: int,
                             behind_parallel_iter: set) -> None:
        self.model_id = model_id
        self.index_id = index_id
        self.iter_id = iter_id
        self.start_time = start_time
        self.end_time = end_time
        self.behind_parallel_iter = behind_parallel_iter
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
    def __init__(self: any) -> None:
        self.iter_to_iter_info = {}
        self.is_parallel_scene = False

    def initial_iter_to_info(self: any) -> None:




class IterationUpdater:
    def __init__(self: any) -> None:
        self.active_parallel_iter_set = set([])
        self.iteration_manager = IterationManager()
        self.current_iter = -1
        self.min_same_end_iter = -1

    def update_parallel_iteration_pool(self: any) -> None:
        pass

    def add_new_iteration(self: any) -> None:
        pass