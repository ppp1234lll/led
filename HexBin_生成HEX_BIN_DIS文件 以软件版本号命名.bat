
@echo off
::创建输出文件夹 HexBin
if not exist HexBin (mkdir HexBin)
::设置fromelf.exe位置
set exe_location=%~1ARM\ARMCC\bin\fromelf.exe
::设置.axf文件的位置
set obj_location=%2
::获取工程名
set project_name=%3
::设置.axf文件所在目录路径
set obj_path=%4
::设置输出后的文件名
set output_name=%project_name%

::echo "Clean old files..."
for /f "delims=" %%A in ('dir /b ..\HexBin') do del ..\HexBin\%%A"

@REM GET DIS
echo "Output dis file: ..\OBJ\%output_file_name%.dis"
%exe_location% --text -a -c --output=..\OBJ\%output_name%.dis %obj_location% >nul

::将bin文件生成到HexBin文件夹  >nul屏蔽成功命令
%exe_location% --bin -o ..\HexBin\%output_name%.bin %obj_location% >nul
::将hex文件重命名
ren %obj_path%%project_name%.hex %output_name%.hex >nul
::将hex文件复制到HexBin文件夹
move %obj_path%%output_name%.hex ..\HexBin >nul

@REM 软件版本文件路径
set VERSION_FILE_PATH=..\INCLUDE\appconfig.h
@REM 软件版本字符串的格式
set SOFTWARE_VERSION="#define SOFT_NO_STR"
@REM 获取软件版本
for /f "tokens=3 delims= " %%i in ('findstr /C:%SOFTWARE_VERSION% %VERSION_FILE_PATH%') do set SW_Ver=%%i
@REM 去除字符串两端的双引号，如果代码中定义版本字符的不包含双引号，需要去除此行

::设置重命名的文件名
set rename_name=%SW_Ver%

::将hex文件重命名
echo "Output Hex file: ..\HexBin\%rename_name%.hex"
ren ..\HexBin\%output_name%.hex %rename_name%.hex >nul
::将BIN文件重命名
echo "Output Bin file: ..\HexBin\%rename_name%.hex"
ren ..\HexBin\%output_name%.bin %rename_name%.bin >nul




