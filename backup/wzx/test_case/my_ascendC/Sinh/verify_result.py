import os
import sys
import numpy as np

loss = 1e-3

def verify_result(real_result, golden):
    result = np.fromfile(real_result, dtype=np.float16)
    golden = np.fromfile(golden, dtype=np.float16)
    flag = 0
    for i in range(len(result)):
        diff = abs(result[i] - golden[i])
        if abs(golden[i]) > 0.001:
            if  abs(diff / golden[i]) > loss:
                flag += 1
                error_message = "output[{}] is {}, expect {}".format(i, result[i], golden[i])
                print(error_message)
        else:
            if diff > loss:
                flag += 1
                error_message = "output[{}] is {}, expect {}".format(i, result[i], golden[i])
                print(error_message)

    if flag > 16:
        print("[ERROR] The accuracy error exceeds two thousandths.")
        return False

    print("test pass")
    return True


if __name__ == '__main__':
    verify_result(sys.argv[1], sys.argv[2])
