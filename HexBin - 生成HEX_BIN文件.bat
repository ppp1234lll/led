
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
::将bin文件生成到HexBin文件夹  >nul屏蔽成功命令
%exe_location% --bin -o .\HexBin\%output_name%.bin %obj_location% >nul
::将hex文件重命名
ren %obj_path%%project_name%.hex %output_name%.hex >nul
::将hex文件复制到HexBin文件夹
move %obj_path%%output_name%.hex .\HexBin >nul

