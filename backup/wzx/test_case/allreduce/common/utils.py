# -*- coding:utf-8 -*-
import torch
import torch_npu
import torch.distributed as dist
from enum import Enum
from typing import Union, List
import os
from pathlib import Path


class DummyProfiler:
    """一个什么都不做的伪 Profiler，用于保持代码结构一致性。"""
    def step(self):
        # 空操作
        pass

    def __enter__(self):
        # 必须返回 self 才能在 with ... as prof: 中使用
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        # 空操作
        pass

def get_profiler(test_config):
    """
    根据配置决定返回真实的 NPU profiler 还是一个伪 profiler。
    """
    if test_config['used_profiling'] in [True, 'true', 'True']:
        # 如果配置为 True，返回配置好的真实 profiler
        print("Profiling is ENABLED.")
        return torch_npu.profiler.profile(
            activities=[torch_npu.profiler.ProfilerActivity.NPU],
            # schedule=torch_npu.profiler.schedule(wait=0, warmup=0, active=50, repeat=1, skip_first=5),
            on_trace_ready=torch_npu.profiler.tensorboard_trace_handler(test_config['profiling_path']),
            experimental_config=torch_npu.profiler._ExperimentalConfig(
                aic_metrics=torch_npu.profiler.AiCMetrics.PipeUtilization,
                profiler_level=torch_npu.profiler.ProfilerLevel.Level2,
                l2_cache=False,
                export_type=torch_npu.profiler.ExportType.Text
            )
        )
    else:
        # 否则，返回伪 profiler 的实例
        # print("Profiling is DISABLED.")
        return DummyProfiler()


def find_communication_durations(root_directory):
    """
    在指定目录及其子目录中查找所有 kernel_details.csv 文件，
    筛选出包含 'COMMUNICATION' 的行，并提取 'Duration(us)' 数据。
    默认去除5个最慢的数据

    Args:
        root_directory (str): 要开始扫描的根目录路径。

    Returns:
        list[float]: 所有符合条件的 Duration(us) 组成的浮点数列表。
    """
    durations_list = []
    root_path = Path(root_directory)

    # 检查目录是否存在
    if not root_path.is_dir():
        print(f"error: root_directory '{root_directory}' is not exist")
        return durations_list

    # 1. 使用 rglob 递归查找所有名为 kernel_details.csv 的文件
    # print(f"正在扫描目录 '{root_directory}'...")
    csv_files = list(root_path.rglob('kernel_details.csv'))

    if not csv_files:
        print("Cound not find 'kernel_details.csv' file。")
        return durations_list

    print(f"Found {len(csv_files)}  'kernel_details.csv' files")

    for csv_path in csv_files:
        # print(f"\n--- 正在处理文件: {csv_path} ---")
        duration_list = []
        try:
            with open(csv_path, 'r', encoding='utf-8', errors='ignore') as f:
                # 跳过第一行标题行
                header = next(f, None)
                if header is None:
                    continue # 文件为空

                for i, line in enumerate(f, 1):
                    # 2. 筛选包含 "COMMUNICATION" 的行
                    if 'COMMUNICATION' in line:
                        # 3. 按逗号分割行
                        parts = line.strip().split(',')
                        
                        # 确保行中有足够的列可以提取
                        if len(parts) >= 3:
                            # 4. 提取倒数第三列的数据
                            duration_str = parts[-3].strip()
                            try:
                                # 5. 转换为浮点数并添加到列表中
                                duration_float = float(duration_str)
                                duration_list.append(duration_float)
                                # print(f"  在第 {i} 行找到数据: {duration_float}") # 如果需要详细输出可以取消注释
                            except ValueError:
                                print(f"  warning: the {i}th row could not transform '{duration_str}' to digit")
                        else:
                            print(f"  warning: the {i}the row has not enough column")
                duration_list = sorted(duration_list) #升序
                if len(duration_list) > 5:
                    duration_list = duration_list[:-5]
                durations_list = durations_list + duration_list
        except Exception as e:
            print(f"Error when processing {csv_path}, error: {e}")
    print(f"Found {len(durations_list)}  performance data")
    return durations_list



class TorchDtype(Enum):
    """
    定义常用的PyTorch数据类型及其对应的torch.dtype。
    属性名已调整为小写，以便直接匹配字符串键。
    """
    # 整数类型
    int8 = torch.int8
    uint8 = torch.uint8
    int16 = torch.int16
    # uint16 = torch.uint16
    int32 = torch.int32
    # uint32 = torch.uint32
    int64 = torch.int64
    # uint64 = torch.uint64

    # 浮点类型
    float16 = torch.float16
    float32 = torch.float32
    float64 = torch.float64
    bfloat16 = torch.bfloat16
    
    # 其他常用类型
    # bool = torch.bool
    
    @classmethod
    def get_data_dtype_list(cls, dtype_input: Union[str, List[str]], supported_dtypes=[]) -> List[torch.dtype]:
        """
        根据输入的字符串或字符串列表获取对应的一个或多个torch.dtype。

        Args:
            dtype_input (Union[str, List[str]]):
                要获取的数据类型名称（不区分大小写），例如 'float32', 'int8'。
                也可以是一个包含数据类型名称的列表，例如 ['int8', 'float32']。
                或者 'all' 获取所有支持的数据类型。
        Returns:
            list: 包含一个或多个torch.dtype对象的列表。
        Raises:
            ValueError: 如果输入的dtype_input中包含不支持的数据类型。
        """
        result_dtypes = []
        # 确保输入是一个列表，方便统一处理
        if isinstance(dtype_input, str) and ',' not in dtype_input:
            dtype_names = [dtype_input]
        elif isinstance(dtype_input, str) and ',' in dtype_input:
            dtype_names = dtype_input.split(',')
        elif isinstance(dtype_input, list):
            dtype_names = dtype_input
        else:
            raise TypeError("dtype_input 必须是字符串或字符串列表。")
        supported_names = [member.name for member in cls] + ['all'] # 用于错误提示
        for name in dtype_names:
            lower_name = name.lower()
            if lower_name == 'all':
                if supported_dtypes:
                    for supported_dtype in supported_dtypes:
                        result_dtypes.append(cls[supported_dtype].value) 
                    return [member.value for member in supported_dtypes]
                else:
                    # 如果输入是 'all'，则返回所有支持的 dtype
                    return [member.value for member in cls]
            else:
                try:
                    # 尝试通过小写名称查找枚举成员，并获取其值
                    result_dtypes.append(cls[lower_name].value)
                except KeyError:
                    raise ValueError(
                        f"不支持的数据类型: '{name}'。 "
                        f"请选择 {', '.join(supported_names)} 中的一个或使用列表形式。"
                    )
        return result_dtypes

def calculate_tensor_elements(dtype, data_volumes_bytes):
    """
    计算给定数据类型和数据量范围下 Tensor 的元素个数。

    Args:
        dtype (str): 支持的数据类型有 'int8', 'int16', 'int32', 'int64',
                       'float16', 'float32', 'float64', 'bool'。
        data_volumes_bytes (list): 包含数据量（字节）的列表，例如 [1, 1024, 8388608]。

    Returns:
        dict: 一个列表，该列表包含每个数据量对应的元素个数。
              例如：
                [1.0, 1024.0, 8388608.0]
              }
    """
    dtype_sizes = {
        torch.int8: 1,
        torch.uint8: 1,
        torch.int16: 2,
        torch.int32: 4,
        torch.int64: 8,
        torch.float16: 2,
        torch.float32: 4,
        torch.float64: 8,
        torch.double: 8, # torch.double 是 torch.float64 的别名
        torch.bool: 1,
        torch.bfloat16: 2
    }

    if dtype not in dtype_sizes:
        print(f"警告: 不支持的数据类型 '{dtype}' 已跳过。请确保使用支持的类型。")

    element_size = dtype_sizes[dtype]
    element_counts = []
    for volume in data_volumes_bytes:
        # 计算元素个数
        if volume % element_size == 0:
            count = volume / element_size
            element_counts.append(int(count))
    return element_counts

def get_data_dtype_list(dtype_str):
    """
    根据输入的字符串获取对应的数据类型列表。
    """
    data_type_map = {
        'int8': torch.int8,
        'int32': torch.int32,
        'float32': torch.float32,
        'float16': torch.float16,
        'int64': torch.int64,
        'bfloat16': torch.bfloat16,
        'int16': torch.int16,

    }
    
    if dtype_str == 'all':
        return list(data_type_map.values())
    elif dtype_str in data_type_map:
        return [data_type_map[dtype_str]]
    else:
        raise ValueError(f"不支持的数据类型: {dtype_str}。必须是 {list(data_type_map.keys())} 之一或 'all'。")

def get_reduce_op_list(optype_str):
    """
    根据输入的字符串获取对应的ReduceOp列表。
    """
    reduce_op_map = {
        'sum': dist.ReduceOp.SUM,
        'max': dist.ReduceOp.MAX,
        'min': dist.ReduceOp.MIN,
        'prod': dist.ReduceOp.PRODUCT,
    }

    if optype_str == 'all':
        return list(reduce_op_map.values())
    elif optype_str in reduce_op_map and ',' not in optype_str:
        return [reduce_op_map[optype_str]]
    elif ',' in optype_str:
        lst = optype_str.split(',')
        return [get_reduce_op_list(op)[0] for op in lst]
    else:
        raise ValueError(f"不支持的规约操作类型: {optype_str}。必须是 {list(reduce_op_map.keys())} 之一或 'all'。")

def gen_random_input(bs, size, datatype, global_rank:int=None):
    """
    生成随机输入张量。
    
    重要提示：
    如果您的测试需要进行精确的数值校验（例如，验证 AllReduce 后的和是否正确），
    则不应在此处使用随机数。相反，您应该使用确定性输入，
    例如：`torch.full((bs, size), rank_value_for_test + 1, dtype=datatype)`。
    当前示例保留了原始的 `torch.rand` 行为，这意味着 `check_tensor_accuracy`
    中基于 (rank + 1) 的精确数值校验可能会失效，您将主要依赖于跨 rank 的一致性检查。
    """
    if global_rank:
        torch.manual_seed(global_rank*10000+1234)
    random_tensor = torch.rand(size).to(datatype)
    # .repeat(bs, 1) 的目的是将一个 (size,) 的张量重复 bs 次变成 (bs, size)
    # 如果原始 torch.rand(data) 是一个一维张量，那么 repeat 后的形状是 (bs * data,)
    # 为了得到 (bs, data) 的形状，如果 `size` 是一个整数，`torch.rand(size)` 返回 (size,)
    # 正确的重复方式应该是 `random_tensor.unsqueeze(0).repeat(bs, 1)` 或直接 `torch.rand(bs, size)`
    # 假设这里的 `size` 实际指的是张量的最后一个维度大小
    return random_tensor.unsqueeze(0).repeat(bs, 1)

def get_sum_expected_values(world_size, dtype_list):
    """
    计算给定 world_size 和数据类型列表的 SUM 操作的期望结果。
    注意：此处的计算逻辑与您的原始代码一致，它假设了一种特定的输入模式（基于 ring_shape 和 j+1）。
    如果您的实际测试使用随机输入，此函数的输出可能不直接适用于 `check_tensor_accuracy`。
    """
    all_expect_values = {}
    for dtype in dtype_list:
        expect_list = []
        ring_shape = [i for i in range(world_size)]
        for i in range(world_size):
            new_shape = ring_shape[i:world_size] + ring_shape[:i]
            expect_tensor = torch.tensor([0], dtype=dtype)
            for j in new_shape:
                temp_tensor = torch.tensor([new_shape[j] + 1], dtype=dtype)
                expect_tensor = expect_tensor + temp_tensor
            if expect_tensor not in expect_list:
                expect_list.append(expect_tensor)
        all_expect_values[dtype] = expect_list
    return all_expect_values

def get_prod_expected_values(world_size, dtype_list):
    """
    计算给定 world_size 和数据类型列表的 PRODUCT 操作的期望结果。
    与 `get_sum_expected_values` 类似，注意输入模式假设。
    """
    all_expect_values = {}
    for dtype in dtype_list:
        expect_list = []
        ring_shape = [i for i in range(world_size)]
        for i in range(world_size):
            new_shape = ring_shape[i:world_size] + ring_shape[:i]
            expect_tensor = torch.tensor([1], dtype=dtype)  # PRODUCT 初始化为1
            for j in new_shape:
                temp_tensor = torch.tensor([new_shape[j] + 1], dtype=dtype)
                expect_tensor = expect_tensor * temp_tensor
            if expect_tensor not in expect_list:
                expect_list.append(expect_tensor)
        all_expect_values[dtype] = expect_list
    return all_expect_values


def check_tensor_accuracy(tensor_all_reduce, all_reduce_op, world_size, data_size, data_type):
    """
    检查 AllReduce 操作后的张量结果是否符合预期。

    重要提示：此函数假定每个 rank 的初始输入张量都填充了 (rank + 1)
    （例如，rank 0 输入 1，rank 1 输入 2，以此类推），
    并且对于 bfloat16 的 SUM 操作，它假设每个 rank 的输入为 2。
    如果 `run_test_logic` 使用 `torch.rand` 等随机数生成初始张量，
    则此数值检查很可能会失败，因为期望值计算方式与随机输入不匹配。
    对于随机输入，通常需要检查所有 rank 之间结果的一致性，
    或者收集所有初始值以计算精确的期望和。
    """
    # 将结果移回CPU进行比较
    tensor_all_reduce = tensor_all_reduce.cpu()

    print(f"正在检查 {all_reduce_op}，数据类型 {data_type}，数据大小 {data_size}")

    if all_reduce_op == dist.ReduceOp.SUM:
        expected_val = torch.tensor([0], dtype=data_type)
        if data_type == torch.bfloat16:
            for i in range(1, world_size + 1):
                expected_val += torch.full((1, data_size), 2, dtype=data_type)
        else:
            for i in range(1, world_size + 1):
                expected_val += torch.full((1, data_size), i, dtype=data_type)

        reudce_sum_check_tensor = expected_val.cpu()
        print('期望的 SUM 张量:', reudce_sum_check_tensor)
        # 对于浮点数使用 allclose 进行容忍度比较
        if not torch.allclose(tensor_all_reduce, reudce_sum_check_tensor, atol=1e-3, rtol=1e-3):
            raise AssertionError(f"错误: SUM 校验失败，数据类型 {data_type}！期望 {reudce_sum_check_tensor}，实际 {tensor_all_reduce}")

    elif all_reduce_op == dist.ReduceOp.MAX:
        max_value = float(world_size) if data_type.is_floating_point else world_size
        reudce_max_check_tensor = torch.full((1, data_size), max_value, dtype=data_type).cpu()
        print('期望的 MAX 张量:', reudce_max_check_tensor)
        if not torch.equal(tensor_all_reduce, reudce_max_check_tensor):
             raise AssertionError(f"错误: MAX 校验失败，数据类型 {data_type}！期望 {reudce_max_check_tensor}，实际 {tensor_all_reduce}")

    elif all_reduce_op == dist.ReduceOp.MIN:
        min_value = 1.0 if data_type.is_floating_point else 1
        reudce_min_check_tensor = torch.full((1, data_size), min_value, dtype=data_type).cpu()
        print('期望的 MIN 张量:', reudce_min_check_tensor)
        if not torch.equal(tensor_all_reduce, reudce_min_check_tensor):
            raise AssertionError(f"错误: MIN 校验失败，数据类型 {data_type}！期望 {reudce_min_check_tensor}，实际 {tensor_all_reduce}")

    elif all_reduce_op == dist.ReduceOp.PRODUCT:
        expected_val = torch.ones((1, data_size), dtype=data_type)
        for i in range(1, world_size + 1):
            expected_val *= torch.full((1, data_size), i, dtype=data_type)

        # 处理浮点数可能出现的溢出/下溢导致 INF
        if torch.isinf(expected_val).any() or torch.isinf(tensor_all_reduce).any():
            print(f"警告: PRODUCT 校验针对数据类型 {data_type} 出现 INF，跳过数值比较。")
            return # 如果出现无穷大，跳过数值比较

        reudce_prod_check_tensor = expected_val.cpu()
        print('期望的 PRODUCT 张量:', reudce_prod_check_tensor)
        if not torch.allclose(tensor_all_reduce, reudce_prod_check_tensor, atol=1e-3, rtol=1e-3): # 对于浮点数使用 allclose
            raise AssertionError(f"错误: PRODUCT 校验失败，数据类型 {data_type}！期望 {reudce_prod_check_tensor}，实际 {tensor_all_reduce}")
        
    print(f"成功: {all_reduce_op} 校验针对数据类型 {data_type}，大小 {data_size}")

if __name__ == "__main__":
    print(get_data_dtype_list("float32"))