#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import logging

from msparser.cluster.meta_parser import MetaParser
from common_func.ms_constant.number_constant import NumberConstant
from common_func.constant import Constant


class CriticalPathParser(MetaParser):
    """
    critical path parser
    """
    def __init__(self, ge_op_events: list, hccl_op_events: dict) -> None:
        self.ge_op_events = ge_op_events
        self.hccl_op_events = hccl_op_events
        self.events = []

    @staticmethod
    def get_event_dict_by_pid_tid(timeline_data: list) -> dict:
        event_dict = {}
        for event in timeline_data:
            identify = f"{event.get(Constant.TID)}_{event.get(Constant.TASK_TYPE)}"
            if identify not in event_dict.keys():
                event_dict[identify] = [event]
            else:
                event_dict[identify].append(event)
        return event_dict

    @staticmethod
    def get_pre_event_in_same_stream(cur_event: dict, event_dict: dict) -> dict:

        cur_identify = f"{cur_event.get(Constant.TID)}_{cur_event.get(Constant.TASK_TYPE)}"
        cur_stream_event = event_dict.get(cur_identify)
        sorted_cur_stream_event = sorted(cur_stream_event, key=lambda s: float(s[Constant.TS]), reverse=True)
        cur_idx = len(sorted_cur_stream_event)
        for idx, event in enumerate(sorted_cur_stream_event):
            if event[Constant.TS] == cur_event[Constant.TS]:
                cur_idx = idx
                break
        pre_idx = cur_idx + 1
        return sorted_cur_stream_event[pre_idx] if pre_idx < len(sorted_cur_stream_event) else {}

    @staticmethod
    def get_pre_event_in_different_stream(event_list: list, cur_event: dict) -> dict:

        pre_idx = None
        for idx, event in enumerate(event_list):
            if float(event[Constant.ES]) <= cur_event[Constant.TS] and \
                    event[Constant.TS] != cur_event[Constant.TS]:
                pre_idx = idx
                break
        return event_list[pre_idx] if pre_idx else {}

    @staticmethod
    def get_event_start_end_time(event: dict) -> tuple:
        event_start_time = float(event[Constant.TS])
        event_end_time = float(event[Constant.ES])
        return event_start_time, event_end_time

    @staticmethod
    def parse_op_list(op_list: list, topk_type: str) -> tuple:
        sorted_op_list = sorted(op_list, key=lambda s: float(s[topk_type]), reverse=True)
        op_num = len(sorted_op_list)
        op_time = sum([float(op[Constant.DUR]) for op in sorted_op_list])
        return sorted_op_list, op_num, op_time

    @staticmethod
    def filter_method(op: dict) -> bool:
        filtered_op_names = ['Receive', 'Send', 'send', 'receive']
        return all([name not in op.get(Constant.NAME) for name in filtered_op_names])

    @classmethod
    def get_critical_path(cls, event_list: list) -> list:
        """Critical Path Analysis"""
        sorted_event_by_start_time = sorted(event_list, key=lambda s: float(s[Constant.TS]), reverse=False)
        sorted_event_by_end_time = sorted(event_list, key=lambda s: float(s[Constant.ES]), reverse=True)
        first_event = sorted_event_by_start_time[0]
        last_event = sorted_event_by_end_time[0]
        critical_path_event_list = []
        cur_event = last_event

        event_dict = cls.get_event_dict_by_pid_tid(event_list)
        while cur_event[Constant.TS] != first_event[Constant.TS]:
            critical_path_event_list.append(cur_event)
            pre_event = cls.get_pre_event_in_same_stream(cur_event, event_dict)

            if not pre_event:
                pre_event = cls.get_pre_event_in_different_stream(sorted_event_by_end_time, cur_event)
            pre_event_end_time = float(pre_event[Constant.ES])
            if float(cur_event[Constant.TS]) - pre_event_end_time <= NumberConstant.TIME_INTERVAL:
                cur_event = pre_event
            else:
                pre_event = cls.get_pre_event_in_different_stream(sorted_event_by_end_time, cur_event)
                cur_event = pre_event
            if not cur_event:
                break
            if cur_event[Constant.TS] == first_event[Constant.TS]:
                critical_path_event_list.append(cur_event)
        sorted_critical_path = sorted(critical_path_event_list, key=lambda s: float(s[Constant.TS]), reverse=False)
        return sorted_critical_path

    @classmethod
    def event_type_analysis(cls, critical_path: list, top_k: int = 5) -> dict:
        top_type = "serial_time"
        op_dict = {
            Constant.TASK_TYPE_AI_CORE: [],
            Constant.TASK_TYPE_AI_CPU: [],
            Constant.TASK_TYPE_HCCL: []
        }
        for event in critical_path:
            if event[Constant.TASK_TYPE] in op_dict:
                op_dict[event[Constant.TASK_TYPE]].append(event)

        analysis_result = {}
        for op_type in op_dict.keys():
            sorted_op_list, op_num, op_time = cls.parse_op_list(op_dict[op_type], top_type)

            # Filter out Receive and Send operators
            if op_type == Constant.TASK_TYPE_HCCL:
                topk_op = [op for i, op in zip(range(top_k), filter(cls.filter_method, sorted_op_list))]
            else:
                topk_op = sorted_op_list[0:top_k]

            analysis_result[op_type] = {"op_num": op_num, "op_time": op_time, "topk_op": topk_op}
        return analysis_result

    @classmethod
    def get_time_intersection_event(cls, cur_event: dict, event_list: list) -> list:
        """Find all events that have intersections with cur_event."""
        interval_event_list = []
        cur_ts, cur_te = cls.get_event_start_end_time(cur_event)
        for event in event_list:
            event_ts, event_te = cls.get_event_start_end_time(event)
            if event != cur_event and event_ts < cur_te and event_te > cur_ts and event_ts < event_te:
                interval_event_list.append(event)
        return interval_event_list

    @classmethod
    def get_event_serial_parallel_time(cls, cur_event: dict, intersection_event_list: list) -> tuple[float, float]:
        """Calculate the serial time and parallel time of an event."""
        sorted_intersection_event = sorted(intersection_event_list, key=lambda s: float(s[Constant.TS]), reverse=False)
        individual_time_list = []
        while len(sorted_intersection_event) > 0:
            individual_event = sorted_intersection_event.pop(0)
            individual_event_ts, individual_event_te = cls.get_event_start_end_time(individual_event)
            removed_event_list = []
            for event in sorted_intersection_event:
                event_ts, event_te = cls.get_event_start_end_time(event)
                # Delete events hidden in individual_event time
                if event_te <= individual_event_te and event != individual_event and event not in removed_event_list:
                    removed_event_list.append(event)
                if event_ts < individual_event_te < event_te:
                    individual_event_te = event_te
                    removed_event_list.append(event)
                if event_ts >= individual_event_te:
                    break
            for event in removed_event_list:
                sorted_intersection_event.remove(event)
            individual_time_list.append([individual_event_ts, individual_event_te])
        parallel_time = 0
        cur_ts, cur_te = cls.get_event_start_end_time(cur_event)
        for time_record in individual_time_list:
            ts = cur_ts if time_record[0] <= cur_ts else time_record[0]
            te = cur_te if time_record[1] >= cur_te else time_record[1]
            parallel_time += te - ts if te - ts >= 0 else 0
        serial_time = float(cur_event[Constant.DUR]) - parallel_time
        return serial_time, parallel_time

    @classmethod
    def event_execution_type_analysis(cls, critical_path: list, event_list: list) -> list:
        """"get execution type of event in critical path """
        execution_type_analysis_result = []
        for event in critical_path:
            if event.get(Constant.TASK_TYPE) != Constant.TASK_TYPE_HCCL:
                continue
            intersection_event_list = cls.get_time_intersection_event(event, event_list)
            serial_time, parallel_time = cls.get_event_serial_parallel_time(event, intersection_event_list)
            event["serial_time"] = serial_time
            event["parallel_time"] = parallel_time
            execution_type_analysis_result.append(event)
        return execution_type_analysis_result

    def run(self) -> tuple:
        self.parse_ge_ops(self.ge_op_events)
        self.parse_hccl_ops(self.hccl_op_events)
        top_communication_op = self.parse()
        return top_communication_op

    def parse_ge_ops(self, ge_op_events: list) -> None:
        """Parse the timestamp of Ge Ops"""
        for ge_op_event in ge_op_events:
            self.events.append(
                {"op_name": ge_op_event.op_name,
                 "op_type": ge_op_event.op_type,
                 "task_type": ge_op_event.task_type,
                 "tid": ge_op_event.stream_id,
                 "ts": ge_op_event.start_time / NumberConstant.CONVERSION_TIME,
                 "es": (ge_op_event.start_time + ge_op_event.duration_time) / NumberConstant.CONVERSION_TIME,
                 "dur": ge_op_event.duration_time / NumberConstant.CONVERSION_TIME
                 }
            )

    def parse_hccl_ops(self, hccl_op_events: dict) -> None:
        """Parse the timestamp of Hccl Ops"""
        for hccl_op, hccl_events in hccl_op_events.items():
            hccl_op_dict = {'op_name': hccl_op,
                            'task_type': Constant.TASK_TYPE_HCCL,
                            'tid': hccl_events[0].stream_id,
                            'ts': hccl_events[0].first_timestamp,
                            'es': max([event.timestamp + event.duration for event in hccl_events])}
            hccl_op_dict['dur'] = format(hccl_op_dict['es'] - hccl_op_dict['ts'], '.4f')
            self.events.append(hccl_op_dict)

    def parse(self) -> tuple:
        """Get Top HCCL ops Through Critical Path Analysis"""
        logging.info('start to critical path analysis !'
                     f'Total ops num: {len(self.events)}, hccl ops num: {len(self.hccl_op_events)}')
        critical_path_event = self.get_critical_path(self.events)
        event_execution_type_analysis_data = self.event_execution_type_analysis(critical_path_event, self.events)
        op_type_analysis = self.event_type_analysis(event_execution_type_analysis_data)
        logging.info(f"With critical path analysis, total ops num: {len(critical_path_event)}, "
                     f"hccl ops num: {op_type_analysis[Constant.TASK_TYPE_HCCL]['op_num']}")

        top_communication_op = set()
        for idx, communication_op in enumerate(op_type_analysis[Constant.TASK_TYPE_HCCL]["topk_op"]):
            logging.info(f"Top {idx} hccl op: {communication_op[Constant.NAME]}, "
                         f"serial_time(us): {communication_op['serial_time']}, "
                         f"op dur time(us): {communication_op['dur']}")
            top_communication_op.add(communication_op[Constant.NAME])
        return tuple(top_communication_op)
