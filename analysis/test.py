import os
import struct

dir_name = 'D:\\Job\\lliantiao\\cluster\\profiling_out\\PROF_000001_20220813111618169_HEBAKDMGIIHNFLIC\\device_5\\data'
data_list = []
for file_name in os.listdir(dir_name):
    if file_name.startswith('aicore') and not file_name.endswith('done'):
        try:
            with open(os.path.join(dir_name, file_name), 'rb') as file:
                data = file.read()
            struct_length = len(data) // 128
            for i in range(struct_length):
                tmp = struct.unpack('BBHHHII10Q8I', data[i * 128: i * 128 + 128])
                stream_id = tmp[17]
                task_id = tmp[4]
                total_cycle = tmp[7]
                pmu_list = tmp[9:17]
                data_list.append((stream_id, task_id, total_cycle) + pmu_list)
        except OSError:
            break
a=1