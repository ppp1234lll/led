::.\HEXBIN.bat $K !L @L $L

@echo off
@REM 获取脚本当前目录
set CURRENT_DIR=%~dp0
::echo "CURRENT_DIR:%CURRENT_DIR%"

::创建输出文件夹 Debug
set output_dir=Debug

:: 拼接完整路径（当前目录+文件夹名）
set output_path=%CURRENT_DIR%%output_dir%
::echo "output_path:%output_path%"
if not exist %output_path% md %output_path%

::设置fromelf.exe位置
set exe_location=D:\Keil_v5\ARM\ARMCC\bin\fromelf.exe

::获取工程名
set project_name=atk_h743

::设置.axf文件
set axf_location=%CURRENT_DIR%Output\atk_h743.axf
::echo "axf_location:%axf_location%"

::设置.hex文件所在目录路径
set hex_path=%CURRENT_DIR%Output
::echo "hex_path:%hex_path%..."

::设置输出后的文件名
set output_name=%project_name%
::echo "output_name:%output_name%..."

::echo "Clean old files..."
for /f "delims=" %%A in ('dir /b %output_path%') do del %output_path%\%%A"

@REM GET DIS
::echo "Output dis file: %output_path%\%output_name%.dis"
%exe_location% --text -a -c --output=%output_path%\%output_name%.dis %axf_location% >nul

::将bin文件生成到Debug文件夹  >nul屏蔽成功命令
%exe_location% --bin -o %output_path%\%output_name%.bin %axf_location% >nul

::将hex文件复制到HexBin文件夹
copy %hex_path%\%project_name%.hex %output_path% >nul

::将axf文件复制到HexBin文件夹
copy %axf_location% %output_path% >nul

::将map文件复制到HexBin文件夹
copy %CURRENT_DIR%Output\%project_name%.map %output_path% >nul

@REM 软件版本文件路径
set VERSION_FILE_PATH=%CURRENT_DIR%\INCLUDE\appconfig.h

@REM 获取软件版本
::for /f "tokens=3 delims= " %%a in ('findstr /C:%SOFTWARE_VERSION% %VERSION_FILE_PATH%') do set SW_Ver=%%a
for /f "tokens=3 delims= " %%a in ('findstr /i /c:"#define SOFT_NO_STR" "%VERSION_FILE_PATH%"') do (
    set sw_ver=%%a
)
:: 去除双引号（如把"V1.2.0"转为V1.2.0）
set sw_ver=%sw_ver:"=%
::echo "sw_ver...%sw_ver%"
 
@REM 获取软件版本
for /f "tokens=3 delims= " %%b in ('findstr /i /c:"#define HARD_NO_STR" "%VERSION_FILE_PATH%"') do (
    set hw_ver=%%b
)
set hw_ver=%hw_ver:"=%
::echo "hw_ver...%hw_ver%"

::设置重命名的文件名
set rename_name=%hw_ver%-%sw_ver%
::echo "rename_name..%rename_name%.hex"

::将hex文件重命名
::echo "Output Hex file: ..\HexBin\%rename_name%.hex"
ren %output_path%\%output_name%.hex %rename_name%.hex >nul
::将BIN文件重命名
::echo "Output Bin file: ..\HexBin\%rename_name%.hex"
ren %output_path%\%output_name%.bin %rename_name%.bin >nul









