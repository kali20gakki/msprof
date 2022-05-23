#!/usr/bin/python3
# coding: utf-8
"""
This script is used to provide for time line page
Copyright Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
"""
import collections
import json

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.msvp_common import float_calculate
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant


def calculate_task_status_time(task: list, delta: float) -> tuple:
    """
    calculate task status time
    """
    wait = {"start": float_calculate([float(task[2]) / NumberConstant.NANO_SECOND, delta]),
            "end": float_calculate([float(task[4]) / NumberConstant.NANO_SECOND, delta]),
            "status": "waiting", "rowId": task[-1]}
    complete = {"start": float_calculate([float(task[4]) / NumberConstant.NANO_SECOND, delta]),
                "end": float_calculate([float(task[-2]) / NumberConstant.NANO_SECOND, delta]),
                "status": "running", "rowId": task[-1]}
    return wait, complete


def get_result_data_for_dvpp(res: list, delta: float) -> dict:
    """
    method that provides resulta for get_dvpp_total_data
    """
    data_time = collections.OrderedDict()
    proc_util_ = []
    all_util_ = []
    proc_time_ = []
    last_time_ = []
    proc_frame_ = []
    last_frame_ = []
    for i, _ in enumerate(res):
        proc_time_.append(tuple(float_calculate([res[i][0], delta]),
                                res[i][1]))
        last_time_.append(tuple(float_calculate([res[i][0], delta]),
                                res[i][2]))
        proc_frame_.append(tuple(float_calculate([res[i][0], delta]),
                                 res[i][3]))
        last_frame_.append(tuple(float_calculate([res[i][0], delta]),
                                 res[i][4]))
        # change proc_util to percentage
        per1 = float(res[i][5].split("%")[0])
        per1 = round(per1 / NumberConstant.PERCENTAGE, 3)
        proc_util = (float_calculate([res[i][0], delta]), per1)
        proc_util_.append(proc_util)
        # change all_util to percentage
        per2 = float(res[i][6].split("%")[0])
        per2 = round(per2 / NumberConstant.PERCENTAGE, 3)
        all_util_.append((float_calculate([res[i][0], delta]), per2))

    data_time['proc_time'] = proc_time_
    data_time['last_time'] = last_time_
    data_time['proc_frame'] = proc_frame_
    data_time['last_frame'] = last_frame_
    data_util = {"proc_utilization": proc_util_, "all_utilization": all_util_}

    return {"time and frame": data_time, "utilization": data_util}


def get_dvpp_total_data(param: dict, conn: any, delta_dev: float) -> dict:
    """
    provides data for get_dvpp_timeline method
    """
    curs = conn.cursor()
    dvpp_typw_name = ['VDEC', 'JPEGD', 'PNGD', 'JPEGE', 'VPC', 'VENC']
    # get time interval
    count = curs.execute("select count(1) from DvppOriginalData "
                         "where enginetype=? and engineid=? "
                         "and timestamp between ? and ? and device_id=?",
                         (dvpp_typw_name.index(param['enginetype']), param['engineid'],
                          param['start_time'] - delta_dev, param['end_time'] - delta_dev,
                          param['device_id'])).fetchone()
    # get timestamp of the desired data
    if count[0] < param['number']:
        res = curs.execute("select timestamp, proctime/1, lasttime/1, "
                           "procframe/1, lastframe/1, procutilization, "
                           "allutilization from DvppOriginalData "
                           "where enginetype=? and engineid=? "
                           "and timestamp between ? and ? and device_id=?",
                           (dvpp_typw_name.index(param['enginetype']), param['engineid'],
                            param['start_time'] - delta_dev, param['end_time'] - delta_dev,
                            param['device_id'])).fetchall()
    else:
        group_number = 0
        if not NumberConstant.is_zero(param['number']):
            group_number = count[0] / param['number']
        res = curs.execute("select timestamp, proctime/1, lasttime/1, "
                           "procframe/1, lastframe/1, procutilization, "
                           "allutilization from DvppOriginalData "
                           "where rowid-(rowid/?)*?=0 and enginetype=? and engineid=? "
                           "and timestamp between ? and ? and device_id=?",
                           (group_number, group_number,
                            dvpp_typw_name.index(param['enginetype']), param['engineid'],
                            param['start_time'] - delta_dev, param['end_time'] - delta_dev,
                            param['device_id'])).fetchall()
    return get_result_data_for_dvpp(res, delta_dev)


def get_dvpp_timeline(param: dict) -> str:
    """
    function that provides dvpp data searched from peripheral.db
    """
    conn, curs = DBManager.check_connect_db(param.get('project_path', ''), DBNameConstant.DB_PERIPHERAL)
    delta_dev = InfoConfReader().get_delta_time()
    if not (conn and curs):
        return json.dumps({'status': NumberConstant.ERROR, "info": "The db doesn't exist!"})
    counter_exist = DBManager.judge_table_exist(curs, "DvppOriginalData")
    if not counter_exist:
        return json.dumps({'status': NumberConstant.ERROR, "info": "The table doesn't exist."})
    try:
        data_total = get_dvpp_total_data(param, conn, delta_dev)
    except (OSError, SystemError, ValueError, TypeError, RuntimeError):
        return json.dumps({"status": NumberConstant.ERROR, "info": "No data is collected."})
    else:
        if data_total:
            return json.dumps({"status": NumberConstant.SUCCESS, "info": "", "data": data_total})
        return json.dumps({"status": NumberConstant.ERROR, "info": "No data is collected."})
    finally:
        DBManager.destroy_db_connect(conn, curs)


def get_result_data_for_nic(res: list, count: int) -> dict:
    """
    method that provides resulta for get_nic_total_data
    """
    rev_data, send_data = collections.OrderedDict(), collections.OrderedDict()
    rx_band, rx_packet, rx_error, rx_dropp = [], [], [], []
    tx_band, tx_packet, tx_error, tx_dropp = [], [], [], []
    for i, _ in enumerate(res):
        # receive data metric
        rx_band.append([res[i][0], res[i][1]])
        rx_packet.append([res[i][0], res[i][2]])
        rx_error.append([res[i][0], res[i][3]])
        rx_dropp.append([res[i][0], res[i][4]])
        # send data metrics
        tx_band.append([res[i][0], res[i][5]])
        tx_packet.append([res[i][0], res[i][6]])
        tx_error.append([res[i][0], res[i][7]])
        tx_dropp.append([res[i][0], res[i][8]])

    rev_data['Rx Bandwith efficiency(%)'] = rx_band
    rev_data['rxDropped rate'] = rx_dropp
    rev_data['rxError rate'] = rx_error
    rev_data['rxPacket/s'] = rx_packet

    send_data['Tx Bandwith efficiency(%)'] = tx_band
    send_data['txDropped rate'] = tx_dropp
    send_data['txError rate'] = tx_error
    send_data['txPacket/s'] = tx_packet
    return {"total_count": count, "receive": rev_data, "send": send_data}


def get_nic_total_data(param: dict, conn: any) -> dict:
    """
    provides data for get_nic_timeline method
    """
    curs = conn.cursor()
    # get time interval
    count = curs.execute("select count(*) from NicReceiveSend "
                         "where timestamp between ? and ? and device_id=?", (param['start_time'], param['end_time'],
                                                                             param['device_id'])).fetchone()
    # get timestamp of the desired data
    if count[0] < param['number']:
        res = curs.execute("select timestamp, rx_bandwidth_efficiency, rx_packets, "
                           "rx_error_rate, rx_dropped_rate, tx_bandwidth_efficiency, "
                           "tx_packets, tx_error_rate, tx_dropped_rate from NicReceiveSend "
                           "where timestamp between ? and ? and device_id=?", (param['start_time'], param['end_time'],
                                                                               param['device_id'])).fetchall()
    else:
        group_number = 0
        if not NumberConstant.is_zero(param['number']):
            group_number = count[0] / param['number']
        res = curs.execute("select timestamp, rx_bandwidth_efficiency, rx_packets, "
                           "rx_error_rate, rx_dropped_rate, tx_bandwidth_efficiency, "
                           "tx_packets, tx_error_rate, tx_dropped_rate from NicReceiveSend "
                           "where rowid-(rowid/?)*?=0 and timestamp between ? and ? "
                           "and device_id=?", (group_number, group_number,
                                               param['start_time'], param['end_time'],
                                               param['device_id'])).fetchall()
    return get_result_data_for_nic(res, count[0])


def get_nic_timeline(param: dict) -> str:
    """
    function that provides nic data searched from nicreceivesend_table.db
    """
    conn, curs = DBManager.check_connect_db(param['project_path'], DBNameConstant.DB_NIC_RECEIVE)
    if not (conn and curs):
        return json.dumps({'status': NumberConstant.ERROR, "info": "The db doesn't exist!"})
    counter_exist = DBManager.judge_table_exist(curs, "NicReceiveSend")
    if not counter_exist:
        return json.dumps({'status': NumberConstant.ERROR, "info": "The table doesn't exist."})
    try:
        data_total = get_nic_total_data(param, conn)
    except (OSError, SystemError, ValueError, TypeError, RuntimeError):
        return json.dumps({"status": NumberConstant.ERROR, "info": "No data is collected."})
    else:
        if data_total:
            return json.dumps({"status": NumberConstant.SUCCESS, "info": "", "data": data_total})
        return json.dumps({"status": NumberConstant.ERROR, "info": "No data is collected."})
    finally:
        DBManager.destroy_db_connect(conn, curs)
