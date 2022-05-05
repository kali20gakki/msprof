"""
This script is used to parse and save time data to db.
Copyright Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
"""

import configparser
import logging
import os

from common_func.constant import Constant
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.file_manager import check_path_valid
from common_func.file_name_manager import get_file_name_pattern_match
from common_func.file_name_manager import get_host_start_compiles
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from saver.training.time_saver import TimeSaver


class TimeParser(MsMultiProcess):
    """
    parser of time data
    """

    def __init__(self: any, config: dict) -> None:
        MsMultiProcess.__init__(self, config)
        self.path = config["result_dir"]
        self.sql_path = PathManager.get_sql_dir(self.path)

    def parse_time(self: any) -> None:
        """
        parse job info
        :return: None
        """
        file_patterns = get_host_start_compiles()
        for file_name in os.listdir(self.path):
            host_start_result = get_file_name_pattern_match(file_name, *file_patterns)
            if host_start_result and is_valid_original_data(file_name, self.path, is_conf=True):
                message = self.get_sync_data(file_name)
                message['sql_path'] = self.sql_path
                if TimeSaver.save_data_to_db(message):
                    FileManager.add_complete_file(self.path, file_name)

    def get_sync_data(self: any, file_name: str) -> dict:
        """
        path:/FILE_PATH/analysis/analysis_num
        get time sync data from dev_start.log and host_start.log
        :param file_name: file name
        :return: sync time dict
        """
        try:
            if not os.path.exists(os.path.join(self.path, file_name)):
                logging.error("No data file was generated.")
                raise ProfException(ProfException.PROF_INVALID_PATH_ERROR)
        except (OSError, ProfException) as err:
            logging.error("Error in parsing sync data:%s", str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return {}
        logging.info("Start parsing sync data ...")
        # initialize time synchronize messages
        message = self.__parse_time_data()
        logging.info("Parsing sync data finished...")
        return message

    def __parse_time_data_helper(self: any, message: dict, index: int) -> None:
        host_start_path = os.path.join(self.path, "host_start.log.{}".format(index))
        dev_start_path = os.path.join(self.path, "dev_start.log.{}".format(index))
        if os.path.exists(host_start_path) and os.path.exists(dev_start_path):
            check_path_valid(host_start_path, True)
            config = configparser.ConfigParser()
            config.read(host_start_path)
            host_wall, host_mon = self.parse_host_start(config, index)
            with FileOpen(dev_start_path, "r") as dev_:
                dev_wall, dev_mon, dev_cntvct = self.parse_dev_start(dev_.file_reader)

            data = {"device_id": index, "dev_mon": int(dev_mon),
                    "dev_wall": int(dev_wall), "dev_cntvct": int(dev_cntvct),
                    "host_mon": int(host_mon), "host_wall": int(host_wall)}
            message["data"].append(data)

    def __parse_time_data(self: any) -> dict:
        """
        path:/FILE_PATH/analysis/
        call parse_host_start to parse host_start.log
        call parse_dev_start to parse dev_start.log
        """
        message = {"data": []}
        try:
            for index in range(8):
                self.__parse_time_data_helper(message, index)
            return message
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error('Parse time sync data error: %s', str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return message

    @staticmethod
    def parse_host_start(config: any, index: int) -> tuple:
        """
        parse host_start.log
        :param config: config parser
        :param index: device id
        :return: host monotonic time and host wall time
        """
        if not config.has_section("Device{}".format(index)):
            return -1, -1
        times = config.items("Device{}".format(index))
        host_wall, host_mon = 0, 0
        for clock in times:
            if clock[0] == "clock_realtime":
                host_wall = clock[1]
            elif clock[0] == "clock_monotonic_raw":
                host_mon = clock[1]
        return host_wall, host_mon

    @staticmethod
    def parse_dev_start(dev_: any) -> tuple:
        """
        parse dev_start.log
        :param dev_: file reader
        :return: device time
        """
        dev_wall, dev_mon, dev_cntvct = 0, 0, 0
        while True:
            line = dev_.readline(Constant.MAX_READ_LINE_BYTES)
            if not line:
                break
            replay_status, timestamp = line.strip().split(":")
            if replay_status == 'clock_realtime':
                dev_wall = int(float(timestamp))
            elif replay_status == 'clock_monotonic_raw':
                dev_mon = int(float(timestamp))
            elif replay_status == 'cntvct':
                dev_cntvct = int(float(timestamp))
            elif dev_wall and dev_mon and dev_cntvct:
                break
        return dev_wall, dev_mon, dev_cntvct

    def ms_run(self: any) -> None:
        """
        entrance of time parser
        :return: None
        """
        try:
            self.parse_time()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            pass
