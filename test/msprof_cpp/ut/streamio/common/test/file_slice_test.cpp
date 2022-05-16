#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "securec.h"
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/vfs.h>
#include <locale>
#include <errno.h>
#include <algorithm>
#include <fstream>
//mac
#include <net/if.h>
#include <sys/prctl.h>
#include "file_slice.h"
#include "message/prof_params.h"
#include "thread/thread_pool.h"

using namespace analysis::dvvp::common::thread;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::transport;

class COMMON_FILE_SLICE_TEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
        GlobalMockObject::verify();
    }
private:
};

TEST_F(COMMON_FILE_SLICE_TEST, GetSliceKey) {
    std::string dir = "/tmp";
    std::string limit = "500MB";
    FileSlice wfTransport(128, dir, limit);
    std::string fileName = "hwts.log";
    std::string key = wfTransport.GetSliceKey(dir, fileName);
    EXPECT_STREQ("/tmp/hwts.log.slice_", key.c_str());
}

TEST_F(COMMON_FILE_SLICE_TEST, Init_failed) {
    std::string dir = "";
    std::string limit = "500MB";
    FileSlice wfTransport(128, dir, limit);
    EXPECT_EQ(PROFILING_FAILED, wfTransport.Init());

    FileSlice wfTransport1(128, "/tmp/../../aaa/b", limit);

}

TEST_F(COMMON_FILE_SLICE_TEST, SetChunkTime) {
    
    std::string dir = "/tmp";
    std::string limit = "500MB";
    FileSlice wfTransport(128, dir, limit);
    wfTransport.Init();
    int ret = wfTransport.SetChunkTime("", 0, 1);
    EXPECT_EQ(PROFILING_FAILED, ret);

    MOCKER(analysis::dvvp::common::utils::Utils::IsFileExist)
        .stubs()
        .will(returnValue(false));
    
    MOCKER(analysis::dvvp::common::utils::Utils::GetFileSize)
        .stubs()
        .will(returnValue(0));

    ret = wfTransport.SetChunkTime("hwts.log.slice_", 0, 1);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(COMMON_FILE_SLICE_TEST, WriteToLocalFiles) {

    std::string key = "hwts.log.slice_";
    char *data = "test";
    int dataLen = strlen(data);
    int offset = -1;
    bool isLastChunk = true;
    std::string dir = "/tmp";
    FileSlice wfTransport(128, dir, "200MB");
    wfTransport.Init();

    MOCKER(analysis::dvvp::common::utils::Utils::IsFileExist)
        .stubs()
        .will(returnValue(true))
        .then(returnValue(false))
        .then(returnValue(false))
        .then(returnValue(true));

    MOCKER(analysis::dvvp::common::utils::Utils::GetFileSize)
        .stubs()
        .will(returnValue((long long)127 * 1024 + 1))
        .then(returnValue((long long)128 * 1024 + 1));
    
    MOCKER_CPP(&analysis::dvvp::transport::FileSlice::CreateDoneFile)
            .stubs()
            .will(returnValue(false))
            .then(returnValue(true));

    //key.length() = 0
    int ret = wfTransport.WriteToLocalFiles("", data, dataLen, offset, isLastChunk);
    EXPECT_EQ(PROFILING_FAILED, ret);
    //Failed to create file:hwts.log.slice_
    ret = wfTransport.WriteToLocalFiles(key, data, dataLen, offset, isLastChunk);
    EXPECT_EQ(PROFILING_FAILED, ret);

    //open failed
    isLastChunk = false;
    key = "/tmp/not_exist_dir/hwts.log";
    ret = wfTransport.WriteToLocalFiles(key, data, dataLen, offset, isLastChunk);
    EXPECT_EQ(PROFILING_FAILED, ret);

    //diskFull return true
    key = "hwts.log.slice_";
    ret = wfTransport.WriteToLocalFiles(key, data, dataLen, offset, isLastChunk);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    
    //open file failed 
    key = "/tmp/not_exist_dir/hwts.log";
    ret = wfTransport.WriteToLocalFiles(key, data, dataLen, offset, isLastChunk);
    EXPECT_EQ(PROFILING_FAILED, ret);

    //create done file failed
    key = "hwts.log.slice_";
    ret = wfTransport.WriteToLocalFiles(key, data, dataLen, 0, isLastChunk);
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    ret = wfTransport.WriteToLocalFiles(key, data, dataLen, 0, isLastChunk);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(COMMON_FILE_SLICE_TEST, SaveDataToLocalFiles) {

    std::string dir = "/home/test";
    std::string limit = "500MB";
    
    std::shared_ptr<analysis::dvvp::proto::FileChunkReq> message(
        new analysis::dvvp::proto::FileChunkReq());
    std::shared_ptr<google::protobuf::Message> fileChunkReq = message;
  
    message->mutable_hdr()->set_job_ctx("fake_job_ctx_json");
    message->set_filename("");
    message->set_offset(-1); 
    message->set_chunk("123", 3);
    message->set_chunksizeinbytes(3);
    message->set_islastchunk(false);
    message->set_needack(false);

    std::string invalidHomeDir = "";
    std::string homeDir = "/home/test";
    MOCKER(analysis::dvvp::common::utils::Utils::IdeReplaceWaveWithHomedir)
        .stubs()
        .will(returnValue(invalidHomeDir))
        .then(returnValue(homeDir));
    
    MOCKER(analysis::dvvp::common::utils::Utils::CreateDir)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    
    FileSlice wfTransport(128, dir, limit);
    wfTransport.Init();

    //filename length is 0
    int ret = wfTransport.SaveDataToLocalFiles(message);
    EXPECT_EQ(PROFILING_FAILED, ret);
    
    //ide_replace_wave_with_home is invalid
    message->set_filename("__PROFILER_HOST_ST__");
    ret = wfTransport.SaveDataToLocalFiles(message);
    EXPECT_EQ(PROFILING_FAILED, ret);

    //CreateDir failed
    ret = wfTransport.SaveDataToLocalFiles(message);
    EXPECT_EQ(PROFILING_FAILED, ret);

    //fake job_ctx_json
    message->mutable_hdr()->set_job_ctx("fake_job_ctx_json");
    ret = wfTransport.SaveDataToLocalFiles(message);
    EXPECT_EQ(PROFILING_FAILED, ret);

    std::string invalidKey = "";
    std::string key = "hwts.log.slice_";
    MOCKER_CPP(&analysis::dvvp::transport::FileSlice::GetSliceKey)
            .stubs()
            .will(returnValue(invalidKey))
            .then(returnValue(key));
    
    MOCKER_CPP(&analysis::dvvp::transport::FileSlice::SetChunkTime)
            .stubs()
            .will(returnValue(PROFILING_FAILED))
            .then(returnValue(PROFILING_SUCCESS));
    
    MOCKER_CPP(&analysis::dvvp::transport::FileSlice::WriteToLocalFiles)
            .stubs()
            .will(returnValue(PROFILING_FAILED))
            .then(returnValue(PROFILING_SUCCESS));

    //GetSliceKey return length 0
    analysis::dvvp::message::JobContext job_ctx;
    job_ctx.job_id = "2";
    job_ctx.dev_id = "1";
    message->mutable_hdr()->set_job_ctx(job_ctx.ToString());
    ret = wfTransport.SaveDataToLocalFiles(message);
    EXPECT_EQ(PROFILING_FAILED, ret);

    //SetChunkTime return Failed
    message->set_filename("/home/test/test.log");
    ret = wfTransport.SaveDataToLocalFiles(message);
    EXPECT_EQ(PROFILING_FAILED, ret);    

    //WriteToLocalFiles Failed
    ret = wfTransport.SaveDataToLocalFiles(message);
    EXPECT_EQ(PROFILING_FAILED, ret);
}

TEST_F(COMMON_FILE_SLICE_TEST, CreateDoneFile) {
    GlobalMockObject::verify();
    MOCKER(analysis::dvvp::common::utils::Utils::GetFileSize)
        .stubs()
        .will(returnValue((long long)1000));
    
    std::string absolutePath = "";
    std::string fileSize = "1000";
    std::string startTime = "0";
    std::string endTime = "1";
    std::string limit = "500MB";
    std::string dir = "/tmp";
    FileSlice wfTransport(128, dir, limit);
    wfTransport.Init();
    bool ret = wfTransport.CreateDoneFile(absolutePath, fileSize, startTime, endTime, absolutePath);
    EXPECT_EQ(true, ret);
    
    std::string fileName = "hwts.log";
    absolutePath = "hwts.log.slice_";
    MOCKER(analysis::dvvp::common::utils::Utils::IsFileExist)
        .stubs()
        .will(returnValue(false));
    wfTransport.GetSliceKey(dir, fileName);
    wfTransport.SetChunkTime(dir+fileName+".slice_", 0, 0);
    ret = wfTransport.CreateDoneFile(absolutePath, fileSize, startTime, endTime, "/home/test/hwts.log.slice_0");
    EXPECT_EQ(true, ret);    

    MOCKER(analysis::dvvp::common::utils::Utils::IsFileExist)
        .stubs()
        .will(returnValue(true));
    wfTransport.GetSliceKey(dir, fileName);
    wfTransport.SetChunkTime(dir+fileName+".slice_", 0, 0);
    ret = wfTransport.CreateDoneFile(absolutePath, fileSize, startTime, endTime, "/home/test/hwts.log.slice_0");
    EXPECT_EQ(true, ret);
}

TEST_F(COMMON_FILE_SLICE_TEST, CreateDoneFileForFail) {
    std::string absolutePath = "";
    std::string fileSize = "1000";
    std::string startTime = "0";
    std::string endTime = "1";
    std::string dir = "/tmp";
    std::string limit = "500MB";

    FileSlice wfTransport(128, dir, limit);
    wfTransport.Init();
    bool ret = wfTransport.CreateDoneFile(absolutePath, fileSize, startTime, endTime, absolutePath);
    EXPECT_EQ(true, ret);
    
    std::string fileName = "hwts.log";
    absolutePath = "hwts.log.slice_";
    MOCKER(analysis::dvvp::common::utils::Utils::IsFileExist)
        .stubs()
        .will(returnValue(false));
    wfTransport.GetSliceKey(dir, fileName);
    wfTransport.SetChunkTime(dir+fileName+".slice_", 0, 0);
    ret = wfTransport.CreateDoneFile(absolutePath, fileSize, startTime, endTime, "/home/test/hwts.log.slice_0");
    EXPECT_EQ(true, ret);    

    MOCKER(analysis::dvvp::common::utils::Utils::IsFileExist)
        .stubs()
        .will(returnValue(true));
    wfTransport.GetSliceKey(dir, fileName);
    wfTransport.SetChunkTime(dir+fileName+".slice_", 0, 0);
    ret = wfTransport.CreateDoneFile(absolutePath, fileSize, startTime, endTime, "/home/test/hwts.log.slice_0");
    EXPECT_EQ(true, ret);
}

TEST_F(COMMON_FILE_SLICE_TEST, CreateDoneFileForOpenFail) {
    std::string absolutePath = "./data/host_start.log";
    std::string dir = "/home/test/";
    std::string limit = "500MB";

    FileSlice wfTransport(128, dir, limit);
    wfTransport.Init();
    bool ret = wfTransport.CreateDoneFile(absolutePath, "1000", "", "", "");
    EXPECT_EQ(false, ret);
}

TEST_F(COMMON_FILE_SLICE_TEST, FileSliceFlush) {
    
    std::string dir = "/home/test";
    std::string limit = "500MB";

    FileSlice wfTransport(128, dir, limit);
    wfTransport.Init();
    std::string fileName = "test.log";
    wfTransport.GetSliceKey(dir, fileName);

    MOCKER_CPP(&analysis::dvvp::transport::FileSlice::CreateDoneFile)
            .stubs()
            .will(returnValue(false))
            .then(returnValue(true));

    MOCKER(analysis::dvvp::common::utils::Utils::IsFileExist)
        .stubs()
        .will(returnValue(true));
    
    MOCKER(analysis::dvvp::common::utils::Utils::GetFileSize)
        .stubs()
        .will(returnValue((long long)128 * 1024));

    int ret = wfTransport.FileSliceFlush();
    EXPECT_EQ(false, ret);    
    ret = wfTransport.FileSliceFlush();
    EXPECT_EQ(true, ret);    
}

TEST_F(COMMON_FILE_SLICE_TEST, FileSliceFlushPolymorphism) {
    
    std::string dir = "/home/test/1234";
    std::string limit = "500MB";
    FileSlice wfTransport(128, dir, limit);

    wfTransport.Init();
    
    std::string fileName = "test.log";
    wfTransport.GetSliceKey(dir, fileName);

    MOCKER_CPP(&analysis::dvvp::transport::FileSlice::CreateDoneFile)
            .stubs()
            .will(returnValue(false))
            .then(returnValue(true));

    MOCKER(analysis::dvvp::common::utils::Utils::IsFileExist)
        .stubs()
        .will(returnValue(true));
    
    MOCKER(analysis::dvvp::common::utils::Utils::GetFileSize)
        .stubs()
        .will(returnValue((long long)128 * 1024));

    std::string jobIDRelative = "1234";
    std::string devID = "0";
    std::string fileSliceName = "";
    fileSliceName.append(".").append(devID).append(".slice_");
    wfTransport.GetSliceKey(dir, fileSliceName);
    
    int ret = wfTransport.FileSliceFlushByJobID(jobIDRelative, devID);
    EXPECT_EQ(PROFILING_FAILED, ret);
    ret = wfTransport.FileSliceFlushByJobID(jobIDRelative, devID);
    EXPECT_EQ(PROFILING_SUCCESS, ret);    
}

TEST_F(COMMON_FILE_SLICE_TEST, WriteCtrlDataToFile) {
    std::string absolutePath = "/tmp";
    std::string data = "test";
    std::string limit = "500MB";
    FileSlice wfTransport(128, "/tmp", limit);
    remove("/tmp/ctrl_data.txt");
    EXPECT_EQ(PROFILING_SUCCESS, wfTransport.Init());
    EXPECT_EQ(PROFILING_FAILED, wfTransport.WriteCtrlDataToFile("non_exist.txt", data, 0));
    wfTransport.WriteCtrlDataToFile(absolutePath, data, data.size());
    wfTransport.WriteCtrlDataToFile(absolutePath, "", data.size());
    wfTransport.WriteCtrlDataToFile("/tmp/ctrl_data.txt", "test", data.size());
    remove("/tmp/ctrl_data.txt");

    
}