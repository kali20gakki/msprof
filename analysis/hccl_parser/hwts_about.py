#!/usr/bin/env python
# coding=utf-8

# Copyright (c) Huawei Technologies Co., Ltd. 2017-2019. All rights reserved.
# description: hwts文件相关的解�?
# Author: c00293166
# Create: 2021-5-5

import json
import os
import re
import struct
import sys
from . import list_file


def get_log_slice_id(file_name=None):
    pattern = re.compile(r'(?<=slice_)\d+')
    slice_id = pattern.findall(file_name)
    return int(slice_id[0])


def hwts_binfile_merge(file_path):
    name_list = []
    file_join_name = ''

    if os.path.exists(file_path):
        print('hwst', file_path)
        files = os.listdir(file_path)
        for file_name in files:
            if 'slice_' in file_name and not file_name.endswith('done') and file_name.startswith('hwts'):
                name_list.append(file_name)

        name_list.sort(key=get_log_slice_id)

        file_join_name = file_path + os.sep + 'hwts.data'
        if os.path.exists(file_join_name):
            os.remove(file_join_name)

        with open(file_join_name, 'ab') as bin_data:
            for name in name_list:
                file_name = file_path + os.sep + name
                with open(file_name, 'rb') as txt:
                    bin_data.write(txt.read())

    return file_join_name


def hwts_parser(file_path):
    hwts_trace = []
    log_type = ['Start of task', 'End of task', 'Start of block', \
                'End of block', 'Block PMU']
    file_join_name = hwts_binfile_merge(file_path)
    if file_join_name:
        with open(file_join_name, 'rb') as hwts_data:
            while 1:
                _line = hwts_data.read(64)
                if _line:
                    if not _line.strip():
                        continue
                else:
                    break

                # 按照struct格式解析hwts bin文件
                _format = ['QIIIIIIIIIIII', 'QIIQIIIIIIII', 'IIIIQIIIIIIII']
                byte_first_four = struct.unpack('BBHHH', _line[0:8])
                byte_first = bin(byte_first_four[0]).replace('0b', '').zfill(8)
                _type = byte_first[-3:]
                is_warn_res0_ov = byte_first[4]
                cnt = int(byte_first[0:4], 2)
                core_id = byte_first_four[1]
                blk_id, task_id = byte_first_four[3], byte_first_four[4]
                if _type in ['000', '001']:  # log type 0,1
                    _result = struct.unpack(_format[0], _line[8:])
                    syscnt = _result[0]
                    stream_id = _result[1]
                    hwts_trace.append((log_type[int(_type, 2)], cnt, \
                                       core_id, blk_id, task_id, \
                                       syscnt, stream_id))
    else:
        print('ERROR! hwts bin file parsing error!')
    # remove中间文件
    os.remove(file_join_name)

    return hwts_trace

