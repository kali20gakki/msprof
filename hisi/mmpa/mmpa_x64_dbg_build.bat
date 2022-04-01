rem Copyright (C) Huawei Technologies Co., Ltd. 2012-2019. All rights reserved.
rem compile mmpa for  windows
rem Create 2018-8-8

@echo off

set topdir=%~dp0

if not exist "%topdir%\..\out\matebook\windows\release_imgs" (
    md "%topdir%\..\out\matebook\windows\release_imgs"
)


set path=%path%;D:\VS2017\MSBuild\15.0\Bin\
msbuild dll-mmpa.sln /t:rebuild /p:configuration=debug /p:platform=x64

copy "%topdir%\x64\Debug\dll-mmpa.dll" "%topdir%\..\out\matebook\windows\release_imgs"
copy "%topdir%\x64\Debug\dll-mmpa.lib" "%topdir%\..\out\matebook\windows\release_imgs"


