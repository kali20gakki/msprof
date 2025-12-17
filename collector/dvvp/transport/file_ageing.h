/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
#ifndef ANALYSIS_DVVP_STREAMIO_COMMON_FILE_AGEING_H
#define ANALYSIS_DVVP_STREAMIO_COMMON_FILE_AGEING_H

#include <map>
#include <list>
#include "message/prof_params.h"
#include "queue/bound_queue.h"
#include "singleton/singleton.h"
#include "thread/thread.h"

namespace analysis {
namespace dvvp {
namespace transport {

class FileAgeing {
public:
    FileAgeing(const std::string &storageDir, const std::string &storageLimit);
    ~FileAgeing();

public:
    int Init();
    bool IsNeedAgeingFile();
    void RemoveAgeingFile();
    void AppendAgeingFile(const std::string &filePath, const std::string &doneFilePath,
                            uint64_t fileSize, uint64_t doneFileSize);
private:
    struct ToBeAgedFile {
        bool isValid;
        bool isNeedPair;
        bool isPaired;
        uint64_t fileSize;
        uint64_t doneFileSize;
        std::string fileName;
        std::string doneFileName;
        std::string filePath;
        std::string doneFilePath;
        std::string fileCountTag;   // file name cut the slice num
        std::string pairFileTag;
    };

private:
    bool IsNoAgeingFile(const std::string &fileName) const;
    bool IsLastFile(const std::string &fileCountTag);
    uint64_t GetStorageLimit() const;
    int32_t CutSliceNum(const std::string &fileName, std::string &cutFileName) const;
    void PairInsertList(const std::string &pairedFileTag, ToBeAgedFile &insertedFile);
    void RemoveFile(const ToBeAgedFile &file, uint64_t &removeFileSize);
    void PrintAgeingFile() const;

private:
    bool inited_;
    uint64_t overThdSize_;
    uint64_t storagedFileSize_;
    uint64_t storageVolumeUpThd_;
    uint64_t storageVolumeDownThd_;
    std::string storageDir_;
    std::string storageLimit_;
    std::vector<std::string> noAgeingFile_;         // can not be aged file name
    std::list<ToBeAgedFile> ageingFileList_;        // to be ageing file list
    std::map<std::string, uint32_t> fileCount_;     // <file name, file num>
    std::map<std::string, uint32_t> unPairCount_;   // <pair tag, file num>
};
}
}
}
#endif  // ANALYSIS_DVVP_STREAMIO_COMMON_FILE_AGEING_H
