# -*- coding:utf-8 -*-
import os
import argparse
import torch
import torch_npu
import torch.distributed as dist
import torch.multiprocessing as mp

from abc import ABC, abstractmethod
from common.utils import get_sum_expected_values, get_prod_expected_values

class DistributedTestBase(ABC):
    """
    分布式测试的基类，处理参数解析、环境配置和多进程启动。
    子类应重写 run_test_logic 方法。
    """
    def __init__(self):
        self.args = None
        self.config = {} # 用于存储解析和处理后的配置

    def define_arguments(self, parser):
        """
        定义公共和自定义命令行参数。子类可以覆盖此方法以添加自己的特定参数。
        """
        parser.add_argument('--master_ip', default=None, type=str,
                            help="分布式设置的主节点 IP 地址。")
        parser.add_argument('--master_port', default='50001', type=str,
                            help="分布式设置的主节点端口。")
        parser.add_argument('--dev_num', default=None, type=int, required=True,
                            help="每个节点中设备的数量（NPU/GPU）。")
        parser.add_argument('--server_num', type=int, required=True,
                            help="参与测试的服务器总数。")
        parser.add_argument('--server_index', default=0, type=int,
                            help="当前服务器的索引（从0开始）。")
        
        # 通用 HCCL/PyTorch 分布式参数
        parser.add_argument('--used_devices', default=None, type=str,
                             help="要使用的设备 ID 列表，以逗号分隔，例如 '0,1,2,3'。会设置 ASCEND_RT_VISIBLE_DEVICES。")
        parser.add_argument('--used_profiling', default='false', choices=['true', 'false'],
                             help="启用性能分析。'true' 或 'false'。")
        parser.add_argument('--profiling_path', default='/home/log/npu/profiling/', type=str,
                             help="保存性能分析数据的路径。")
        parser.add_argument('--used_algo', default='auto_algo', 
                             choices=['auto_algo', 'ring', 'H-D_R', 'NHR', 'NB','pipeline', 'pairwise', 'fullmesh'],
                             help="要使用的 HCCL 算法。")
        parser.add_argument('--expansion_mode', default='false', 
                             choices=['false', 'AI_CPU', 'AIV', 'HOST'],
                             help="HCCL 操作扩展模式。")
        parser.add_argument('--used_deterministic', default='false', 
                             choices=['true', 'false','STRICT'],
                             help="启用 HCCL 确定性模式。")

        # 具体的测试参数（可以在子类中添加或调整）
        parser.add_argument('--data_list', default='1024',
                             help="数据大小列表，以逗号分隔，例如 '1024,2048'。")
        parser.add_argument('--dtype', default='all',
                             help="要测试的数据类型，例如 'float32', 'int8', 或 'all'。")
        parser.add_argument('--optype', default='all',
                             help="要测试的规约操作类型，例如 'sum', 'max', 或 'all'。")
        parser.add_argument('--mul_num', default=1, type=int, # 确保类型为 int
                             help="每种配置的测试循环次数。")
        parser.add_argument('--batch_num', default=3, type=int, # 确保类型为 int
                             help="张量的批次数量。")


    def _parse_arguments(self):
        """
        解析命令行参数并进行基本校验。
        """
        cmdline = argparse.ArgumentParser(
            formatter_class=argparse.ArgumentDefaultsHelpFormatter)
        self.define_arguments(cmdline) # 允许子类定义或修改参数
        
        self.args, unknown_args = cmdline.parse_known_args()
        if len(unknown_args) > 0:
            for bad_arg in unknown_args:
                print(f"错误: 未知的命令行参数: {bad_arg}")
            raise ValueError("无效的命令行参数！")

    def _validate_and_process_args(self):
        """
        校验和处理解析后的参数，填充 config 字典。
        采用白名单方式，将 argparse.Namespace 的属性安全地复制到 config 字典。
        """
        # 定义需要直接从 self.args 复制到 self.config 的参数白名单
        # 参数名必须与 argparse 定义的参数名一致
        direct_copy_params = [
            'master_ip', 'master_port', 'dev_num', 'server_num', 'server_index',
            'dtype', 'optype', 'mul_num', 'batch_num', 'used_devices',
            'used_profiling', 'profiling_path', 'used_algo', 'expansion_mode', 'used_deterministic',
            'data_list'
        ]

        # 使用字典推导式批量赋值，简洁高效
        self.config = {
            param: getattr(self.args, param)
            for param in direct_copy_params
        }

        # master_ip 校验：确保主节点 IP 已设置
        if self.config['master_ip'] is None:
            print("警告: master_ip 未指定，尝试自动获取本机 IP 地址...")
            try:
                import subprocess
                cmd = "ifconfig | grep 'inet 90' | head -n 1 | awk '{print$2}'"
                result = subprocess.run(cmd, shell=True, check=True, capture_output=True, text=True)
                host_ip = result.stdout.strip()
                if host_ip:
                    self.config['master_ip'] = host_ip
                    print(f"成功获取 master_ip: {host_ip}")
                else:
                    raise ValueError("无法自动获取 master_ip，请通过 --master_ip 参数手动指定。")
            except subprocess.CalledProcessError as e:
                raise ValueError(f"执行 IP 获取命令失败: {e.stderr.strip()}。请通过 --master_ip 参数手动指定。")
            except Exception as e:
                raise ValueError(f"自动获取 master_ip 过程中发生错误: {e}。请通过 --master_ip 参数手动指定。")

        # data_list: 从逗号分隔的字符串转换为整数列表
        # 即使 argparse 定义了类型，也在此处进行显式转换以确保格式统一
        self.config['data_list'] = [int(i) for i in str(self.config['data_list']).split(',')]
        
        # used_profiling: 将字符串 'true'/'false' 转换为布尔值 True/False
        self.config['used_profiling'] = (self.config['used_profiling'].lower() == 'true')

        # 计算world_size大小
        self.config['world_size'] = self.config['server_num'] * self.config['dev_num']

        # --- 设置环境变量 ---
        self._set_environment_variables()

        # --- 预计算期望值 ---
        # 这些函数返回用于校验的期望结果，依赖于 world_size 和数据类型
        self.config['float_type_list'] = [torch.float32, torch.float16, torch.bfloat16]
        all_sum_check = get_sum_expected_values(self.config['world_size'], self.config['float_type_list'])
        all_prod_check = get_prod_expected_values(self.config['world_size'], self.config['float_type_list'])
        self.config['all_float_check'] = {'sum': all_sum_check, 'prod': all_prod_check}


    def _set_environment_variables(self):
        """
        根据配置参数设置 HCCL 相关的环境变量。
        """
        # ASCEND_RT_VISIBLE_DEVICES
        if self.config['used_devices'] is not None:
            os.environ['ASCEND_RT_VISIBLE_DEVICES'] = self.config['used_devices']

        # Profiling
        os.makedirs(self.config['profiling_path'], exist_ok=True)

        # HCCL_ALGO
        algo_map = {
            'auto_algo': None, # 表示不设置该环境变量
            'ring': "level0:NA;level1:ring",
            'H-D_R': "level0:NA;level1:H-D_R",
            'NHR': "level0:NA;level1:NHR",
            'NB': "level0:NA;level1:NB",
            'pipeline': "level0:NA;level1:pipeline",
            'pairwise': "level0:NA;level1:pairwise",
            'fullmesh': "level0:NA;level1:fullmesh"
        }
        hccl_algo = algo_map.get(self.config['used_algo'])
        if hccl_algo is None:
            if 'HCCL_ALGO' in os.environ:
                del os.environ['HCCL_ALGO'] # auto_algo则取消设置环境变量
        else:
            os.environ['HCCL_ALGO'] = hccl_algo

        # HCCL_OP_EXPANSION_MODE
        expansion_map = {
            'false': None,
            'AI_CPU': 'AI_CPU',
            'AIV': 'AIV',
            'HOST': 'HOST'
        }
        expansion_mode = expansion_map.get(self.config['expansion_mode'])
        if expansion_mode is None:
            if 'HCCL_OP_EXPANSION_MODE' in os.environ:
                del os.environ['HCCL_OP_EXPANSION_MODE']
        else:
            os.environ['HCCL_OP_EXPANSION_MODE'] = expansion_mode

        # HCCL_DETERMINISTIC
        if self.config['used_deterministic'] == 'true':
            os.environ['HCCL_DETERMINISTIC'] = 'true'
        elif self.config['used_deterministic'] == 'STRICT':
            os.environ['HCCL_DETERMINISTIC'] = 'STRICT'
        else: # 'false' 或其他值，表示不设置
            if 'HCCL_DETERMINISTIC' in os.environ:
                del os.environ['HCCL_DETERMINISTIC']

    def _print_config(self):
        """
        打印最终的配置信息，方便调试和日志记录。
        """
        print("\n--- 测试配置 ---")
        for key, value in self.config.items():
            print(f"{key:<20}: {value}")
        print(f"主进程 PID          : {os.getpid()}")
        print("------------------\n")

    def _run_worker(self, local_rank):
        """
        此方法在每个子进程中执行。
        它负责初始化分布式环境，然后调用子类实现的具体测试逻辑。
        """
        torch_npu.npu.set_device(local_rank)
        print(f"Rank {local_rank} 当前设备: {torch_npu.npu.current_device()}")

        # 计算全局 rank：本地 rank 加上服务器索引带来的偏移量
        global_rank = local_rank + self.config['dev_num'] * self.config['server_index']
        
        init_method = f"tcp://{self.config['master_ip']}:{self.config['master_port']}"
        world_size = self.config['world_size']

        print(f"Rank {global_rank} (本地 {local_rank}) 初始化方法: {init_method}, 全局进程数: {world_size}, PID: {os.getpid()}")
        
        # 初始化进程组
        dist.init_process_group(backend="hccl", 
                                rank=global_rank, 
                                world_size=world_size, 
                                init_method=init_method)

        try:
            # 调用子类实现的具体测试逻辑
            self.run_test_logic(local_rank, global_rank, self.config)
        except Exception as e:
            print(f'ERROR: device {local_rank} (global rank {global_rank}) PyTorch HCCL test fail: {e}')
            raise 
        else:
            if global_rank == 0:
                print(f'SUCCESS: device {local_rank} (global rank {global_rank}) PyTorch HCCL execute success')
        finally:
            dist.destroy_process_group()

    @abstractmethod
    def run_test_logic(self, local_rank, global_rank, config):
        """
        抽象方法：此方法包含具体的分布式测试逻辑。
        每个继承 DistributedTestBase 的子类必须实现此方法。

        Args:
            local_rank (int): 当前进程在当前节点中的设备索引。
            global_rank (int): 当前进程在整个分布式环境中的全局唯一 rank。
            config (dict): 包含所有解析和处理后的配置参数。
        """
        pass

    def run(self):
        """
        主入口点，用于解析参数、准备环境并启动多进程。
        """
        self._parse_arguments()
        self._validate_and_process_args()
        self._print_config()

        # 启动分布式进程。
        # args 是传给 _run_worker 的，由于 _run_worker 会访问 self.config，
        # 所以不需要额外传递 config 参数。
        mp.spawn(self._run_worker,
                 args=(), # 没有额外的 args 需要传给 _run_worker
                 nprocs=self.config['dev_num']) # 启动的进程数等于每个节点的设备数