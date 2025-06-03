/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : db_config.h
 * Description        : DB相关常量映射
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */

#ifndef ANALYSIS_VIEWER_DATABASE_DATABASE_H
#define ANALYSIS_VIEWER_DATABASE_DATABASE_H

#include <vector>
#include <unordered_map>
#include <string>

#include "analysis/csrc/infrastructure/db/include/connection.h"

namespace Analysis {
namespace Infra {

using TableColumns = std::vector<TableColumn>;
const std::string SQL_TEXT_TYPE = "TEXT";
const std::string SQL_INTEGER_TYPE = "INTEGER";
const std::string SQL_NUMERIC_TYPE = "NUMERIC";
const std::string SQL_REAL_TYPE = "REAL";

// DB中Table映射基类
class Database {
public:
    Database() = default;
    // 获取该DB实际落盘的文件名
    std::string GetDBName() const;
    // 获取该DB指定表中字段名
    TableColumns GetTableCols(const std::string &tableName);

protected:
    std::string dbName_;
    std::unordered_map<std::string, TableColumns> tableColNames_;
};

class ApiEventDB : public Database {
public:
    ApiEventDB();
};

class RuntimeDB : public Database {
public:
    RuntimeDB();
};

class GEInfoDB : public Database {
public:
    GEInfoDB();
};

class HashDB : public Database {
public:
    HashDB();
};

class GeModelInfoDB : public Database {
public:
    GeModelInfoDB();
};

class HCCLDB : public Database {
public:
    HCCLDB();
};

class RtsTrackDB : public Database {
public:
    RtsTrackDB();
};

class AscendTaskDB : public Database {
public:
    AscendTaskDB();
};


class TraceDB : public Database {
public:
    TraceDB();
};

class HCCLSingleDeviceDB : public Database {
public:
    HCCLSingleDeviceDB();
};

class NpuMemDB : public Database {
public:
    NpuMemDB();
};

class NpuModuleMemDB : public Database {
public:
    NpuModuleMemDB();
};

class TaskMemoryDB : public Database {
public:
    TaskMemoryDB();
};

class NicDB : public Database {
public:
    NicDB();
};

class NicReceiveSendDB : public Database {
public:
    NicReceiveSendDB();
};

class RoceDB : public Database {
public:
    RoceDB();
};

class RoceReceiveSendDB : public Database {
public:
    RoceReceiveSendDB();
};

class HBMDB : public Database {
public:
    HBMDB();
};

class DDRDB : public Database {
public:
    DDRDB();
};

class LLCDB : public Database {
public:
    LLCDB();
};

class AicoreDB : public Database {
public:
    AicoreDB();
};

class AiVectorCoreDB : public Database {
public:
    AiVectorCoreDB();
};

class AccPmuDB : public Database {
public:
    AccPmuDB();
};

class SocProfilerDB : public Database {
public:
    SocProfilerDB();
};

class PCIeDB : public Database {
public:
    PCIeDB();
};

class HCCSDB : public Database {
public:
    HCCSDB();
};

class NetDevStatsDB : public Database {
public:
    NetDevStatsDB();
};

class FreqDB : public Database {
public:
    FreqDB();
};

class MsprofTxDB : public Database {
public:
    MsprofTxDB();
};

class StepTraceDB : public Database {
public:
    StepTraceDB();
};


class KfcInfo : public Database {
public:
    KfcInfo();
};

class Mc2CommInfoDB : public Database {
public:
    Mc2CommInfoDB();
};

class HostCpuUsage : public Database {
public:
    HostCpuUsage();
};

class HostMemUsage : public Database {
public:
    HostMemUsage();
};

class HostDiskUsage : public Database {
public:
    HostDiskUsage();
};

class HostNetworkUsage : public Database {
public:
    HostNetworkUsage();
};

class HostRuntimeApi : public Database {
public:
    HostRuntimeApi();
};

class ChipTransDB : public Database {
public:
    ChipTransDB();
};

class GeLogicStreamDB : public Database {
public:
    GeLogicStreamDB();
};

class SioDB : public Database {
public:
    SioDB();
};

class QosDB : public Database {
public:
    QosDB();
};
} // namespace Infra
} // namespace Analysis
#endif // ANALYSIS_VIEWER_DATABASE_DATABASE_H
