#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

import os
import re
import sys
import logging

PATTERN_FUNCTION = re.compile(r'MSVP_PROF_API\s+\n+.+\w+\([^();]*\);|.+\w+\([^();]*\);')
PATTERN_RETURN = re.compile(r'([^ ]+[ *])\w+\([^;]+;')
RETURN_STATEMENTS = {
    'aclError':                 '    printf("[ERROR]: stub library cannot be'\
                                     ' used for execution, please '\
                                     'check your environment variables '\
                                     'and compilation options to make sure '\
                                     'you use the correct ACL library.\\n");\n'  \
                                     '    return static_cast<aclError>(ACL_'\
                                     'ERROR_COMPILING_STUB_MODE);',
    'aclprofConfig':            '    return nullptr;',
    'aclprofSubscribeConfig':   '    return nullptr;',
    'uint64_t':                 '    return static_cast<uint64_t>(0);',
    'size_t':                   '    return static_cast<size_t>(0);',
    'aclprofStepInfo':          '    return nullptr;',
    'void *':                     '    return nullptr;',
    'Status':                   '    return SUCCESS;',
    'aclgrphProfConfig':        '    return nullptr;',
    'void':                     ''
}


def log_info(info):
    """
    wrapper print
    """
    logging.info(info)


def collect_header_files(path):
    """
    collect header from path(ge, acl)
    """
    acl_headers = []
    ge_headers = []
    for root, _, files in os.walk(path):
        files.sort()
        for file in files:
            if file.find("acl") >= 0:
                file_path = os.path.join(root, file)
                file_path = file_path.replace('\\', '/')
                acl_headers.append(file_path)
            elif file.find("ge") >= 0:
                file_path = os.path.join(root, file)
                file_path = file_path.replace('\\', '/')
                ge_headers.append(file_path)
    return acl_headers, ge_headers


def implement_function(func):
    """
    create function by template
    """
    function_def = func[:len(func) - 1]
    function_def += '\n'
    function_def += '{\n'
    m = PATTERN_RETURN.search(func)
    if m:
        ret_type = m.group(1).strip()
        if RETURN_STATEMENTS.__contains__(ret_type):
            function_def += RETURN_STATEMENTS.get(ret_type)
        else:
            if ret_type.endswith('*'):
                function_def += '    return nullptr;'
            else:
                log_info("Unhandled return type: " + ret_type)
    else:
        function_def += '    return nullptr;'
    function_def += '\n'
    function_def += '}\n'
    return function_def


def collect_functions(file_path):
    """
    collect all function from file_path
    """
    signatures = []
    with open(file_path) as f:
        content = f.read()
        matches = PATTERN_FUNCTION.findall(content)
        for signature in matches:
            signatures.append(signature)
    return signatures


def generate_function(header_files, inc_dir):
    """
    generate cpp content
    """
    includes = []
    includes.append('#include <stdio.h>\n')
    # generate includes
    for header in header_files:
        if not header.endswith('.h'):
            continue
        include_str = '#include "{}"\n'.format(header[len(inc_dir):])
        includes.append(include_str)

    content = includes
    log_info("include concent build success")
    total = 0
    content.append('\n')
    # generate implement
    for header in header_files:
        header_base_name = os.path.basename(header)
        if not header_base_name.endswith('.h'):
            continue
        content.append("// stub for {}\n".format(header[len(inc_dir):]))
        if header_base_name == "ge_prof.h":
            content.append("namespace ge {\n")
        content.append("\n")
        functions = collect_functions(header)
        log_info("inc file:{}, functions numbers:{}".format(header, len(functions)))
        total += len(functions)
        for func in functions:
            content.append("{}\n".format(implement_function(func)))
            content.append("\n")
        if header_base_name == "ge_prof.h":
            content.append("} //ge\n")
    log_info("implement concent build success")
    log_info('total functions number is {}'.format(total))
    return content


def generate_stub_file(inc_dir):
    """
    collect header files and generate function
    """
    acl_header_files, ge_header_files = collect_header_files(inc_dir)
    log_info("header files has been generated")
    acl_content = generate_function(acl_header_files, inc_dir)
    log_info("acl_content has been generated")
    ge_content = generate_function(ge_header_files, inc_dir)
    log_info("ge_content has been generated")
    return acl_content, ge_content


def gen_code(inc_dir, acl_stub_path, ge_stub_path):
    """
    generate stub file and write to file
    """
    if not inc_dir.endswith('/'):
        inc_dir += '/'
    acl_content, ge_content = generate_stub_file(inc_dir)
    log_info("acl_content, ge_content have been generated")
    with open(acl_stub_path, mode='w') as f:
        f.writelines(acl_content)
    with open(ge_stub_path, mode='w') as f:
        f.writelines(ge_content)


if __name__ == '__main__':
    extern_inc_dir = sys.argv[1]
    acl_stub_output = sys.argv[2]
    ge_stub_output = sys.argv[3]
    gen_code(extern_inc_dir, acl_stub_output, ge_stub_output)
