#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2022. All rights reserved.
import os
from setuptools import setup
from setuptools import find_packages

__version__ = '0.0.1'
cur_path = os.path.abspath(os.path.dirname(__file__))
root_path = os.path.join(cur_path, "../")


def generate_path_list_of_whl_package(prefix_dir: str = ""):
    path_list = [
        "analysis.mscalculate.ge", "analysis.msconfig", "analysis.msmodel.hardware", "analysis.msmodel.hccl", \
        "analysis.msmodel.msproftx", "analysis.msmodel.runtime", "analysis.msparser.step_trace.helper", \
        "analysis.profiling_bean.ge", "analysis.profiling_bean.hardware", "analysis.profiling_bean.helper", \
        "analysis.viewer.association"
    ]
    return find_packages(prefix_dir) + path_list


setup(
    name="msprof",
    version=__version__,
    description="msprof desc",
    url="msprof",
    author="msprof",
    author_email="",
    license="",
    package_dir={"": root_path},
    packages=generate_path_list_of_whl_package(root_path),
    include_package_data=False,
    package_data={
        "": ["*.json"]
    },
    python_requires=">=3.7"
)
