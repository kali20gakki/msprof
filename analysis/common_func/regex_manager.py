#!/usr/bin/python3
# coding=utf-8
"""
Copyright Huawei Technologies Co., Ltd. 2020. All rights reserved.
"""


class RegexManagerConstant:
    """
    regex manager constant class
    """
    # check the expression inner bracket.
    REGEX_INNER_BRACKET = r'\([^()]+\)'
    # check the expression connected by && or ||.
    REGEX_CONNECTED = r'\w+\.?\w+\s*[&|]{2}\s*\w+\.?\w+'
    # check the expression split by && or ||.
    REGEX_SPLIT_CONNECTED = r'[&|() ]'
    # check the expression and get comparator symbols.
    REGEX_COMPARATOR_SYMBOLS = r'\w|\.| '
    # check minus with operate symbol.
    REGEX_CALCULATE_OPERATOR = r'\+-|\+\+|--|-\+'
    # check and filter low level operate symbol.
    REGEX_HIGH_OPERATOR = r'\d|\.|\+|-| '
    # check and filter number.
    REGEX_LOW_OPERATOR = r'\d|\.| '

    # calculate multiply for match of num*num
    REGEX_CALCULATE_MULTIPLY = r'(\d+\.?\d*\*-?\d+\.?\d*)'
    # calculate division for match of num/num
    REGEX_CALCULATE_DIVISION = r'(\d+\.?\d*/-?\d+\.?\d*)'
    # calculate remainder for match of num%num
    REGEX_CALCULATE_REMAINDER = r'(\d+\.?\d*%-?\d+\.?\d*)'
    # calculate SrcAnd for match of num&num
    REGEX_CALCULATE_SRCAND = r'(\d+\.?\d*&-?\d+\.?\d*)'
    # calculate addition for match of num+num
    REGEX_CALCULATE_ADDITION = r'(\d+\.?\d*\+-?\d+\.?\d*)'
    # calculate subtraction for match of num-num
    REGEX_CALCULATE_SUBTRACTION = r'(\d+\.?\d*--?\d+\.?\d*)'

    # slice for slice_num
    REGEX_SLICE = r'(slice_\d+)'
    REGEX_NUM = r'(\d+)'

    def get_regex_manager_class_name(self: any) -> any:
        """
        get regex manager class name
        """
        return self.__class__.__name__

    def get_regex_manager_class_member(self: any) -> any:
        """
        get regex manager class member num
        """
        return self.__dict__
