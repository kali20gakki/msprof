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
import os
from setuptools import setup
from setuptools import find_packages

try:
    from packaging.version import Version, InvalidVersion
    __version__ = os.environ.get("WHL_VERSION", "0.0.1")
    try:
        # Verify whether the version number complies with PEP 440
        Version(__version__)
        print(f"Using version: {__version__}")
    except InvalidVersion as e:
        print(f"Warning: Version '{__version__}' is invalid: {e}. Using default '0.0.1'")
        __version__ = "0.0.1"
except ImportError:
    # If there is no packaging library, use the simplified verification method
    print("Warning: packaging module not available, using version as-is")
    __version__ = os.environ.get("WHL_VERSION", "0.0.1")
except Exception as e:
    print(f"Error: Failed to process version, using default '0.0.1'. Error: {e}")
    __version__ = "0.0.1"

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
