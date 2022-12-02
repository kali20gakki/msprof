#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import configparser
import csv
import json
import logging
import os
import shutil
from collections import OrderedDict
from decimal import Decimal
from functools import reduce
from operator import add
from operator import mul
from operator import sub
from operator import truediv

from common_func.common import error
from common_func.constant import Constant
from common_func.file_manager import check_path_valid
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.os_manager import check_file_readable
from common_func.path_manager import PathManager
from common_func.return_code_checker import ReturnCodeCheck
from config.config_manager import ConfigManager


class MsvpCommonConst:
    """
    msvp common
    """
    CONFIG_PATH = os.path.join(os.path.realpath(os.path.dirname(__file__)), "..", "config")
    FILE_NAME = os.path.basename(__file__)
    CPU_CONFIG_TYPE = {
        "ts_cpu": ConfigManager.get("TsCPUConfig"),
        "ai_cpu": ConfigManager.get("AICPUConfig"),
        "ai_core": ConfigManager.get("AICoreConfig"),
        "ctrl_cpu": ConfigManager.get("CtrlCPUConfig"),
        "constant": ConfigManager.get("ConstantConfig")
    }

    @staticmethod
    def class_name() -> str:
        """
        class name
        """
        return "MsvpCommonConst"

    def file_name(self: any) -> str:
        """
        file name
        """
        return self.FILE_NAME


def _compute_clock_realtime(timestamp: str) -> float:
    try:
        dev_wall = float(timestamp) / NumberConstant.NANO_SECOND
    except ValueError:
        dev_wall = 0
    finally:
        pass
    return dev_wall


def _compute_clock_monotonic_raw(timestamp: str) -> float:
    try:
        dev_mon = float(timestamp) / NumberConstant.NANO_SECOND
    except ValueError:
        dev_mon = 0
    finally:
        pass
    return dev_mon


def _compute_cntvct(timestamp: str) -> float:
    dev_cnt = 0
    try:
        dev_cnt = float(timestamp) / NumberConstant.NANO_SECOND
    except ValueError:
        dev_cnt = 0
    finally:
        pass
    return dev_cnt


def _get_device_duration_delta_time(file_handler: any) -> tuple:
    dev_wall, dev_mon, dev_cnt = 0, 0, 0
    while True:
        line = file_handler.readline()
        if not line:
            break
        replay_status, timestamp = line.strip().split(":")
        if replay_status == 'clock_realtime':
            dev_wall = _compute_clock_realtime(timestamp)
        elif replay_status == 'clock_monotonic_raw':
            dev_mon = _compute_clock_monotonic_raw(timestamp)
        elif replay_status == 'cntvct':
            dev_cnt = _compute_cntvct(timestamp)
        elif dev_wall and dev_mon:
            break
    return dev_wall, dev_mon, dev_cnt


def get_device_duration_delta_time(device_duration_file: str, dev_wall: int, dev_mon: int) -> tuple:
    """
    get device duration delta time
    :param device_duration_file: device duration file
    :param dev_wall: device wall time
    :param dev_mon: device monotonic time
    :return: device time and sys cnt
    """
    dev_cnt = 0
    if os.path.exists(device_duration_file):
        check_file_readable(device_duration_file)
        with open(device_duration_file, 'r') as file_handler:
            dev_wall, dev_mon, dev_cnt = _get_device_duration_delta_time(file_handler)
    return dev_wall, dev_mon, dev_cnt


def get_host_duration_delta_time(host_duration_file: str, host_wall: int, host_mon: int, device_id: int) -> tuple:
    """
    get host duration delta time
    :param host_duration_file: host duraton file
    :param host_wall: host wall time
    :param host_mon: host monotonic time
    :param device_id: device id
    :return: host time
    """
    if os.path.exists(host_duration_file):
        check_file_readable(host_duration_file)
        config = configparser.ConfigParser()
        config.read(host_duration_file)
        if config.has_section("Device" + str(device_id)):
            get_time = config.items("Device" + str(device_id))
        else:
            get_time = config.items("Host")
        host_wall = float(get_time[0][1]) / NumberConstant.NANO_SECOND
        host_mon = float(get_time[1][1]) / NumberConstant.NANO_SECOND
    return host_wall, host_mon


def get_host_device_time(host_duration_file: str, device_duration_file: str, device_id: any) -> tuple:
    """
    provides host and device time for get_delta_time method
    :param host_duration_file: host duration file
    :param device_duration_file: device duration file
    :param device_id: device id
    :return: host and device time
    """
    host_wall, host_mon, dev_wall, dev_mon, dev_cnt = 0, 0, 0, 0, 0
    host_device_time = (host_wall, host_mon, dev_wall, dev_mon, dev_cnt)
    try:
        if host_duration_file and device_duration_file:
            host_wall, host_mon = get_host_duration_delta_time(host_duration_file,
                                                               host_wall, host_mon, device_id)
            dev_wall, dev_mon, dev_cnt = get_device_duration_delta_time(device_duration_file,
                                                                        dev_wall, dev_mon)
            host_device_time = (host_wall, host_mon, dev_wall, dev_mon, dev_cnt)
        return host_device_time
    except (OSError, SystemError, NameError, TypeError, RuntimeError) as err:
        logging.error(err)
        return host_device_time


def get_delta_time(project_path: str, device_id: int, view_type: int = 0) -> tuple:
    """
    calculate time difference between host and device
    view_type=0:change all data time to host wall time
    view_type=1:change all data time to host monotic time
    view_type=2:change all data time to device wall time
    view_type=3:change all data time to device monotic time
    """
    host_duration_file = os.path.join(project_path, "host_start.log." + str(device_id))
    device_duration_file = os.path.join(project_path, "dev_start.log." + str(device_id))

    delta_host, delta_dev = 0, 0
    host_wall, host_mon, dev_wall, dev_mon, _ = \
        get_host_device_time(host_duration_file, device_duration_file, device_id)
    if view_type == 0:
        delta_host = host_wall - host_mon
        delta_dev = host_wall - dev_mon
        return delta_host, delta_dev
    if view_type == 1:
        delta_host = 0
        delta_dev = host_mon - dev_mon
        return delta_host, delta_dev
    if view_type == 2:
        delta_host = dev_wall - host_mon
        delta_dev = dev_wall - dev_mon
        return delta_host, delta_dev
    if view_type == 3:
        delta_host = dev_mon - host_mon
        delta_dev = 0
        return delta_host, delta_dev
    return delta_host, delta_dev


def config_file_obj(file_name: str = "constant") -> any:
    """
    :param file_name: config file name
    :return: config file object
    """
    try:
        config = MsvpCommonConst.CPU_CONFIG_TYPE.get(file_name)
        return config
    except configparser.Error:
        return []
    finally:
        pass


def _get_cpu_metrics(sections: str, cpu_cfg: any) -> dict:
    if sections == 'metrics':
        metrics_map = cpu_cfg.items(sections)
        metrics = OrderedDict(metrics_map)
        return metrics
    if sections == 'formula':
        return OrderedDict(cpu_cfg.items("formula"))
    if sections == 'formula_l2':
        return OrderedDict(cpu_cfg.items("formula_l2"))
    return {}


def read_cpu_cfg(cpu_type: str, sections: str) -> dict:
    """
    read cpu configure file
    :param cpu_type: cpu type
    :param sections: section for cpu
    :return:
    """
    cpu_cfg = MsvpCommonConst.CPU_CONFIG_TYPE.get(cpu_type)
    try:
        if sections in ['events', 'event2metric']:
            events_map = cpu_cfg.items(sections)
            events = {int(k, Constant.HEX_NUMBER): v for k, v in events_map}
            return events
        return _get_cpu_metrics(sections, cpu_cfg)
    except configparser.Error:
        return {}
    finally:
        pass


def get_cpu_event_config(sample_config: dict, cpu_type: str) -> list:
    """
    get cpu event from config
    :param sample_config: sample config
    :param cpu_type: cpu type
    :return: cpu events
    """
    events = []
    for _events in sample_config.get(cpu_type + "_profiling_events").split(","):
        if _events != '0x11':
            events.append(_events)
    cpu_events = []
    new_events = (events[i:i + 6] for i in range(0, len(events), 6))
    for j in new_events:
        cpu_events.append(j)
    if cpu_type == 'ai_ctrl_cpu':
        cpu_events[0] += ['r11']
    else:
        cpu_events[0] += ['0x11']
    return cpu_events


def get_cpu_event_chunk(sample_config: dict, cpu_type: str) -> list:
    """
    get cpu event chunk
    :param sample_config: sample config
    :param cpu_type: cpu type
    :return: cpu events
    """
    return get_cpu_event_config(sample_config, cpu_type)


def _do_change_file_mod(file_path: str) -> None:
    files_list = []
    file_path = os.path.realpath(file_path)
    for lists in os.listdir(file_path):
        path = os.path.join(file_path, lists)
        files_list.append(path)
        if os.path.isdir(path):
            files_chmod(path)
    for data_file in files_list:
        if not os.path.isdir(data_file):
            os.chmod(data_file, NumberConstant.FILE_AUTHORITY)
        else:
            os.chmod(data_file, NumberConstant.DIR_AUTHORITY)
    os.chmod(file_path, NumberConstant.DIR_AUTHORITY)


def files_chmod(file_path: str) -> None:
    """
    change the rights of the files
    :param file_path: file path
    :return: None
    """
    try:
        if os.path.exists(file_path):
            _do_change_file_mod(file_path)
    except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
        error(MsvpCommonConst.FILE_NAME, str(err))
    finally:
        pass


def create_csv(csv_file: str, headers: list, data: list, save_old_file: bool = True) -> str:
    """
    create csv table
    :param csv_file: csv file path
    :param headers: data header for csv file
    :param data: data for csv file
    :param save_old_file: save old file or not
    :return: result for creating csv file
    """
    if not bak_and_make_dir(csv_file, save_old_file):
        return json.dumps({'status': NumberConstant.ERROR, 'info': str('bak or mkdir json dir failed'), 'data': ''})
    try:
        with os.fdopen(os.open(csv_file, Constant.WRITE_FLAGS,
                               Constant.WRITE_MODES), 'w', newline='') as _csv_file:
            os.chmod(csv_file, NumberConstant.FILE_AUTHORITY)
            f_csv = csv.writer(_csv_file)
            if headers:
                f_csv.writerow(headers)
            while NumberConstant.DATA_NUM < len(data):
                f_csv.writerows(data[:NumberConstant.DATA_NUM])
                data = data[NumberConstant.DATA_NUM:]
            f_csv.writerows(data)
        return json.dumps({'status': NumberConstant.SUCCESS, 'info': '', 'data': csv_file})
    except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
        return json.dumps({'status': NumberConstant.ERROR, 'info': str(err), 'data': ''})
    finally:
        pass


def create_json(json_file: str, headers: list, data: list, save_old_file: bool = True) -> str:
    """
    create json file
    :param json_file: json file name
    :param headers: data header for json file
    :param data: data for json file
    :param save_old_file: save old file or not
    :return: result of creating json file
    """
    json_result = []
    for each in data:
        json_result.append(OrderedDict(list(zip(headers, each))))
    if not bak_and_make_dir(json_file, save_old_file):
        return json.dumps({'status': NumberConstant.ERROR, 'info': str('bak or mkdir csv dir failed'), 'data': ''})
    try:
        with os.fdopen(os.open(json_file, Constant.WRITE_FLAGS,
                               Constant.WRITE_MODES), 'w') as _json_file:
            os.chmod(json_file, NumberConstant.FILE_AUTHORITY)
            json.dump(json_result, _json_file)
        return json.dumps({'status': NumberConstant.SUCCESS, 'info': '', 'data': json_file})
    except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
        return json.dumps({'status': NumberConstant.ERROR, 'info': str(err), 'data': ''})


def create_json_for_dict(json_file: str, dict_result: dict) -> str:
    """
    create json file for dict object
    :param json_file: json file name
    :param dict_result: dict
    :return: result of creating json file
    """
    if not bak_and_make_dir(json_file, False):
        return json.dumps({'status': NumberConstant.ERROR, 'info': str('bak or mkdir csv dir failed'), 'data': ''})
    try:
        with os.fdopen(os.open(json_file, Constant.WRITE_FLAGS,
                               Constant.WRITE_MODES), 'w') as _json_file:
            os.chmod(json_file, NumberConstant.FILE_AUTHORITY)
            json.dump(dict_result, _json_file)
        return json.dumps({'status': NumberConstant.SUCCESS, 'info': '', 'data': json_file})
    except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
        return json.dumps({'status': NumberConstant.ERROR, 'info': str(err), 'data': ''})


def bak_and_make_dir(file: str, save_old_file: bool = True) -> bool:
    json_file_back = file + ".bak"
    try:
        if os.path.exists(file):
            if save_old_file:
                shutil.move(file, json_file_back)
            else:
                os.remove(file)
    except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
        logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        return False

    try:
        if not os.path.exists(os.path.dirname(file)):
            os.makedirs(os.path.dirname(file), Constant.FOLDER_MASK)
        return True
    except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
        logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        return False


def path_check(path: str) -> str:
    """
    check existence of path
    :param path: file path
    :return:
    """
    if os.path.exists(path):
        return path
    return ""


def add_aicore_units(header: list) -> list:
    """
    add unit for ai core report headers and modify total_time to aicore_time
    :param header:
    :return: headers
    """
    new_header = []
    for item in header:
        if item == "total_time":
            item = "aicore_time"
        if "time" in item:
            item += "(us)"
        if StrConstant.BANDWIDTH in item and "(GB/s)" not in item:
            item += "(GB/s)"
        new_header.append(item)

    return new_header


def check_file_writable(path: str) -> None:
    """
    check path is file and writable
    :param path: file path
    :return: None
    """
    if path and os.path.exists(path):
        check_path_valid(path, True)
        if not os.access(path, os.W_OK):
            ReturnCodeCheck.print_and_return_status(json.dumps(
                {'status': NumberConstant.ERROR,
                 'info': "The path '%s' does not have permission to write. "
                         'Please check that the path is writeable.' % path}))


def check_dir_writable(path: str, create_dir: bool = False) -> None:
    """
    check path is dir and writable
    :param path: file path
    :param create_dir: create directory or not
    :return: None
    """
    if not os.path.exists(path) and create_dir:
        try:
            os.makedirs(path, Constant.FOLDER_MASK)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            ReturnCodeCheck.print_and_return_status(json.dumps(
                {'status': NumberConstant.ERROR,
                 'info': "Failed to create path '%s'. %s" % (path, err)}))
        finally:
            pass
    check_path_valid(path, False)
    if not os.access(path, os.W_OK):
        ReturnCodeCheck.print_and_return_status(json.dumps(
            {'status': NumberConstant.ERROR,
             'info': "The path '%s' does not have permission to write. Please "
                     'check that the path is writeable.' % path}))


def is_valid_original_data(file_name: str, project_path: str, is_conf: bool = False) -> bool:
    """
    is original data
    :param file_name: file name
    :param project_path: project path
    :param is_conf: is config file or not
    :return: result of checking original data
    """
    file_parent_path = project_path if is_conf else PathManager.get_data_dir(project_path)
    if file_name.endswith(Constant.COMPLETE_TAG) or file_name.endswith(Constant.DONE_TAG) \
            or file_name.endswith(Constant.ZIP_TAG):
        return False
    if os.path.exists(os.path.join(file_parent_path, file_name + Constant.COMPLETE_TAG)):
        return False
    return True


def float_calculate(input_list: list, operator: str = '+') -> str:
    """
    float data calculate
    :param input_list: data for calculating
    :param operator: operator
    :return: result after calculated
    """
    operator_dict = {
        StrConstant.OPERATOR_PLUS: add, StrConstant.OPERATOR_MINUS: sub,
        StrConstant.OPERATOR_MULTIPLY: mul, StrConstant.OPERATOR_DIVISOR: truediv
    }
    if operator not in operator_dict or not input_list or None in input_list:
        return str(0)
    if operator == StrConstant.OPERATOR_DIVISOR and str(input_list[1]) == "0":
        return str(0)
    new_input_list = []
    for i in input_list:
        new_input_list.append(Decimal(str(i)))
    input_list = new_input_list
    try:
        result = reduce(operator_dict.get(operator), input_list)
        return str(result)
    except (OSError, SystemError, ValueError, TypeError, RuntimeError, ArithmeticError):
        return str(0)


def is_number(float_num: any) -> bool:
    """
    check whether s is number or not
    :param float_num: number to check
    :return: is number or not
    """
    try:
        float(float_num)
        return True
    except (ValueError, TypeError):
        return False
    finally:
        pass


def is_nonzero_number(float_num: any) -> bool:
    return is_number(float_num) and not NumberConstant.is_zero(float_num)


def clear_project_dirs(project_dir: str) -> None:
    """
    remove sqlite data and complete data
    project_dir : result path
    """
    if os.path.exists(PathManager.get_sql_dir(project_dir)):
        for file_name in os.listdir(PathManager.get_sql_dir(project_dir)):
            os.remove(os.path.join(PathManager.get_sql_dir(project_dir), file_name))
    for file_name in os.listdir(PathManager.get_data_dir(project_dir)):
        if file_name.endswith(Constant.COMPLETE_TAG):
            os.remove(os.path.join(PathManager.get_data_dir(project_dir), file_name))
