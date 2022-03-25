#!usr/bin/env python
# coding:utf-8
"""
Copyright Huawei Technologies Co., Ltd. 2020. All rights reserved.
"""
from common_func.empty_class import EmptyClass


class MetaMetricManager:
    """
    metric manager
    """

    def __init__(self: any) -> None:
        self.metric_func = {}

    def register(self: any, metric_name: str, metric: any) -> None:
        """
        register metrics
        """
        self.metric_func[metric_name] = metric

    def get_metric(self: any, metric_name: str) -> any:
        """
        get metric
        """
        if metric_name not in self.metric_func:
            return EmptyClass()
        return self.metric_func.get(metric_name).get_metric(metric_name)


class OperatorMetricManager(MetaMetricManager):
    """
    save all operator metric
    compute metric with operator
    """

    def __init__(self: any) -> None:
        MetaMetricManager.__init__(self)


class NetMetricManager(MetaMetricManager):
    """
    save all operator metric
    compute metric with operator
    """

    def __init__(self: any) -> None:
        MetaMetricManager.__init__(self)
