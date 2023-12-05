/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : file_utest.cpp
 * Description        : File UT
 * Author             : msprof team
 * Creation Date      : 2023/11/15
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "file.h"
#include "error_code.h"

using namespace Analysis;
using namespace Analysis::Utils;

class FileUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        GlobalMockObject::verify();
        FileWriter fw("test_file");  // 创建test_file文件
    }
    virtual void TearDown()
    {
        File::DeleteFile("test_file");  // 删除test_file文件
    }
};

TEST_F(FileUTest, TestChmod)
{
    const mode_t mode = 0640;
    EXPECT_FALSE(File::Chmod("", mode));
    EXPECT_FALSE(File::Chmod("test_file_not_exist", mode));
    EXPECT_TRUE(File::Chmod("test_file", mode));
}

TEST_F(FileUTest, TestExist)
{
    EXPECT_FALSE(File::Exist(""));
    EXPECT_FALSE(File::Exist("test_file_not_exist"));
    EXPECT_TRUE(File::Exist("test_file"));
}

TEST_F(FileUTest, TestIsSoftLink)
{
    EXPECT_EQ(0, symlink("test_file", "test_file_soft_link"));  // 创建软连接
    EXPECT_FALSE(File::IsSoftLink(""));
    EXPECT_FALSE(File::IsSoftLink("test_file_not_exist"));
    EXPECT_FALSE(File::IsSoftLink("test_file"));
    EXPECT_TRUE(File::IsSoftLink("test_file_soft_link"));
    EXPECT_TRUE(File::DeleteFile("test_file_soft_link"));  // 删除test_file_soft_link软连接
}

TEST_F(FileUTest, TestIsFile)
{
    EXPECT_FALSE(File::IsFile(""));
    EXPECT_FALSE(File::IsFile("test_file_not_exist"));
    EXPECT_TRUE(File::IsFile("test_file"));
}

TEST_F(FileUTest, TestSize)
{
    EXPECT_EQ(0, File::Size(""));
    EXPECT_EQ(0, File::Size("test_file_not_exist"));
    EXPECT_EQ(0, File::Size("test_file"));
}

TEST_F(FileUTest, TestCreateDir)
{
    EXPECT_FALSE(File::CreateDir(""));
    EXPECT_TRUE(File::CreateDir("test_dir"));  // 创建目录成功返回true
    EXPECT_TRUE(File::CreateDir("test_dir"));  // 创建已存在的目录返回true
    EXPECT_EQ(0, symlink("test_file", "test_file_soft_link"));
    EXPECT_FALSE(File::CreateDir("test_file_soft_link"));
    EXPECT_TRUE(File::DeleteFile("test_file_soft_link"));
    EXPECT_EQ(0, rmdir("test_dir"));
}

TEST_F(FileUTest, TestRemoveDir)
{
    // 测试删除不存在的文件夹
    EXPECT_FALSE(File::RemoveDir("test_dir_not_exist", 0));
    // 测试删除软连接
    EXPECT_EQ(0, symlink("test_link_file", "test_file_soft_link"));
    EXPECT_FALSE(File::RemoveDir("test_link_file", 0));
    EXPECT_TRUE(File::DeleteFile("test_file_soft_link"));
    // 测试删除嵌套文件夹
    EXPECT_TRUE(File::CreateDir("test_dir"));
    EXPECT_TRUE(File::CreateDir("test_dir/dir"));
    EXPECT_TRUE(File::CreateDir("test_dir/directory"));
    EXPECT_TRUE(File::CreateDir("test_dir/directory_done"));
    EXPECT_TRUE(File::RemoveDir("test_dir", 0));
}

TEST_F(FileUTest, TestPathJoinShouldReturnJoinedPath)
{
    EXPECT_EQ("", File::PathJoin({}));
    EXPECT_EQ("/test_dir//test_subdir/test_file", File::PathJoin({"/test_dir/", "test_subdir", "test_file"}));
}

TEST_F(FileUTest, TestGetFilesWithPrefixShouldReturn2SubFilesWhenGetFileMatchPrefix)
{
    EXPECT_EQ(0, File::GetFilesWithPrefix("", "").size());

    EXPECT_TRUE(File::CreateDir("test_dir"));
    EXPECT_TRUE(File::CreateDir("test_dir/dir"));
    EXPECT_TRUE(File::CreateDir("test_dir/directory"));
    EXPECT_TRUE(File::CreateDir("test_dir/directory_done"));
    const size_t fileNum = 2;
    EXPECT_EQ(fileNum, File::GetFilesWithPrefix("test_dir", "directory").size());
    EXPECT_EQ(0, rmdir("test_dir/directory_done"));
    EXPECT_EQ(0, rmdir("test_dir/directory"));
    EXPECT_EQ(0, rmdir("test_dir/dir"));
    EXPECT_EQ(0, rmdir("test_dir"));
}

TEST_F(FileUTest, TestFilterFileWithSuffixShouldReturn2FilesWhenFilterSuffix)
{
    std::vector<std::string> files = {
        "test1",
        "test2",
        "test3",
        "test1.done",
        "test2.done",
    };
    const size_t fileNumNotFiltered = 5;
    EXPECT_EQ(fileNumNotFiltered, File::FilterFileWithSuffix(files, "").size());
    const size_t fileNumFiltered = 3;
    EXPECT_EQ(fileNumFiltered, File::FilterFileWithSuffix(files, "done").size());
}

TEST_F(FileUTest, TestFileCheck)
{
    EXPECT_FALSE(File::Check(""));
    const int pathDepth = 350;
    std::vector<std::string> paths(pathDepth, "test");
    std::string path = File::PathJoin(paths);
    EXPECT_FALSE(File::Check(path));
    EXPECT_EQ(0, symlink("test_file", "test_file_soft_link"));  // 创建软连接
    EXPECT_FALSE(File::Check("test_file_soft_link"));
    EXPECT_TRUE(File::Check("test_file"));
    EXPECT_TRUE(File::DeleteFile("test_file_soft_link"));  // 删除test_file_soft_link软连接
}

TEST_F(FileUTest, TestFileCheckShouldReturnFalseWhenFileSizeToolarge)
{
    const uint32_t maxReadBytes = 64 * 1024 * 1024;
    MOCKER_CPP(&File::Size).stubs()
        .will(returnValue(maxReadBytes + 1));
    EXPECT_FALSE(File::Check("test_file"));
}

TEST_F(FileUTest, TestFileReaderCheckShouldReturnFalseWhenNotFile)
{
    EXPECT_FALSE(FileReader::Check(""));
    EXPECT_FALSE(FileReader::Check("test_file_not_exist"));
}

TEST_F(FileUTest, TestFileReaderCheckShouldReturnFalseWhenFileNoReadAccess)
{
    MOCKER_CPP(&File::Access).stubs()
        .will(returnValue(false));
    EXPECT_FALSE(FileReader::Check("test_file"));
}

TEST_F(FileUTest, TestFileReaderCheckShouldReturnTrueWhenFileCheckSuccess)
{
    EXPECT_TRUE(FileReader::Check("test_file"));
}

TEST_F(FileUTest, TestFileReaderOpenShouldNotOpenWhenEmptyPath)
{
    FileReader fd;
    fd.Open("");
    EXPECT_FALSE(fd.IsOpen());
}

TEST_F(FileUTest, TestFileReaderOpenShouldOpenWhenAlreadyOpened)
{
    MOCKER_CPP(&FileReader::IsOpen).stubs()
        .will(returnValue(true));
    FileReader fd;
    fd.Open("test_file");
    EXPECT_TRUE(fd.IsOpen());
}

TEST_F(FileUTest, TestFileReaderOpenShouldNotOpenWhenOpenedFailed)
{
    MOCKER_CPP(&FileReader::Check).stubs()
        .will(returnValue(true));
    FileReader fd;
    fd.Open("test_file_not_exist");
    EXPECT_FALSE(fd.IsOpen());
}

TEST_F(FileUTest, TestFileReaderOpenShouldOpenWhenOpenedSuccess)
{
    FileReader fd;
    fd.Open("test_file");
    EXPECT_TRUE(fd.IsOpen());
}

TEST_F(FileUTest, TestReadBinaryShouleReturnErrorWhenFileNotOpen)
{
    FileReader fd;
    std::stringstream ss;
    EXPECT_EQ(ANALYSIS_ERROR, fd.ReadBinary(ss));
}

TEST_F(FileUTest, TestReadBinaryShouleReturnOKWhenReadSuccess)
{
    FileReader fd;
    fd.Open("test_file");
    std::stringstream ss;
    EXPECT_EQ(ANALYSIS_OK, fd.ReadBinary(ss));
}

TEST_F(FileUTest, TestReadTextShouleReturnErrorWhenFileNotOpen)
{
    FileReader fd;
    std::vector<std::string> text;
    EXPECT_EQ(ANALYSIS_ERROR, fd.ReadText(text));
}

TEST_F(FileUTest, TestReadTextShouleReturnOKWhenReadSuccess)
{
    FileReader fd;
    fd.Open("test_file");
    std::vector<std::string> text;
    EXPECT_EQ(ANALYSIS_OK, fd.ReadText(text));
}

TEST_F(FileUTest, TestReadJsonShouleReturnErrorWhenFileNotOpen)
{
    FileReader fd;
    nlohmann::json content;
    EXPECT_EQ(ANALYSIS_ERROR, fd.ReadJson(content));
}

TEST_F(FileUTest, TestReadJsonShouleReturnErrorWhenParseJsonFailed)
{
    FileReader fd;
    fd.Open("test_file");
    nlohmann::json content;
    EXPECT_EQ(ANALYSIS_ERROR, fd.ReadJson(content));
}

TEST_F(FileUTest, TestReadJsonShouleReturnOKWhenParseJsonSuccess)
{
    FileReader fd;
    FileWriter jsonWriter("test_file");
    ASSERT_TRUE(jsonWriter.IsOpen());
    jsonWriter.WriteText("{}");
    jsonWriter.Close();
    fd.Open("test_file");
    ASSERT_TRUE(fd.IsOpen());
    nlohmann::json content;
    EXPECT_EQ(ANALYSIS_OK, fd.ReadJson(content));
}

TEST_F(FileUTest, TestFileWriterCheckShouldReturnTrueWhenFileNotExist)
{
    EXPECT_FALSE(FileWriter::Check(""));
    EXPECT_TRUE(FileWriter::Check("test_file_not_exist"));
}

TEST_F(FileUTest, TestFileWriterCheckShouldReturnFalseWhenPathIsDir)
{
    EXPECT_TRUE(File::CreateDir("test_dir"));
    EXPECT_FALSE(FileWriter::Check("test_dir"));
    EXPECT_EQ(0, rmdir("test_dir"));
}

TEST_F(FileUTest, TestFileWriterCheckShouldReturnFalseWhenFileNoWriteAccess)
{
    MOCKER_CPP(&File::Exist).stubs()
        .will(returnValue(true));
    MOCKER_CPP(&File::Access).stubs()
        .will(returnValue(false));
    EXPECT_FALSE(FileWriter::Check("test_file"));
}

TEST_F(FileUTest, TestFileWriterCheckShouldReturnTrueWhenFileCheckSuccess)
{
    EXPECT_TRUE(FileWriter::Check("test_file"));
}

TEST_F(FileUTest, TestFileWriterOpenShouldNotOpenWhenEmptyPath)
{
    FileWriter fw;
    fw.Open("");
    EXPECT_FALSE(fw.IsOpen());
}

TEST_F(FileUTest, TestFileWriterOpenShouldOpenWhenAlreadyOpened)
{
    MOCKER_CPP(&FileWriter::IsOpen).stubs()
        .will(returnValue(true));
    FileWriter fw;
    fw.Open("test_file");
    EXPECT_TRUE(fw.IsOpen());
}

TEST_F(FileUTest, TestFileWriterOpenShouldOpenWhenOpenedSuccess)
{
    FileWriter fd;
    fd.Open("test_file");
    EXPECT_TRUE(fd.IsOpen());
}

TEST_F(FileUTest, TestWriteText)
{
    FileWriter fd("test_file");
    ASSERT_TRUE(fd.IsOpen());
    fd.WriteText("test");
}

TEST_F(FileUTest, TestGetOriginDataShouldReturn2ValidFileWhen2ValidAnd2Invalid)
{
    // 创建test_file文件
    FileWriter fw0("/tmp/prefix_test_slice_0");
    FileWriter fw1("/tmp/prefix2_test_slice_1");
    FileWriter fw2("/tmp/prefix_test_slice_0.done");
    FileWriter fw3("/tmp/prefix_test_slice_0.complete");
    fw0.Close();
    fw1.Close();
    fw2.Close();
    fw3.Close();

    int expectRes = 2;
    auto files = File::GetOriginData("/tmp", {"prefix_test_slice", "prefix2_test_slice"}, {"done", "complete"});
    EXPECT_EQ(expectRes, files.size());

    File::DeleteFile("/tmp/prefix_test_slice_0");
    File::DeleteFile("/tmp/prefix2_test_slice_1");
    File::DeleteFile("/tmp/prefix_test_slice_0.done");
    File::DeleteFile("/tmp/prefix_test_slice_0.complete");
}
