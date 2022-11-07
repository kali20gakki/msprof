import logging

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel


class ParallelModel(ParserModel):
    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_PARALLEL, [DBNameConstant.TABLE_PARALLEL_STRATEGY])

    def flush(self: any, table_name: str, data_list: list) -> None:
        """
        flush data to db
        """
        self.insert_data_to_db(table_name, data_list)


class ParallelViewModel(ViewModel):
    def __init__(self: any, path: str) -> None:
        super().__init__(path, DBNameConstant.DB_PARALLEL, [])

    def get_parallel_table_name(self: any) -> str:
        sql = "select parallel_mode from {}".format(DBNameConstant.TABLE_PARALLEL_STRATEGY)
        if not DBManager.fetch_all_data(self.cur, sql):
            return Constant.NA
        parallel_mode = DBManager.fetch_all_data(self.cur, sql)[0]
        if not parallel_mode:
            return Constant.NA
        elif parallel_mode[0] == "data-parallel":
            return DBNameConstant.TABLE_DATA_PARALLEL
        elif parallel_mode[0] == "model-parallel":
            return DBNameConstant.TABLE_MODEL_PARALLEL
        elif parallel_mode[0] == "pipeline-parallel":
            return DBNameConstant.TABLE_PIPELINE_PARALLEL
        else:
            return Constant.NA