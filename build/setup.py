#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2022. All rights reserved.
import os
from setuptools import setup
from setuptools import find_packages

__version__ = '0.0.1'
cur_path = os.path.abspath(os.path.dirname(__file__))
root_path = os.path.join(cur_path, "../")


setup(
    name="msprof",
    version=__version__,
    description="msprof desc",
    url="msprof",
    author="msprof",
    author_email="",
    license="",
    package_dir={"": root_path},
    packages=find_packages(root_path),
    include_package_data=False,
    package_data={
        "": ["*.json"],
        "analysis": ["lib64/*"]
    },
    python_requires=">=3.7"
)
