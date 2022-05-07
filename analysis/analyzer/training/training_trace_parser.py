"""
This script is used to parse and save training_trace data to db
Copyright Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
"""

import logging
import os
import struct

from common_func.constant import Constant
from common_func.file_manager import FileManager
from common_func.file_name_manager import FileNameManagerConstant
from common_func.file_name_manager import get_file_name_pattern_match
from common_func.file_name_manager import get_training_trace_compiles
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_valid_original_data
from common_func.ms_constant.str_constant import StrConstant
from common_func.path_manager import PathManager
from saver.training.training_trace_saver import TrainingTraceSaver


class TrainingTraceParser(MsMultiProcess):
    """
    parser of training trace data
    """
    UPPER_LIMIT_TAG_ID = 65535

    def __init__(self: any, config: dict) -> None:
        MsMultiProcess.__init__(self, config)
        self.path = config["result_dir"]
        self.host_id = config["host_id"]
        self.job_id = ""
        self.sql_path = PathManager.get_sql_dir(self.path)

    def parse_training_trace(self: any) -> None:
        """
        parse job info
        """
        data_path = os.path.join(self.path, StrConstant.DATA_PATH)
        file_patterns = get_training_trace_compiles()
        for file_name in os.listdir(data_path):
            training_result = get_file_name_pattern_match(file_name, *file_patterns)
            if training_result and is_valid_original_data(file_name, self.path):
                self.job_id = InfoConfReader().get_job_info()
                file_relation = {"job_id": self.job_id,
                                 "file_path": os.path.join(self.path),
                                 'sql_path': self.sql_path}
                if not TrainingTraceSaver.save_file_relation_to_db(file_relation):
                    return
                message = self.get_cp_data(file_name)
                message['sql_path'] = self.sql_path
                if TrainingTraceSaver.save_data_to_db(message):
                    FileManager.add_complete_file(self.path, file_name)

    def get_cp_data(self: any, file_name: str) -> dict:
        """
        path:/FILE_PATH/analysis/analysis_num/host_id/tag_id
        get cp data
        """
        file_name = os.path.basename(file_name)
        check_name = get_file_name_pattern_match(file_name, *get_training_trace_compiles())
        if check_name:
            device_id = check_name.groups()[FileNameManagerConstant.MATCHED_DEV_ID_INX]
        else:
            logging.error('Invalid file name.')
            return {}
        iteration_id = 1
        file_name = os.path.join(self.path, StrConstant.DATA_PATH, file_name)
        if os.path.exists(file_name):
            logging.info("Start parsing training trace data...")
            # initialize framework messages
            with open(file_name, "rb") as file_:
                param = [iteration_id, self.host_id, device_id]
                message, iteration_id = self.__parse_trace_data(file_, param)
            logging.info("Parsing training trace data finished...")
            return message
        logging.error("No data file was generated.")
        return {}

    def __parse_trace_data_helper(self: any, message: dict, file_: any, iteration_id: any) -> None:
        while True:

            job_id_bin = file_.read(struct.calcsize("=Q"))

            if job_id_bin:
                # job_id
                cp_add = {"iteration_id": iteration_id}
                message["data"].append(cp_add)

                job_id = struct.unpack("=Q", job_id_bin)[0]
                if job_id > self.UPPER_LIMIT_TAG_ID:
                    cp_add["job_id"] = str(self.job_id)
                    cp_add["job_task"] = struct.unpack("=H",
                                                       file_.read(struct.calcsize("=H")))[0]
                    cp_add["job_stream"] = struct.unpack("=H",
                                                         file_.read(struct.calcsize("=H")))[0]
                    # syscnt for job_id
                    _ = struct.unpack("=Q", file_.read(struct.calcsize("=Q")))[0]

                    check_id_bin = file_.read(struct.calcsize("=Q"))

                    cp_add["all_reduces"] = []
                    self.__get_fp_bp(check_id_bin, cp_add, file_)
                    iteration_id += 1
                else:
                    _ = file_.read(struct.calcsize("=HHQ"))
                    continue
            else:
                break

    def __parse_trace_data(self: any, file_: any, param: any) -> tuple:
        """parse cp data"""
        message = {"host_id": param[1],
                   "device_id": int(param[2]),
                   "data": []}

        iteration_id = int(param[0])
        try:
            self.__parse_trace_data_helper(message, file_, iteration_id)
            return message, iteration_id
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error("Parse training trace data err: %s", str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return {}, 0

    def __get_fp_bp(self: any, check_id_bin: any, cp_add: dict, file_: any) -> None:
        """parse fp start, bp end, reduce start, reduce end"""
        while check_id_bin:
            check_id = struct.unpack("=Q", check_id_bin)[0]
            if check_id < self.UPPER_LIMIT_TAG_ID:
                if check_id == 1:
                    cp_add["FP_task"] = \
                        struct.unpack("=H", file_.read(struct.calcsize("=H")))[0]
                    cp_add["FP_stream"] = \
                        struct.unpack("=H", file_.read(struct.calcsize("=H")))[0]
                    cp_add["FP_start"] = \
                        struct.unpack("=Q", file_.read(struct.calcsize("=Q")))[0]

                elif check_id == 2:
                    cp_add["BP_task"] = \
                        struct.unpack("=H", file_.read(struct.calcsize("=H")))[0]
                    cp_add["BP_stream"] = \
                        struct.unpack("=H", file_.read(struct.calcsize("=H")))[0]
                    cp_add["BP_end"] = \
                        struct.unpack("=Q", file_.read(struct.calcsize("=Q")))[0]
                elif check_id % 2 == 1:
                    cp_reduceadd = {}
                    cp_reduceadd["start_task"] = \
                        struct.unpack("=H", file_.read(struct.calcsize("=H")))[0]
                    cp_reduceadd["start_stream"] = \
                        struct.unpack("=H", file_.read(struct.calcsize("=H")))[0]
                    cp_reduceadd["start"] = \
                        struct.unpack("=Q", file_.read(struct.calcsize("=Q")))[0]
                elif check_id % 2 == 0:
                    cp_reduceadd["end_task"] = \
                        struct.unpack("=H", file_.read(struct.calcsize("=H")))[0]
                    cp_reduceadd["end_stream"] = \
                        struct.unpack("=H", file_.read(struct.calcsize("=H")))[0]
                    cp_reduceadd["end"] = \
                        struct.unpack("=Q", file_.read(struct.calcsize("=Q")))[0]
                    cp_add["all_reduces"].append(cp_reduceadd)

                check_id_bin = file_.read(struct.calcsize("=Q"))
            elif check_id == self.UPPER_LIMIT_TAG_ID:
                cp_add["iter_task"] = \
                    struct.unpack("=H", file_.read(struct.calcsize("=H")))[0]
                cp_add["iter_stream"] = \
                    struct.unpack("=H", file_.read(struct.calcsize("=H")))[0]
                cp_add["iteration_end"] = \
                    struct.unpack("=Q", file_.read(struct.calcsize("=Q")))[0]
                break
            else:
                break

    def ms_run(self: any) -> None:
        """
        main entry for training trace parser
        :return: None
        """
        try:
            self.parse_training_trace()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            pass
