import torch
import torch.distributed as dist
import torch_npu
import os
import numpy as np

# 从 common 目录导入基类和工具函数
from common.base import DistributedTestBase
from common.utils import TorchDtype, gen_random_input, get_reduce_op_list, calculate_tensor_elements, get_profiler, find_communication_durations



# DATA_SIZE = [2**i for i in range(24)] # 8M对应2**23

class AllReduceTest(DistributedTestBase):
    def __init__(self):
        super().__init__()
        os.environ['ASCEND_GLOBAL_LOG_LEVEL']='2'
        # os.environ['ASCEND_PROCESS_LOG_PATH'] = '/root/ascend/log'

    def run_test_logic(self, local_rank, global_rank, config):
        """
        实现 AllReduce 具体的测试逻辑。此方法会在每个分布式进程中被调用。
        """
        # 从 config 中获取参数，这些参数已经在基类中被解析和处理
        torch.npu.set_device(local_rank)
        data_type_list = TorchDtype.get_data_dtype_list(config['dtype'], ['int8', 'int16', 'int32', 'float16', 'float32', 'bfloat16'])
        all_reduce_op_list = get_reduce_op_list(config['optype'])
        batch_num = config['batch_num']
        world_size = config['world_size']
        DATA_SIZE = config['data_list']
        local_tests_passed_status = []
        prof_context = get_profiler(config)
        
        with prof_context as prof:
            for data_type in data_type_list:
                datasize_list = calculate_tensor_elements(data_type, DATA_SIZE)
                for data_size in datasize_list:
                    for all_reduce_op in all_reduce_op_list:
                        if all_reduce_op == dist.ReduceOp.PRODUCT and data_type == torch.bfloat16:
                            print("   警告: 当前不支持 reduce prod bfloat16，跳过此测试。")
                            continue
                        if  global_rank == 0:
                            print(f"   数据大小: {data_size}, 类型: {data_type}, 操作: {all_reduce_op}, 循环{config['mul_num']}次中")

                        # 生成初始张量并移动到 NPU 设备
                        tensor_init = gen_random_input(batch_num, data_size, data_type, global_rank).npu()

                        # 存储每次循环的输出，以便后续聚合和一致性检查
                        all_loop_outputs_local = []
                        for test_loop in range(config['mul_num']):
                            tensor_reduce = tensor_init.clone().detach()
                            dist.all_reduce(tensor_reduce, op=all_reduce_op)
                            # 收集每次循环的输出副本
                            all_loop_outputs_local.append(tensor_reduce.cpu().clone().detach()) # 克隆并分离，避免后续操作影响
                            #prof.step()
                        is_consistent = True
                        if not all_loop_outputs_local:
                            print(f"   Rank {global_rank}: 错误: 未收集到任何结果进行本地一致性检查。")
                            is_consistent = False
                        else:
                            # 以当前 Rank 的第一次循环结果作为参考基准
                            reference_output = all_loop_outputs_local[0]
                            # 遍历当前 Rank 的所有循环结果，确保都与参考结果一致
                            for loop_idx in range(1, config['mul_num']):
                                current_tensor = all_loop_outputs_local[loop_idx]
                                if not torch.equal(current_tensor, reference_output):
                                    is_consistent = False
                                    print(f"   数据大小: {data_size}, 类型: {data_type}, 操作: {all_reduce_op} Rank {global_rank}: 一致性错误: 循环 {loop_idx} 的输出与首次循环结果不同（精确匹配失败）。")
                                    print(f"   参考 (前5个): {reference_output.flatten()[:5].tolist()}\n   当前 (前5个): {current_tensor.flatten()[:5].tolist()}\n差异: {torch.abs(current_tensor - reference_output).max()}")
                                    break
                        if is_consistent:
                            # print(f"   DETERMINISTIC_AIV_SUCCESS\n-------------------------------------------")
                            local_tests_passed_status.append(True)
                        else:
                            # print(f"  DETERMINISTIC_AIV_FAIL\n-------------------------------------------")
                            local_tests_passed_status.append(False)

        all_ranks_local_statuses = [None] * world_size
        dist.all_gather_object(all_ranks_local_statuses, local_tests_passed_status)
        global_pass = True
        for r_idx in range(world_size):
            rank_status_list = all_ranks_local_statuses[r_idx]
            if not all(rank_status_list): # 检查当前 Rank 的所有本地测试是否都通过
                global_pass = False
                print(f"   Rank {r_idx} 的某些本地确定性测试未能通过。")

        if global_pass:
            print(f"\nSummary: DETERMINISTIC_AIV_SUCCESS: 数据大小: {DATA_SIZE}, 类型: {data_type_list}, 操作: {all_reduce_op_list} AllReduce")
        else:
            print(f"\nSummary: FAILED: 数据大小: {DATA_SIZE}, 类型: {data_type_list}, 操作: {all_reduce_op_list} AllReduce 测试失败")
        if global_pass == True:
            print(f"device:{local_rank} pytorch hccl test success")
        if global_rank==0 and config['used_profiling']:
            perf_data = find_communication_durations(config['profiling_path'])
            print(f"[AllReduce] performance(us):{sum(perf_data) / len(perf_data):.2f}")
            if os.path.exists(config['profiling_path']):
                import shutil
                shutil.rmtree(config['profiling_path'])

if __name__ == "__main__":
    # 创建测试实例并运行
    test = AllReduceTest()
    test.run()
