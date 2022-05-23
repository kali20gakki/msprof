#!/usr/bin/env python
# coding=utf-8

# Copyright (c) Huawei Technologies Co., Ltd. 2017-2019. All rights reserved.
# description: 对HCCL文件格式的处�?
# Author: c00293166
# Create: 2021-5-5

import json
import os
import re
import struct
import sys
from . import list_file


def operate_hccl_file(h_file, con_list):
    with open(h_file, "r", encoding="utf-8") as f_hccl:
        for con in f_hccl.readlines():
            if ('flag' in con) or (con[0] == ','):
                continue
            if ('end' not in con) and ('} },' not in con) and ('duration' in con):
                print('con = ', con, 'con[-1] = ', con[-1])
                con = '%s%s' % (con, ',')
            con_list.append(con.strip())


def json_turn_str_to_dict(each_op_data, op_name_tag, op_num_tag, dev_num_tag):
    op_res = json.loads(each_op_data)
    op_res["traceEvents"] = \
        op_res.pop("%s_%s-%s" % (op_name_tag, op_num_tag, dev_num_tag))
    final_op_res = json.dumps(op_res)
    return final_op_res


def get_workflow_mode(h_file):
    with open(h_file, "r", encoding="utf-8") as f_hccl:
        for con in f_hccl.readlines():
            if 'flag' in con:
                if 'workFlowMode' in con:
                    online_mode = int(con.split(':')[-1][0])
                    return online_mode
    return 0


def get_localrank_id(h_file):
    with open(h_file, "r", encoding="utf-8") as f_hccl:
        for con in f_hccl.readlines():
            if 'flag' in con:
                if 'localRank' in con:
                    num_list = re.findall(r'\d+', con.split(':')[-2])
                    local_rank = int(num_list[0])
                    return local_rank
    return 0


# 添加�?hccl 3w task切分1个文件，第二个文件开�?逗号的问�?要求文件排序ok，如：AR1_0,AR1_1,br1_0,br1_1
def trans_hccl_file(devfile_list):
    conn_list = []
    for h_file in devfile_list:
        operate_hccl_file(h_file, conn_list)
    if len(conn_list) == 0:
        print('[ERROR] load hccl file to string list fail')
        return -1

    con_string = "".join(conn_list).replace("}{", "}\n{")
    data_list = con_string.split("\n")
    h_file_dir = os.path.dirname(os.path.abspath(h_file))
    for each_op_data in data_list[::-1]:
        op_name_tag = \
            each_op_data[each_op_data.find("hcom_"): each_op_data.rfind("_")]
        op_num_tag = \
            each_op_data[each_op_data.rfind("_") + 1: each_op_data.find("-")]
        dev_num_tag = \
            each_op_data[each_op_data.find("-") + 1: \
                each_op_data.find("-") + 2]
        new_file = '%s%s%s%s%s%s%s%s' % (h_file_dir, os.sep, op_name_tag, \
            '_', op_num_tag, '_dev', dev_num_tag, '_0.trans')
        ret_json = json_turn_str_to_dict(each_op_data, op_name_tag, \
                                         op_num_tag, dev_num_tag)
        with open(new_file, "w", encoding="utf-8") as f_write:
            f_write.write(ret_json)


def getkey(op_elem):
    return int(op_elem['tag'].split('_')[2])


def getseq(op_elem):
    return int(op_elem['tag'].split('_')[3])

def get_sidtid_from_ops(ops, ar_op):
    stid_list = []
    ops.sort(key=getkey)

    if ar_op == 'all':
        for op_elem in ops:
            op_list = []
            stream_nums = len(op_elem['streams'])
            for stream in op_elem['streams']:
                for task in stream['tasks']:
                    if 'stage' in task['name']:
                        continue
                    sid = stream['stream_id']
                    tid = task['args']['task_id']
                    stid = [sid, tid]
                    op_list.append(stid)
                    break
            stid_list.append([op_list, 0, stream_nums])

        return stid_list

    else:
        for op_elem in ops:
            if ar_op in op_elem['tag']:
                op_list = []
                stream_nums = len(op_elem['streams'])
                for stream in op_elem['streams']:
                    for task in stream['tasks']:
                        if 'stage' in task['name']:
                            continue
                        sid = stream['stream_id']
                        tid = task['args']['task_id']
                        stid = [sid, tid]
                        op_list.append(stid)
                        break
                stid_list.append([op_list, 0, stream_nums])
        return stid_list


# 单算子模式的处理
def getsidtid_from_ops_opmode(ops, ar_op, begin, num):
    ret_ops = []

    if ar_op == 'all':
        ops.sort(key=getseq)
        number = 0
        for op_elem in ops:
            number += 1
            if number >= begin and number < begin + num:
                ret_ops.append(op_elem)

    else:
        ops.sort(key=getkey)
        number = 0
        for op_elem in ops:
            if ar_op in op_elem['tag']:
                number += 1
                if number >= begin and number < begin + num:
                    ret_ops.append(op_elem)

    return ret_ops


# ops_process, 功能：删除task序列�?stage0-step1这种task，方便MI的人解析
# 参数描述�?ops->hccl_process得到的结构体
class DeviceData:

    __slots__ = ('device_id', 'stream')

    def __init__(self, device_id):
        self.device_id = device_id
        self.stream = []

    def add_stream(self, stream):
        self.stream.append(stream)


class Stream:

    __slots__ = ('stream_id', 'stream_type', 'stream_info', 'task')

    def __init__(self, stream_id, stream_info=None):
        self.stream_id = stream_id
        self.stream_type = 'HCCL'
        self.stream_info = stream_info
        self.task = []

    def add_task(self, task):
        self.task.append(task)


class Task:

    __slots__ = (
    'task_type', 'task_id', 'stage', 'step', 'src', 'dst', \
    'notify_id', 'plane_id', 'datasize', \
    'channel_type', 'op_type', 'task_record', 'name', \
    'src_rank', 'dst_rank', 'transport_type')

    def __init__(self, task_type, task_id=None, stage=None, step=None, \
                 src=None, dst=None, notify_id=None,\
                 plane_id=None, channel_type='NULL', datasize=None, \
                 op_type=None, name=None, src_rank=None,\
                 dst_rank=None, transport_type=None):
        self.task_type = task_type
        self.task_id = task_id
        self.plane_id = plane_id
        self.stage = stage
        self.step = step
        self.src = src
        self.dst = dst
        self.notify_id = notify_id
        self.datasize = datasize
        self.channel_type = channel_type
        self.op_type = op_type
        self.task_record = []
        self.name = name
        self.src_rank = src_rank
        self.dst_rank = dst_rank
        self.transport_type = transport_type

    def cal_task_record_bandwith(self):
        for each_task in self.task_record:
            each_task.bandwith = self.datasize / each_task.duration

    def add_task_record(self, task_record):
        self.task_record.append(task_record)


def ops_process(deviceid, local_rankid, ops):
    devicedata = DeviceData(deviceid)

    op_elem = ops[0]
    optag = op_elem['tag']
    end_index = optag.rfind('_')
    optype = optag[len('hcom_'):end_index]

    for streamindex in op_elem['streams']:
        stream_id = streamindex['stream_id']
        planeid = streamindex['plane_id']
        stream_info = str(planeid)
        tmpstream = Stream(stream_id, stream_info)
        taskslist = streamindex['tasks']
        for taskindex in taskslist:
            taskname = taskindex['name']
            if "stage" in taskname:
                continue
            args = taskindex['args']

            local_rank = local_rankid
            remote_rank = args['remote_rank']
            srcrank = local_rank
            dstrank = remote_rank
            if args['role'] == 'dst':
                srcrank = remote_rank
                dstrank = local_rank
            if args['role'] == 'src':
                srcrank = local_rank
                dstrank = remote_rank

            if args['task_type'] == 5 or args['task_type'] == 0 or\
                            args['task_type'] == 16:
                tmptask = Task(taskindex['name'], task_id=args['task_id'],\
                               stage=args['stage'], step=args['step'], \
                               src=args['src'], dst=args['dst'], \
                               channel_type=args['link_type'],\
                               datasize=args['size'], notify_id=0, \
                               plane_id=planeid, op_type=optype, \
                               name=taskname, src_rank=srcrank, \
                               dst_rank=dstrank, \
                               transport_type=args['transport_type'])
            elif args['task_type'] == 14 or args['task_type'] == 15:
                tmptask = Task(taskindex['name'], task_id=args['task_id'],\
                               stage=args['stage'], step=args['step'],\
                               notify_id=args['notify_id'], plane_id=planeid,\
                               op_type=optype, name=taskname, \
                               src_rank=srcrank, dst_rank=dstrank, \
                               transport_type=args['transport_type'])
            else:
                tmptask = Task(taskindex['name'], task_id=args['task_id'],
                               stage=args['stage'], step=args['step'], \
                               src='NULL', dst='NULL', channel_type='Null', \
                               datasize='NULL', notify_id=args['notify_id'], \
                               plane_id=planeid, op_type=optype, \
                               name=taskname)

            tmpstream.add_task(tmptask)

        devicedata.add_stream(tmpstream)

    return devicedata

