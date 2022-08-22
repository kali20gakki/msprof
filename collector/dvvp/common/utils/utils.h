/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 * Description: common util interface
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_COMMON_UTILS_UTILS_H
#define ANALYSIS_DVVP_COMMON_UTILS_UTILS_H

#include <cerrno>
#include <cstdio>
#include <fstream>
#include <memory>
#include <mutex>
#if (defined(linux) || defined(__linux__))
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <pwd.h>
#endif
#include <sstream>
#include <string>
#include <vector>
#include "mmpa_api.h"

template <typename T>
using SHARED_PTR_ALIA = std::shared_ptr<T>;
using MmProcess = Collector::Dvvp::Mmpa::MmProcess;
using MmArgvEnv = Collector::Dvvp::Mmpa::MmArgvEnv;
namespace analysis {
namespace dvvp {
namespace common {
namespace utils {

#define MSVP_MAKE_SHARED0_VOID(instance, Type)   \
    do {                                         \
        try {                                    \
            instance = std::make_shared<Type>(); \
        } catch (...) {                          \
            return;                              \
        }                                        \
    } while (0)

#define MSVP_MAKE_SHARED0_RET(instance, Type, err_code) \
    do {                                                \
        try {                                           \
            instance = std::make_shared<Type>();        \
        } catch (...) {                                 \
            return err_code;                            \
        }                                               \
    } while (0)

#define MSVP_MAKE_SHARED0_BREAK(instance, Type)  \
    do {                                         \
        try {                                    \
            instance = std::make_shared<Type>(); \
        } catch (...) {                          \
            break;                               \
        }                                        \
    } while (0)

#define MSVP_MAKE_SHARED2_BREAK(instance, Type, args0, args1) \
    do {                                                      \
        try {                                                 \
            instance = std::make_shared<Type>(args0, args1);  \
        } catch (...) {                                       \
            break;                                            \
        }                                                     \
    } while (0)

#define MSVP_MAKE_SHARED3_BREAK(instance, Type, args0, args1, arg2) \
    do {                                                            \
        try {                                                       \
            instance = std::make_shared<Type>(args0, args1, arg2);  \
        } catch (...) {                                             \
            break;                                                  \
        }                                                           \
    } while (0)

#define MSVP_MAKE_SHARED0_CONTINUE(instance, Type) \
    do {                                           \
        try {                                      \
            instance = std::make_shared<Type>();   \
        } catch (...) {                            \
            continue;                              \
        }                                          \
    } while (0)

#define MSVP_MAKE_SHARED1_RET(instance, Type, args0, err_code) \
    do {                                                       \
        try {                                                  \
            instance = std::make_shared<Type>(args0);          \
        } catch (...) {                                        \
            return err_code;                                   \
        }                                                      \
    } while (0)

#define MSVP_MAKE_SHARED1_BREAK(instance, Type, arg0) \
    do {                                              \
        try {                                         \
            instance = std::make_shared<Type>(arg0);  \
        } catch (...) {                               \
            break;                                    \
        }                                             \
    } while (0)

#define MSVP_MAKE_SHARED1(instance, Type, para1)      \
    do {                                              \
        try {                                         \
            instance = std::make_shared<Type>(para1); \
        } catch (...) {                               \
            break;                                    \
        }                                             \
    } while (0)

#define MSVP_MAKE_SHARED1_VOID(instance, Type, para1) \
    do {                                              \
        try {                                         \
            instance = std::make_shared<Type>(para1); \
        } catch (...) {                               \
            return;                                   \
        }                                             \
    } while (0)

#define MSVP_MAKE_SHARED1_CONTINUE(instance, Type, arg0) \
    do {                                                 \
        try {                                            \
            instance = std::make_shared<Type>(arg0);     \
        } catch (...) {                                  \
            continue;                                    \
        }                                                \
    } while (0)

#define MSVP_MAKE_SHARED1_CONTINUE_CLOSE(instance, Type, arg1, close_func) \
    do {                                                                   \
        try {                                                              \
            instance = std::make_shared<Type>(arg1);                       \
        } catch (...) {                                                    \
            close_func(arg1);                                              \
            continue;                                                      \
        }                                                                  \
    } while (0)

#define MSVP_MAKE_SHARED1_VOID_CLOSE(instance, Type, arg1, close_func) \
    do {                                                                   \
        try {                                                              \
            instance = std::make_shared<Type>(arg1);                       \
        } catch (...) {                                                    \
            close_func(arg1);                                              \
            return;                                                        \
        }                                                                  \
    } while (0)

#define MSVP_MAKE_SHARED2_RET(instance, Type, arg0, arg1, err_code) \
    do {                                                            \
        try {                                                       \
            instance = std::make_shared<Type>(arg0, arg1);          \
        } catch (...) {                                             \
            return err_code;                                        \
        }                                                           \
    } while (0)

#define MSVP_MAKE_SHARED2_CONTINUE(instance, Type, args0, args1) \
    do {                                                         \
        try {                                                    \
            instance = std::make_shared<Type>(args0, args1);     \
        } catch (...) {                                          \
            continue;                                            \
        }                                                        \
    } while (0)

#define MSVP_MAKE_SHARED3_CONTINUE(instance, Type, args0, args1, args2) \
    do {                                                                \
        try {                                                           \
            instance = std::make_shared<Type>(args0, args1, args2);     \
        } catch (...) {                                                 \
            continue;                                                   \
        }                                                               \
    } while (0)

#define MSVP_MAKE_SHARED3_RET(instance, Type,                    \
    arg1, arg2, arg3, err_code)                                  \
    do {                                                         \
        try {                                                    \
            instance = std::make_shared<Type>(arg1, arg2, arg3); \
        } catch (...) {                                          \
            return err_code;                                     \
        }                                                        \
    } while (0)

#define MSVP_MAKE_SHARED4_VOID(instance, Type, arg0, arg1, arg2, arg3) \
    do {                                                               \
        try {                                                          \
            instance = std::make_shared<Type>(arg0, arg1, arg2, arg3); \
        } catch (...) {                                                \
            return;                                                    \
        }                                                              \
    } while (0)

#define MSVP_MAKE_SHARED3_VOID(instance, Type, arg0, arg1, arg2) \
    do {                                                               \
        try {                                                          \
            instance = std::make_shared<Type>(arg0, arg1, arg2); \
        } catch (...) {                                                \
            return;                                                    \
        }                                                              \
    } while (0)

#define MSVP_MAKE_SHARED4_RET(instance, Type, args0, args1, args2, args3, err_code) \
    do {                                                                            \
        try {                                                                       \
            instance = std::make_shared<Type>(args0, args1, args2, args3);          \
        } catch (...) {                                                             \
            return err_code;                                                        \
        }                                                                           \
    } while (0)

#define MSVP_MAKE_SHARED4_BREAK(instance, Type, args0, args1, args2, args3) \
    do {                                                                    \
        try {                                                               \
            instance = std::make_shared<Type>(args0, args1, args2, args3);  \
        } catch (...) {                                                     \
            break;                                                          \
        }                                                                   \
    } while (0)

#define MSVP_MAKE_SHARED5_RET(instance, Type,                                \
    arg1, arg2, arg3, arg4, arg5, err_code)                                  \
    do {                                                                     \
        try {                                                                \
            instance = std::make_shared<Type>(arg1, arg2, arg3, arg4, arg5); \
        } catch (...) {                                                      \
            return err_code;                                                 \
        }                                                                    \
    } while (0)

#define MSVP_MAKE_SHARED6_RET(instance, Type,                                                   \
    arg1, arg2, arg3, arg4, arg5, arg6, errCode)                                                \
    do {                                                                                        \
        try {                                                                                   \
            instance = std::make_shared<Type>(arg1, arg2, arg3, arg4, arg5, arg6);              \
        } catch (...) {                                                                         \
            return errCode;                                                                     \
        }                                                                                       \
    } while (0)

#define MSVP_MAKE_SHARED7_VOID(instance, Type,                                           \
    arg0, arg1, arg2, arg3, arg4, arg5, arg6)                                            \
    do {                                                                                 \
        try {                                                                            \
            instance = std::make_shared<Type>(arg0, arg1, arg2, arg3, arg4, arg5, arg6); \
        } catch (...) {                                                                  \
            return;                                                                      \
        }                                                                                \
    } while (0)

#define MSVP_MAKE_SHARED7_RET(instance, Type,                                                   \
    arg0, arg1, arg2, arg3, arg4, arg5, arg6, errCode)                                          \
    do {                                                                                        \
        try {                                                                                   \
            instance = std::make_shared<Type>(arg0, arg1, arg2, arg3, arg4, arg5, arg6);        \
        } catch (...) {                                                                         \
            return errCode;                                                                     \
        }                                                                                       \
    } while (0)

#define MSVP_MAKE_SHARED8_VOID(instance, Type,                                                  \
    arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7)                                             \
    do {                                                                                        \
        try {                                                                                   \
            instance = std::make_shared<Type>(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);  \
        } catch (...) {                                                                         \
            return;                                                                             \
        }                                                                                       \
    } while (0)

#define MSVP_MAKE_SHARED8_RET(instance, Type,                                                       \
        arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, errCode)                                    \
        do {                                                                                        \
            try {                                                                                   \
                instance = std::make_shared<Type>(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);  \
            } catch (...) {                                                                         \
                return errCode;                                                                     \
            }                                                                                       \
        } while (0)


#define MSVP_MAKE_SHARED9_VOID(instance, Type,                                                       \
    arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)                                            \
    do {                                                                                             \
        try {                                                                                        \
            instance = std::make_shared<Type>(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8); \
        } catch (...) {                                                                              \
            return;                                                                                  \
        }                                                                                            \
    } while (0)

#define MSVP_MAKE_SHARED9_RET(instance, Type,                                                            \
        arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, errCode)                                   \
        do {                                                                                             \
            try {                                                                                        \
                instance = std::make_shared<Type>(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8); \
            } catch (...) {                                                                              \
                return errCode;                                                                          \
            }                                                                                            \
        } while (0)


#define MSVP_MAKE_SHARED_ARRAY_RET(instance, Type, len, err_code)                           \
    do {                                                                                    \
        try {                                                                               \
            instance = SHARED_PTR_ALIA<Type>(new Type[len], std::default_delete<Type[]>()); \
        } catch (...) {                                                                     \
            return err_code;                                                                \
        }                                                                                   \
    } while (0)

#define MSVP_MAKE_SHARED_ARRAY_VOID(instance, Type, len)                           \
    do {                                                                                    \
        try {                                                                               \
            instance = SHARED_PTR_ALIA<Type>(new Type[len], std::default_delete<Type[]>()); \
        } catch (...) {                                                                     \
            MSPROF_LOGE("new buffer failed");                                               \
            return;                                                                         \
        }                                                                                   \
    } while (0)

#define MSVP_TRY_BLOCK_BREAK(block) \
    do {                            \
        try {                       \
            block                   \
        } catch (...) {             \
                break;              \
        }                           \
    } while (0)

#define FUNRET_CHECK_RET_VALUE(inPut, checkValue, checkOkRetValue, checkFailRetValue) do {   \
    if ((inPut) != (checkValue)) {                                                           \
        MSPROF_LOGE("Function ret check failed");                                            \
        return checkFailRetValue;                                                            \
    } else {                                                                                 \
        return checkOkRetValue;                                                              \
    }                                                                                        \
} while (0)

#define FUNRET_CHECK_FAIL_RET_VALUE(inPut, checkValue, checkFailRetValue) do {               \
    if ((inPut) != (checkValue)) {                                                           \
        MSPROF_LOGE("Function ret check failed");                                            \
        return checkFailRetValue;                                                            \
    }                                                                                        \
} while (0)

#define FUNRET_CHECK_EQUAL_RET_VALUE(inPut, checkValue, checkFailRetValue) do {              \
    if ((inPut) == (checkValue)) {                                                           \
        MSPROF_LOGE("Function ret check failed");                                            \
        return checkFailRetValue;                                                            \
    }                                                                                        \
} while (0)

#define FUNRET_CHECK_RET_VOID(inPut, checkValue) do {                                        \
    if ((inPut) != (checkValue)) {                                                           \
        MSPROF_LOGE("Function ret check failed");                                            \
    }                                                                                        \
    return;                                                                                  \
} while (0)

#define FUNRET_CHECK_FAIL_PRINT(inPut, checkValue) do {                                      \
    if ((inPut) != (checkValue)) {                                                           \
        MSPROF_LOGE("Function ret check failed");                                            \
    }                                                                                        \
} while (0)

#define INOTIFY_LISTEN_EVENTS        (IN_DELETE | IN_CLOSE_WRITE | IN_DELETE_SELF | IN_MOVE_SELF)

#define  US_TO_SECOND_TIMES(x)    (1000000 / (x))

#define UNUSED(x) (void)(x)

using VOID_PTR = void *;
using IDE_SESSION = void *;
using VOID_PTR_PTR = void **;
using CONST_VOID_PTR = const void *;
using UNSIGNED_CHAR_PTR = unsigned char *;
using CONST_UNSIGNED_CHAR_PTR = const unsigned char *;
using CHAR_PTR = char *;
using CHAR_PTR_PTR = char **;
using CONST_CHAR_PTR = const char *;
using CONST_CHAR_PTR_PTR = const char **;
using CHAR_PTR_CONST = char *const;
using CHAR_PTR_CONST_PTR = CHAR_PTR_CONST *;
using CONST_UINT8_PTR = const uint8_t *;
using CONST_UINT32_T_PTR = const uint32_t *;
using UINT32_T_PTR = uint32_t *;
using SIZE_T_PTR = size_t *;


constexpr int INVALID_EXIT_CODE = -1;
constexpr int VALID_EXIT_CODE = 0;
constexpr int MSVP_MAX_DEV_NUM = 64; // 64 : dev max number
constexpr int DEFAULT_HOST_ID = MSVP_MAX_DEV_NUM;
constexpr int UNLOCK_NUM = 2;
constexpr int FILE_WRITE_SIZE = 1;
constexpr unsigned int MAX_ERR_STRING_LEN = 256;
constexpr unsigned int MAX_CMD_PRINT_MESSAGE_LEN = 1024;

#if (defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER))
#define MSVP_STRDUP _strdup
const char * const MSVP_SLASH = "\\";
#else
#define MSVP_STRDUP strdup
const char * const MSVP_SLASH = "/";
#endif

struct ExecCmdParams {
    std::string cmd;
    bool async;    // if async = false, return exit code, if async = true, return child pid
    std::string stdoutRedirectFile;
    ExecCmdParams(const std::string &cmdStr, bool asyncB, const std::string &stdoutRedirectFileStr)
        : cmd(cmdStr), async(asyncB), stdoutRedirectFile(stdoutRedirectFileStr)
    {
    }
};

struct ExecCmdArgv {
    CHAR_PTR_CONST *argv;
    int argvCount;
    CHAR_PTR_CONST *envp;
    int envCount;
    ExecCmdArgv(CHAR_PTR_CONST_PTR argvT, const int argvCountT, CHAR_PTR_CONST_PTR envpT, const int envCountT)
        : argv(argvT), argvCount(argvCountT), envp(envpT), envCount(envCountT)
    {
    }
};

struct ProfIniBlock {
    std::string key;
    std::string value;
};

class Utils {
public:
    static long long GetFileSize(const std::string &path);
    static int GetFreeVolume(const std::string &path, unsigned long long &size);
    static int GetTotalVolume(const std::string &path, unsigned long long &size);
    static bool IsDir(const std::string &path);
    static bool IsAllDigit(const std::string &digitStr);
    static bool IsFileExist(const std::string &path);
    static bool IsDirAccessible(const std::string &path);
    static std::string DirName(const std::string &path);
    static std::string BaseName(const std::string &path);
    static int SplitPath(const std::string &path, std::string &dir, std::string &base);
    static int RelativePath(const std::string &path, const std::string &dir, std::string &relativePath);
    static void GetFiles(const std::string &dir, bool isRecur, std::vector<std::string> &files);
    static void GetChildFilenames(const std::string &dir, std::vector<std::string> &files);
    static int CreateDir(const std::string &path);
    static void RemoveDir(const std::string &dir, bool rmTopDir = true);
    static std::string CanonicalizePath(const std::string &path);
    static int ExecCmd(const ExecCmdParams &execCmdParams,
        const std::vector<std::string> &argv, const std::vector<std::string> &envp,
        int &exitCodeP, MmProcess &childProcess);
    static int ExecCmdC(const ExecCmdArgv &execCmdArgv, const ExecCmdParams &execCmdParams, int &exitCodeP);
    static int ExecCmdCAsync(const ExecCmdArgv &execCmdArgv, const ExecCmdParams &execCmdParams,
                             MmProcess &childProcess);
    static int ChangeWorkDir(const std::string &fileName);
    static void SetArgEnv(CHAR_PTR_CONST argv[], const int argvCount, CHAR_PTR_CONST envp[],
                          const int envCount, MmArgvEnv &argvEnv);
    static int DoCreateCmdProcess(const std::string &stdoutRedirectFile,
                                  const std::string &fileName,
                                  MmArgvEnv &argvEnv, MmProcess &tid);
    static int WaitProcess(MmProcess process, bool &isExited, int &exitCode, bool hang = true);
    static bool ProcessIsRuning(MmProcess process);
    static std::string JoinPath(const std::vector<std::string> &paths);
    static std::string LeftTrim(const std::string &str, const std::string &trims);
    static std::vector<std::string> Split(const std::string &input_str,
                                          bool filter_out_enabled = false,
                                          const std::string &filter_out = "",
                                          const std::string &pattern = " ");
    static std::string ToUpper(const std::string &value);
    static std::string ToLower(const std::string &value);
    static std::string Trim(const std::string &value);
    static int UsleepInterupt(unsigned long usec);
    static unsigned long long GetClockRealtime();
    static unsigned long long GetClockMonotonicRaw();
    static unsigned long long GetCPUCycleCounter();
    static void GetTime(unsigned long long &startRealtime, unsigned long long &startMono, unsigned long long &cntvct);
    static std::string GenerateStartTime(const unsigned long long startRealtime, const unsigned long long startMono,
        const unsigned long long cntvct);
    static void GetChildDirs(const std::string &dir, bool isRecur, std::vector<std::string> &childDirs);
    static std::string TimestampToTime(const std::string &timestamp, int unit = 1);
    static int GetMac(std::string &macAddress);
    static std::string GetEnvString(const std::string &name);
    static std::string IdeGetHomedir();
    static std::string IdeReplaceWaveWithHomedir(const std::string &path);
    static void EnsureEndsInSlash(std::string& path);
    static VOID_PTR ProfMalloc(size_t size);
    static void ProfFree(VOID_PTR &ptr);
    static bool CheckStringIsNonNegativeIntNum(const std::string &numberStr);
    static bool CheckStringIsValidNatureNum(const std::string &numberStr);
    static bool IsDeviceMapping();
    static std::string GetCoresStr(const std::vector<int> &cores, const std::string &separator = ",");
    static std::string GetEventsStr(const std::vector<std::string> &events, const std::string &separator = ",");
    static void PrintSysErrorMsg();
    static std::string GetSelfPath();
    static std::string CreateTaskId(uint64_t index);
    static std::string CreateResultPath(const std::string &devId);
    static std::string ProfCreateId(uint64_t addr);
    static uint32_t GenerateSignature(CONST_UINT8_PTR data, uint64_t len);
    static std::string ConvertIntToStr(const int interval);
    static int32_t GetPid();
    static void RemoveEndCharacter(std::string &input, const char end = '\n');
    static std::string GetCwdString(void);
    static std::string RelativePathToAbsolutePath(const std::string &path);
    static bool IsSoftLink(const std::string &path);
    template<typename T>
    static std::string Int2HexStr(T number)
    {
        std::stringstream ioss;
        std::string ret;
        ioss << std::hex << number;
        ioss >> ret;
        return ret;
    }
    static bool IsAppName(const std::string paramsName);
    static bool IsClusterRunEnv();
    static int32_t GetRankId();
    static std::vector<std::string> GenEnvPairVec(const std::vector<std::string> &envVec);
    static bool PythonEnvReady();
    static bool AnalysisEnvReady(std::string &msprofPyPath);
    static int CloudAnalyze(const std::string &jobDir);
};

template<class T>
class UtilsStringBuilder {
public:
    std::string Join(const std::vector<T> &elems, const std::string &sep);
};

template<class T>
std::string UtilsStringBuilder<T>::Join(const std::vector<T> &elems, const std::string &sep)
{
    std::stringstream result;

    for (size_t ii = 0; ii < elems.size(); ++ii) {
        if (ii == 0) {
            result << elems[ii];
        } else {
            result << sep;
            result << elems[ii];
        }
    }

    return result.str();
}

int32_t WriteFile(const std::string &absolutePath, const std::string &recordFile, const std::string &profName);

int32_t UnFileLock(FILE *file);

}  // namespace utils
}  // namespace common
}  // namespace dvvp
}  // namespace analysis

#endif