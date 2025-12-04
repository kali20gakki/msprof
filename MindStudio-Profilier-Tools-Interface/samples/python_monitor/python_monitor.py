#!/usr/bin/python3
# coding=utf-8
#
# Copyright (C) 2025. Huawei Technologies Co., Ltd. All rights reserved.
#
# The sample demonstrates how to obtain kernel data and hccl data through the MSPTI Python API,
# and how to use MSTX dot to obtain range data
#

import os
import threading
import time
import logging
from multiprocessing import Queue

import torch
import torch_npu

from mspti import (
    KernelData,
    KernelMonitor,
    HcclData,
    HcclMonitor
)

# parser会被多线程调用，所以使用queue存储数据，保证多线程安全
data_queue = Queue()
logging.basicConfig(format='%(asctime)s - %(pathname)s[line:%(lineno)d] - %(levelname)s: %(message)s',
                    level=logging.INFO)


def kernel_parser(data: KernelData):
    data_queue.put(data)


def hccl_parser(data: HcclData):
    data_queue.put(data)


def consumer_func(consume_queue):
    while True:
        if not consume_queue.empty():
            data = consume_queue.get()
            if data is None:
                break
            if isinstance(data, KernelData):
                logging.info(f'{data.kind}, {data.start}, {data.end}, {data.device_id}, {data.stream_id}, '
                             f'{data.correlation_id}, {data.type}, {data.name}')
            elif isinstance(data, HcclData):
                logging.info(f'{data.kind}, {data.start}, {data.end}, {data.device_id}, {data.stream_id}, '
                             f'{data.bandwidth}, {data.name}, {data.comm_name}')
        else:
            time.sleep(0.1)


def init_process(backend="hccl"):
    torch.distributed.init_process_group(backend=backend, init_method='env://')


def test_monitor():
    consumer = threading.Thread(target=consumer_func, args=(data_queue, ))
    consumer.start()

    # enable mspti monitor to collect activity
    k_monitor = KernelMonitor()
    k_monitor.start(kernel_parser)
    h_monitor = HcclMonitor()
    h_monitor.start(hccl_parser)

    init_process()
    device = int(os.getenv('LOCAL_RANK'))
    torch.npu.set_device(device)

    width = 256

    x = torch.randn(width, width, dtype=torch.float16).npu()
    y = torch.randn(width, width, dtype=torch.float16).npu()

    result = x + y
    result = torch.matmul(x, y)
    torch.distributed.all_reduce(result)
    torch.npu.synchronize()

    # stop mspti monitor and consume activity
    k_monitor.stop()
    h_monitor.stop()
    data_queue.put(None)
    consumer.join()


if __name__ == "__main__":
    test_monitor()
