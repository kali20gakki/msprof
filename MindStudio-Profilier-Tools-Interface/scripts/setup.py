#!/usr/bin/env python3
# -*- coding: utf-8 -*-
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
