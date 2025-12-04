#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
__version__ = '0.0.1'

import os
from setuptools import setup
from setuptools import find_packages

cur_path = os.path.abspath(os.path.dirname(__file__))
root_path = os.path.join(cur_path, "..")

setup(
    name="mspti",
    version=__version__,
    description="mspti desc",
    url="mspti",
    author="mspti",
    author_email="",
    license="",
    package_dir={"": root_path},
    packages=find_packages(root_path),
    include_package_data=False,
    package_data={
        "mspti": ["lib64/*"]
    },
    python_requires=">=3.7"
)
