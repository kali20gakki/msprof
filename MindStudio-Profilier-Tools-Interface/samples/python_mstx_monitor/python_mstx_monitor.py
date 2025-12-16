#!/usr/bin/python3
# coding=utf-8
# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

import os
import threading
import time
import logging
from multiprocessing import Queue

import torch
import torch_npu

from mspti import (
    MstxMonitor,
    MarkerData,
    RangeMarkerData
)

# parser会被多线程调用，所以使用queue存储数据，保证多线程安全
data_queue = Queue()

logging.basicConfig(format='%(asctime)s - %(pathname)s[line:%(lineno)d] - %(levelname)s: %(message)s',
                    level=logging.INFO)


def range_mark_parser(data: RangeMarkerData):
    data_queue.put(data)


def instance_mark_parser(data: MarkerData):
    data_queue.put(data)


def consumer_func(consume_queue):
    while True:
        if not consume_queue.empty():
            data = consume_queue.get()
            if data is None:
                break
            if isinstance(data, RangeMarkerData):
                logging.info(f'{data.kind}, {data.source_kind}, {data.id}, {data.name}, {data.domain}, {data.start}, '
                             f'{data.end}, {data.object_id.process_id}, {data.object_id.thread_id}, '
                             f'{data.object_id.stream_id}, {data.object_id.device_id}')
        else:
            time.sleep(0.1)


def test_monitor():
    consumer = threading.Thread(target=consumer_func, args=(data_queue, ))
    consumer.start()
    # mspti monitor开启打点类数据采集
    m_monitor = MstxMonitor()
    # 传入瞬时打点和range打点消费方法
    m_monitor.start(instance_mark_parser, range_mark_parser)

    device = int(os.getenv('LOCAL_RANK'))
    torch.npu.set_device(device)

    width = 256

    x = torch.randn(width, width, dtype=torch.float16).npu()
    y = torch.randn(width, width, dtype=torch.float16).npu()

    stream = torch_npu.npu.current_stream()
    range_id = torch_npu.npu.mstx.range_start("mstx_matmul", stream)

    result = x + y
    result = torch.matmul(x, y)
    torch_npu.npu.mstx.range_end(range_id)

    torch.npu.synchronize()

    m_monitor.stop()
    data_queue.put(None)
    consumer.join()


if __name__ == "__main__":
    test_monitor()
