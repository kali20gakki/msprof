"""
This script is used to load training_trace data from db
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""

import logging
import sqlite3

from common_func.ai_stack_data_check_manager import AiStackDataCheckManager
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.constant import Constant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.path_manager import PathManager
from common_func.step_trace_constant import StepTraceConstant
from common_func.msprof_exception import ProfException
from mscalculate.step_trace.tag_handler.tag_dispatch_handler import DispatchModelHandler


class CreateSubTable:
    """
    create sub table
    """
    data = []
    sample_config = {}
    db_name = None
    table_name = None

    @classmethod
    def extract_data(cls: any, collect_data: any) -> None:
        """
        extract data
        :param collect_data: collect data
        :return: void
        """

    @classmethod
    def create_table(cls: any, conn: any) -> None:
        """
        get table info
        :param conn: connect to database
        :return: create sql, insert sql
        """

    @classmethod
    def connect_db(cls: any) -> None:
        """
        connect and destroy database
        :return: void
        """
        db_path = PathManager.get_db_path(cls.sample_config.get("result_dir"), cls.db_name)
        conn, cur = DBManager.create_connect_db(db_path)
        cls.create_table(conn)
        DBManager.destroy_db_connect(conn, cur)

    @classmethod
    def run(cls: any, sample_config: dict, model_handler: any) -> None:
        """
        extract data and build table
        :param sample_config: sample config
        :param model_handler: model handler
        :return: void
        """
        collect_data = model_handler.get_data()
        cls.sample_config = sample_config
        try:
            cls.extract_data(collect_data)
        except ProfException as step_error:
            logging.error("Fail to create step trace sub table, error code: %s", step_error)
        cls.connect_db()


class CreateStepTraceData(CreateSubTable):
    """
    create step trace table
    """

    data = []
    sample_config = {}
    db_name = DBNameConstant.DB_STEP_TRACE
    table_name = DBNameConstant.TABLE_STEP_TRACE_DATA

    @classmethod
    def extract_data(cls: any, collect_data: any) -> None:
        for model_id, model_data in collect_data.items():
            for index_id, index_data in model_data.items():
                cls.data.append(
                    [index_id, model_id, index_data.get(
                        StepTraceConstant.STEP_START),
                     index_data.get(StepTraceConstant.STEP_END)])

    @classmethod
    def create_table(cls: any, conn: any) -> None:
        cls.data = sorted(cls.data, key=lambda x: x[2])
        cls.data = list(map(lambda datum: datum[1] + [datum[0] + 1], enumerate(cls.data)))

        create_sql = "create table if not exists {0} (index_id int, model_id int, " \
                     "step_start int, step_end int, iter_id int)".format(cls.table_name)

        DBManager.execute_sql(conn, create_sql)

        if not cls.data:
            return

        insert_sql = 'insert into {0} values ({1})'.format(
            DBNameConstant.TABLE_STEP_TRACE_DATA, ",".join("?" * len(cls.data[0])))
        DBManager.executemany_sql(conn, insert_sql, cls.data)


class CreateAllReduce(CreateSubTable):
    """
    create all reduce table
    """
    data = []
    sample_config = {}
    db_name = DBNameConstant.DB_TRACE
    table_name = DBNameConstant.TABLE_ALL_REDUCE

    @classmethod
    def extract_data(cls: any, collect_data: any) -> None:
        for model_id, model_data in collect_data.items():
            for _, index_data in model_data.items():
                for each_reduce in index_data[StepTraceConstant.ALL_REDUCE]:
                    cls.data.append(
                        [cls.sample_config.get("devices"), model_id,
                         index_data[StepTraceConstant.STEP_END], each_reduce[StepTraceConstant.REDUCE_START],
                         each_reduce[StepTraceConstant.REDUCE_END]])

    @classmethod
    def create_table(cls: any, conn: any) -> None:
        create_sql = "create table if not exists {}" \
                     "(device_id int, model_id int," \
                     "iteration_end int, start int, end int, primary key(device_id," \
                     "iteration_end, start))".format(DBNameConstant.TABLE_ALL_REDUCE)

        DBManager.execute_sql(conn, create_sql)

        if not cls.data:
            return

        insert_sql = 'insert into {0} values ({1})'.format(
            DBNameConstant.TABLE_ALL_REDUCE, ",".join("?" * len(cls.data[0])))

        DBManager.executemany_sql(conn, insert_sql, cls.data)


class CreateTrainingTrace(CreateSubTable):
    """
    create training trace table
    """
    data = []
    sample_config = {}
    db_name = DBNameConstant.DB_TRACE
    table_name = DBNameConstant.TABLE_TRAINING_TRACE

    @staticmethod
    def check_timestamp(model_id: int, index_id: int, step_start: int, step_end: int) -> None:
        """
        check timestamp
        :param model_id: model id
        :param index_id: index id
        :param step_start: starting timestamp of a iter
        :param step_end: ending timestamp of a iter
        :return:
        """
        if step_start == NumberConstant.NULL_NUMBER or step_end == NumberConstant.NULL_NUMBER:
            logging.error("step time is None, step start: %d, step end: %d, "
                          "model id: %d, index id: %d", step_start, step_end, model_id, index_id)
            raise ProfException(ProfException.PROF_INVALID_STEP_TRACE_ERROR)

        if step_start == step_end:
            logging.error("start time equals to end time, step start: %d, step end: %d, "
                          "model id: %d, index id: %d", step_start, step_end, model_id, index_id)
            raise ProfException(ProfException.PROF_INVALID_STEP_TRACE_ERROR)

    @classmethod
    def extract_data(cls: any, collect_data: any) -> None:
        for model_id, model_data in collect_data.items():
            for index_id, index_data in model_data.items():
                cls.update_data(model_id, index_id, index_data)

    @classmethod
    def create_table(cls: any, conn: any) -> None:
        create_sql = "create table if not exists {0} " \
                     "(device_id int, model_id int, iteration_id int, " \
                     "FP_start int, " \
                     "BP_end int, iteration_end int, " \
                     "iteration_time int, fp_bp_time int, grad_refresh_bound int, " \
                     "data_aug_bound int)".format(DBNameConstant.TABLE_TRAINING_TRACE)

        DBManager.execute_sql(conn, create_sql)

        if not cls.data:
            return

        cls.update_data_aug()

        insert_sql = 'insert into {0} values ({1})'.format(
            DBNameConstant.TABLE_TRAINING_TRACE, ",".join("?" * len(cls.data[0])))
        DBManager.executemany_sql(conn, insert_sql, cls.data)

    @classmethod
    def update_data(cls: any, model_id: int, index_id: int, index_data: dict) -> None:
        """
        update data
        :param model_id: model id
        :param index_id: index id
        :param index_data: index data
        :return:
        """
        training_trace = index_data.get(StepTraceConstant.TRAINING_TRACE, {})
        step_start = index_data.get(StepTraceConstant.STEP_START, NumberConstant.NULL_NUMBER)
        step_end = index_data.get(StepTraceConstant.STEP_END, NumberConstant.NULL_NUMBER)

        cls.check_timestamp(model_id, index_id, step_start, step_end)

        forward_pro = training_trace.get(StepTraceConstant.FORWARD_PROPAGATION, NumberConstant.NULL_NUMBER)

        back_pro = training_trace.get(StepTraceConstant.BACK_PROPAGATION, NumberConstant.NULL_NUMBER)

        iteration_time = step_end - step_start
        fp_bp_time = \
            NumberConstant.NULL_NUMBER if not (forward_pro and back_pro) else back_pro - forward_pro
        grad_refresh_bound = \
            NumberConstant.NULL_NUMBER if not back_pro else step_end - back_pro

        cls.data.append([cls.sample_config.get("devices"), model_id, index_id, forward_pro,
                         back_pro, step_end, iteration_time, fp_bp_time,
                         grad_refresh_bound, NumberConstant.NULL_NUMBER])

    @classmethod
    def update_data_aug(cls: any) -> None:
        """
        update data aug
        :return: void
        """
        cls.data.sort(key=lambda datum: datum[NumberConstant.STEP_END])
        for current_iter_index, current_datum in enumerate(cls.data):
            if current_datum[NumberConstant.FORWARD_PROPAGATION]:
                last_iter_index = cls.__find_closest_step_end_index(
                    current_iter_index)
                if last_iter_index >= 0:
                    current_datum[
                        NumberConstant.DATA_AUG_BOUND] = \
                        current_datum[
                            NumberConstant.FORWARD_PROPAGATION] - \
                        cls.data[last_iter_index][
                            NumberConstant.STEP_END]

    @classmethod
    def __find_closest_step_end_index(cls: any, current_iter_index: int) -> int:
        """
        find last iter index. Last iter is the closest to
        current iter, and its iter end is before fp of current iter
        :param current_iter_index: current_iter_index
        :return: void
        """
        last_iter_index = current_iter_index - 1

        while last_iter_index >= 0:
            if cls.data[current_iter_index][NumberConstant.FORWARD_PROPAGATION] > \
                    cls.data[last_iter_index][NumberConstant.STEP_END]:
                break
            last_iter_index = last_iter_index - 1

        if last_iter_index < current_iter_index - 1:
            logging.warning("The last iter of the %s iter of "
                            "total iters is not %s, but is %s",
                            current_iter_index,
                            current_iter_index - 1, last_iter_index)

        return last_iter_index


class StepTableBuilder:
    """
    create table from step trace
    """
    TIMESTAMP_INDEX = 2
    model_handler = DispatchModelHandler()
    table_list = [CreateStepTraceData, CreateAllReduce, CreateTrainingTrace]
    step_conn = None
    step_curs = None

    @staticmethod
    def to_dict(record: list) -> dict:
        """
        transform list to dict
        :param record: contain "model_id", "tag_id", timestamp
        :return:
        """
        record_dict = {
            StepTraceConstant.INDEX_ID: record[0], StepTraceConstant.MODEL_ID: record[1],
            StepTraceConstant.TIME_STAMP: record[2], StepTraceConstant.TAG_ID: record[3]
        }

        return record_dict

    @classmethod
    def process_step_trace(cls: any, step_trace: list) -> None:
        """
        process step trace
        :param step_trace: contain model_id, tag_id, timestamp
        :return: void
        """
        step_trace.sort(key=lambda x: x[cls.TIMESTAMP_INDEX])
        for record in step_trace:
            cls.model_handler.receive_record(cls.to_dict(record))

    @classmethod
    def build_table(cls: any, sample_config: dict) -> None:
        """
        build step trace data, all reduce, training trace table
        :param sample_config: sample config
        :return: void
        """
        if AiStackDataCheckManager.contain_training_trace_data(sample_config.get("result_dir")):
            cls.table_list.remove(CreateAllReduce)
            cls.table_list.remove(CreateTrainingTrace)

        for create_table in cls.table_list:
            create_table.run(sample_config, cls.model_handler)

    @classmethod
    def run(cls: any, sample_config: dict) -> None:
        """
        run class
        :param sample_config: sample config
        :return: void
        """
        cls._connect_step_db(sample_config)
        step_trace_data = cls._get_step_trace_data()
        if not step_trace_data:
            return
        cls.process_step_trace(step_trace_data)
        cls.build_table(sample_config)

    @classmethod
    def _get_step_data(cls: any, table_name: str) -> list:
        if not DBManager.judge_table_exist(cls.step_curs, table_name):
            return []

        # iteration range table
        select_sql = "select DISTINCT index_id, model_id, " \
                     "timestamp, tag_id from {}".format(table_name)

        return DBManager.fetch_all_data(cls.step_curs, select_sql)

    @classmethod
    def _connect_step_db(cls: any, sample_config: dict) -> None:
        db_path = PathManager.get_db_path(sample_config.get("result_dir"), DBNameConstant.DB_STEP_TRACE)
        cls.step_conn, cls.step_curs = DBManager.check_connect_db_path(db_path)
        if not cls.step_conn or not cls.step_curs:
            return

    @classmethod
    def _get_step_trace_data(cls: any) -> list:
        _step_data = cls._get_step_data(DBNameConstant.TABLE_STEP_TRACE)
        _helper_data = cls._get_step_data(DBNameConstant.TABLE_MODEL_WITH_Q)
        step_trace_data = _step_data + _helper_data
        if not step_trace_data:
            return []
        _step_trace = sorted(step_trace_data, key=lambda x: float(x[2]))
        DBManager.destroy_db_connect(cls.step_conn, cls.step_curs)
        return _step_trace
