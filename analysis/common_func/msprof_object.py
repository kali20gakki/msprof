#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

"""
This file used for basic python type modification
"""
from typing import OrderedDict
from collections import namedtuple
from collections import OrderedDict as _OrderedDict


class HighPerfDict(OrderedDict):
    """
    this class will provide some high perf interface for dict
    """

    def set_default_call_obj_later(self: any, key: any, _class: any, *args, **kwargs):
        if key in self.keys():
            return self.get(key)
        obj = _class(*args, **kwargs)
        self.setdefault(key, obj)
        return obj


class CustomizedNamedtupleFactory:

    @staticmethod
    def generate_named_tuple_from_dto(dto_class: type, description: any) -> any:
        """
        dynamically generate namedtuple from dto class which meets requests in below:
        1. Contains all the names that sql returns
        2. If an attribute in dto has a default value and not in sql, it should be in the namedtuple
        3. Attribute that come from sql, should be placed ahead with the same order in sql
        4. A default value should be defined for all attributes
        5. dto should be in form of dataclass
        dto_class: dto class
        description: description from sql, each line describe a name from db
        """
        description_set = {i[0] for i in description}
        extend_columns = _OrderedDict()
        # get all attribute of dto class by dir()
        for item in dir(dto_class):
            # filter magic methods which start with "_"
            if not item.startswith("_") and item not in description_set and not item.isupper():
                default_value = getattr(dto_class, item)
                # whether the attribute is a function
                if callable(default_value):
                    continue
                extend_columns[item] = default_value if default_value else None
        filed_names = [i[0] for i in description]
        # place name that in dto and not in sql at the end, use extend slightly improve efficiency
        filed_names.extend(extend_columns.keys())
        defaults = [None] * len(description)
        defaults.extend(extend_columns.values())
        # use the same name as the dto, when call isinstance compare __name__
        base_tuple = namedtuple(dto_class.__name__, filed_names, defaults=defaults)
        return CustomizedNamedtupleFactory._enhance_namedtuple(base_tuple, {})

    @staticmethod
    def _enhance_namedtuple(tuple_type: type, function_dict: dict):
        """
        Enhance namedtuple, add function or attribute for it.
        tuple_type: base type
        function_dict: functions or attrs added to the type
        """
        class_namespace = dict(tuple_type.__dict__)
        class_namespace.update(function_dict)
        # rename _replace, to pass codecheck
        class_namespace["replace"] = class_namespace.get("_replace")
        return type(tuple_type.__name__, (tuple,), class_namespace)
