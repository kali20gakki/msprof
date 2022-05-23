#!/usr/bin/env python
# coding=utf-8

# Copyright (c) Huawei Technologies Co., Ltd. 2017-2019. All rights reserved.
# description: trace文件导出
# Author: c00293166
# Create: 2021-5-5

import sys
import os
import re
import json
import shutil


def getlist_from_hwts(hwts_trace_items, stid_list):
    hwts_task_trace_stream = []
    for hwts_tuple in hwts_trace_items:
        for stid in stid_list:
            if stid[0] == hwts_tuple[4] and stid[1] == hwts_tuple[6]:
                hwts_task_trace_stream.append(hwts_tuple)

    return hwts_task_trace_stream


# find_first_time 功能：按照最终方案，以AR为例，使用AR1和AR2的stid_list寻找hwts中出现begin次的时刻
def find_first_time(hwts_trace_items, stid_list, begin_num):
    iter_num = 1

    for hwts_tuple in hwts_trace_items:
        for op_list in stid_list:
            for stid in op_list[0]:
                if stid[0] == hwts_tuple[6] and stid[1] == hwts_tuple[4]\
                        and 'Start' in hwts_tuple[0]:
                    if op_list[1] % op_list[2] == 0:
                        if iter_num == begin_num:
                            return hwts_trace_items.index(hwts_tuple)
                        iter_num = iter_num + 1
                    op_list[1] += 1

    print('hwts_len = ', len(hwts_trace_items), 'begin_num = ', begin_num)
    print('[WARN]hwts_trace num is not enough to input num')
    return -1


def find_first_time_list(hwts_trace_items, stid_list, max_num):
    id_list = [-1] * max_num
    iter_num = 0

    for i_hwts in range(0, len(hwts_trace_items)):
        hwts_tuple = hwts_trace_items[i_hwts]
        for op_list in stid_list:
            for stid in op_list[0]:
                if stid[0] == hwts_tuple[6] and stid[1] == hwts_tuple[4] \
                        and 'Start' in hwts_tuple[0]:
                    if op_list[1] % op_list[2] == 0:
                        iter_num += 1
                        id_list[iter_num] = i_hwts
                        if iter_num == max_num - 1:
                            return id_list
                    op_list[1] += 1

    print('hwts_len = ', len(hwts_trace_items), 'max_num = ', max_num)
    print('[WARN]hwts_trace num is not enough to input num')
    return id_list


# find_first_time 功能：寻找给定 streamid，taskid的end task 在begin num的下标
def find_first_time_end(hwts_trace_items, sid, tid, begin_num):
    iter_num = 1

    for hwts_tuple in hwts_trace_items:
        if sid == hwts_tuple[6] and tid == hwts_tuple[4]\
                and 'End' in hwts_tuple[0]:
            if iter_num == begin_num:
                return hwts_trace_items.index(hwts_tuple)
            iter_num = iter_num + 1

    print('[WARN]hwts_trace num is not enough to input num')
    return -1


# get_op_from_ops 功能：根据返回的hwts index中的sid，tid，索引到具体的op
def get_op_from_ops(hwts_trace_items, index, ops):
    sid = hwts_trace_items[index][6]
    tid = hwts_trace_items[index][4]
    ret_ops = []

    for op_elem in ops:
        for stream in op_elem['streams']:
            stream_id = stream['stream_id']
            for task in stream['tasks']:
                if 'stage' in task['name']:
                    continue
                task_id = task['args']['task_id']
                if sid == stream_id and tid == task_id:
                    ret_ops.append(op_elem)
                    return ret_ops

    return ret_ops


#这里有个偏差：老工具使用devicedata，包括了所有的op，现在的要求是针对给定op
class TaskRecord:

    __slots__ = ('iteration_num', 'start_ts', 'end_ts', \
                 'duration', 'bandwidth')

    def __init__(self, iteration_num, start_ts, end_ts='NULL', bandwidth=None):
        self.iteration_num = iteration_num
        self.start_ts = start_ts
        self.end_ts = end_ts
        if end_ts == 'NULL':
            self.duration = 'NULL'
        else:
            self.duration = end_ts - start_ts
        self.bandwidth = bandwidth

    def cal_bandwidth(self, datasize):
        if datasize is not None and self.duration != 'NULL':
            if self.duration == 0:
                self.duration = 100
            self.bandwidth = round(datasize / 1000.0 / (self.duration / 100.0), 2)
        else:
            self.bandwidth = 'NULL'


# mode B: 针对给定的op，从指定的index开始，遍历每个task，给每个task赋值hwts的时间戳
def hccl_relate_hwts_mode_b(devicedata, hwts_trace_items, index):
    hwts_len = len(hwts_trace_items)
    less = 0

    for stream in devicedata.stream:
        i = index
        for task in stream.task:
            if 'stage' in task.name:
                continue
            flag = 0
            start_ts = end_ts = 0xffffffff
            i = index
            while i < hwts_len:
                if hwts_trace_items[i][6] == stream.stream_id and \
                                hwts_trace_items[i][4] == task.task_id and \
                                'Start' in hwts_trace_items[i][0]:
                    start_ts = hwts_trace_items[i][5]
                    flag += 1
                    i += 1
                    continue
                if hwts_trace_items[i][6] == stream.stream_id and \
                                hwts_trace_items[i][4] == task.task_id and \
                                'End' in hwts_trace_items[i][0]:
                    end_ts = hwts_trace_items[i][5]
                    flag += 1
                    i += 1
                    continue

                if flag == 2:
                        tmp_task_record = TaskRecord(0, start_ts, end_ts)
                        tmp_task_record.cal_bandwidth(task.datasize)
                        task.add_task_record(tmp_task_record)
                        break

                i += 1

            if i == hwts_len:
                print('######## stream_id = ', stream.stream_id, \
                      'task_id = ', task.task_id, 'flag = ', flag)
                if less == 0:
                    print('[WARN] hwts task less than actual task num')
                    less = 1
                if end_ts == 0xffffffff:
                    tmp_task_record = TaskRecord(0, start_ts, end_ts)
                    tmp_task_record.cal_bandwidth(task.datasize)
                    task.add_task_record(tmp_task_record)
                break


# 单算子模式在hwts中的task搜索
def hccl_relate_hwts_mode_op(devicedata, hwts_trace_items):
    hwts_len = len(hwts_trace_items)
    less = 0

    for stream in devicedata.stream:
        i = 0
        for task in stream.task:
            if 'stage' in task.name:
                continue
            flag = 0
            start_ts = end_ts = 0xffffffff
            while i < hwts_len:
                if hwts_trace_items[i][6] == stream.stream_id and \
                                hwts_trace_items[i][4] == task.task_id and \
                                'Start' in hwts_trace_items[i][0]:
                    start_ts = hwts_trace_items[i][5]
                    flag += 1
                    i += 1
                    continue
                if hwts_trace_items[i][6] == stream.stream_id and \
                                hwts_trace_items[i][4] == task.task_id and \
                                'End' in hwts_trace_items[i][0]:
                    end_ts = hwts_trace_items[i][5]
                    flag += 1
                    i += 1
                    continue

                if flag == 2:
                        tmp_task_record = TaskRecord(0, start_ts, end_ts)
                        tmp_task_record.cal_bandwidth(task.datasize)
                        task.add_task_record(tmp_task_record)
                        break

                i += 1

            if i == hwts_len:
                print('######## stream_id = ', stream.stream_id, \
                      'task_id = ', task.task_id, 'flag = ', flag)
                if less == 0:
                    print('[WARN] hwts task less than actual task num')
                    less = 1
                if end_ts == 0xffffffff:
                    tmp_task_record = TaskRecord(0, start_ts, end_ts)
                    tmp_task_record.cal_bandwidth(task.datasize)
                    task.add_task_record(tmp_task_record)


# export_result 功能描述:生成最终的json文件
def export_result(deviceid, server_data, out_path):
    if os.path.exists(out_path):
        shutil.rmtree(out_path)
        os.mkdir(out_path)

    # 第一轮循环表示 begin到num有多少轮次，server_data中就有多少devicedata
    i = 1
    for devicedata in server_data:
        trace_events = []
        for stream in devicedata.stream:
            for task in stream.task:
                planeid = task.plane_id
                deviceid = devicedata.device_id
                if len(task.task_record) > 0:
                    start_ts = task.task_record[0].start_ts / 100.00
                    duration = task.task_record[0].duration / 100.00
                    bandwidth = task.task_record[0].bandwidth
                name = task.name
                notifyid = task.notify_id
                stage = task.stage
                step = task.step
                streamid = stream.stream_id
                taskid = task.task_id
                tasktype = task.task_type
                src_rank = task.src_rank
                dst_rank = task.dst_rank
                transport_type = task.transport_type
                size = task.datasize

                trace_events.append({
                    "tid": planeid,
                    "pid": deviceid,
                    "ts": start_ts,
                    "dur": duration,
                    "ph": "X",
                    "name": name,
                    "args": {
                        "notify id": notifyid,
                        "duration estimated": duration,
                        "stage": stage,
                        "step": step,
                        "bandwidth": bandwidth,
                        "stream id": streamid,
                        "task id": taskid,
                        "task type": tasktype,
                        "src rank": int(src_rank),
                        "dst rank": int(dst_rank),
                        "transport type": transport_type,
                        "size": size
                    }
                })
        tag = devicedata.stream[0].task[0].op_type
        out_dict = {"device id": deviceid, "iteration": i, \
                    "traceEvents": trace_events}
        json_content = json.dumps(out_dict)
        file_name = '%s%s%s%s' % (tag, os.sep, 'iter', str(i))
        i = i + 1
        if not out_path.endswith('/'):
            out_path = '%s%s' % (out_path, '/')
        file_path = out_path + tag
        if not os.path.exists(file_path):
            os.makedirs(file_path)
        ret_path = '%s%s%s' % (out_path, file_name, '.trace')
        with open(ret_path, 'w') as fout:
            fout.write(json_content)

        print('[info]生成的trace文件所在路径是：', ret_path)
