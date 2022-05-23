#!/usr/bin/env python
# coding=utf-8

# Copyright (c) Huawei Technologies Co., Ltd. 2017-2019. All rights reserved.
# description: HCCL profiling解析工具总入口文�?
# Author: c00293166
# Create: 2021-5-5

import os
import sys
from . import trans_hcclfile_format
from . import hwts_about
from . import export_trace
from . import list_file
from . import hccl_about


def dfx_hwts_broadcast(hwts_trace):
    for item in hwts_trace:
        if item[6] == 89 and item[4] == 3:
            print('index = ', hwts_trace.index(item), 'item = ', item)


def dfx_hwts_broadcast1(hwts_trace):
    for item in hwts_trace:
        if item[6] == 90 and item[4] == 2:
            print('index = ', hwts_trace.index(item), 'item = ', item)


def dfx_hwts_broadcast2(hwts_trace):
    for item in hwts_trace:
        if item[6] == 91 and item[4] == 2:
            print('index = ', hwts_trace.index(item), 'item = ', item)


def dfx_hwts_broadcast3(hwts_trace):
    for item in hwts_trace:
        if item[6] == 88 and item[4] == 2:
            print('index = ', hwts_trace.index(item), 'item = ', item)


# 获取 次数和次数之间的分界线，利用迭代1 �?迭代2�?task不能混插这个特�?
def get_end_from_hwts(hwts_trace, hwts_len, stid_list, index):
    flag = 0
    list_len = len(stid_list)
    i = index
    while i < hwts_len:
        for stid in stid_list:
            if item[6] == stid[0] and item[4] == stid[1] and 'End' in item[0]:
                flag = flag + 1
                break
        if flag == list_len:
            return hwts_trace.index(item)
        i = i + 1

    print('[WARN] last op short of some task')
    return -1


# 功能描述：以op类去匹配迭代轮次。匹配ar�?00，ar�?01，可能是ar1和ar2
def get_iter_serverdatas(device_id, local_rank, hwts_trace, \
                         stid_list, begin, num, ops):
    max_range = begin + num
    server_data = []
    print('hwts_trace len = ', len(hwts_trace))
    hwts_first_index_list = export_trace.find_first_time_list(hwts_trace, stid_list, max_range)
    while begin < max_range:
        hwts_first_index = hwts_first_index_list[begin]

        if hwts_first_index == -1:
            break

        # step2: 反匹配，寻找sid，tid对应的op，具体是ar1 or ar2
        ret_ops = export_trace.get_op_from_ops(hwts_trace,\
                                               hwts_first_index, ops)

        # step3: 对ops进行处理，得到devicedata
        devicedata = trans_hcclfile_format.ops_process(device_id, \
                                                       local_rank, ret_ops)

        export_trace.hccl_relate_hwts_mode_b(devicedata,\
                                             hwts_trace, hwts_first_index)
        server_data.append(devicedata)
        begin += 1

    return server_data


# 功能描述：单算子模式下对stid的匹�?
def get_iter_serverdatas_op(device_id, local_rank, hwts_trace, ops):
    server_data = []

    for op_elem in ops:
        tmp_ops = []
        tmp_ops.append(op_elem)

        devicedata = trans_hcclfile_format.ops_process(device_id, \
                                                       local_rank, tmp_ops)
        export_trace.hccl_relate_hwts_mode_op(devicedata, hwts_trace)
        server_data.append(devicedata)

    return server_data


def get_op_slice_num(elem):
    filename = elem.split(os.sep)[-1]
    return filename.split('_')[1] + filename.split('_')[2] + filename.split('_')[-1]



# 对外暴露API
# 注：op以实际的hccl prof中tag名为准，包括：\
# broadcast/allRduce/allGather/reduceScatter/reduce/all/send/recv
def hccl_parse_op(device_id, profiling_data_path, output_path,\
                  op_type='all', begin=1, num=500):
    if not (int(device_id) >= 0 and int(device_id) <= 7):
        print('[ERROR] device_id is invalid, range shoule be 0~7')
        return

    if begin > 2000 or num > 500:
        print('[ERROR] begin or num value is incorrect')
        return

    if not os.path.isdir(profiling_data_path):
        print('[ERROR] profiling_data_path is not a dir, it shoule be a dir')
        return

    if not os.path.isdir(output_path):
        print('[ERROR] output_path is not a dir, it shoule be a dir')
        return

    profiling_data_path = os.path.abspath(profiling_data_path)
    output_path = os.path.abspath(output_path)
    if profiling_data_path.find(output_path) != -1:
        print('[ERROR] output_path is not allow to the same or belong to input_path')
        return


    online_mode = 0  # 单算子标�?
    local_rank = 0   # 存储本device id在全局网络中的 rank id编号
    # step 1: 遍历目录，寻找到hccl prof文件所在路�?
    if os.path.exists(profiling_data_path):
        filelist = list_file.get_filelist_hccl(profiling_data_path)
        for file_elem in filelist:
            if 'trans' in file_elem:
                if ('dev' + device_id) in file_elem:
                    os.remove(file_elem)

        filelist = list_file.get_filelist_hccl(profiling_data_path)
        devfile_list = list_file.sel_devfile_from_filelist_hccl(device_id,\
                                                                filelist)
        devfile_list.sort(key=get_op_slice_num)
        local_rank, online_mode = hccl_about.get_localrank_and_workflow_mode(devfile_list[0])

    print('step1 done')

    # step 2: 数据预处理，对hccl prof文件进行加载，按需合并生成bin_list
    file_dir = os.path.dirname(os.path.abspath(devfile_list[0]))

    bin_list = hccl_about.bin_parser(file_dir)
    print('step2 done')

    # step 3: 对step 2 生成的文件按�?op-stream-task 三级关系进行整理
    ops = hccl_about.hccl_process(bin_list)
    print('len of ops = ', len(ops))  # check了数据正确�?
    print('step3 done')

    # 添加对单算子模式的分支处�?
    if online_mode == 1:
        ret_ops = trans_hcclfile_format.getsidtid_from_ops_opmode(\
            ops, op_type, begin, num)
        if len(ret_ops) == 0:
            print('[ERROR] at op_mode, begin is out range of iters')
            return -1

        path = list_file.get_dir_hwts(device_id, profiling_data_path)
        hwts_trace = hwts_about.hwts_parser(path)
        server_data = get_iter_serverdatas_op(device_id, local_rank, hwts_trace, ret_ops)
        export_trace.export_result(device_id, server_data, output_path)
        return 0

    # step 4: 根据给定的op获取 sid + tid的列�?
    stid_list = trans_hcclfile_format.get_sidtid_from_ops(ops, op_type)
    print('stid_list = ', stid_list)
    print('step4 done')

    # step 5: 对hwts日志进行解析
    path = list_file.get_dir_hwts(device_id, profiling_data_path)
    hwts_trace = hwts_about.hwts_parser(path)
    print('step5 done')

    server_data = get_iter_serverdatas(device_id, local_rank,\
                                       hwts_trace, stid_list, \
                                       begin, num, ops)
    # step 6: 生成trace
    export_trace.export_result(device_id, server_data, output_path)

    return

if __name__ == '__main__':
    deviceID = sys.argv[1]
    profilingDataPath = sys.argv[2]
    outputPath = sys.argv[3]

    hccl_parse_op(deviceID, profilingDataPath, outputPath)
