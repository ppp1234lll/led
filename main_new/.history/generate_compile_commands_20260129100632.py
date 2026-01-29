#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Generate compile_commands.json file for clangd
"""

import os
import json

# Project root directory
PROJECT_ROOT = os.path.dirname(os.path.abspath(__file__))

# Source file extensions to look for
SOURCE_EXTENSIONS = ['.c', '.cpp']

# Compile flags for STM32H743 with ARM GCC
COMPILE_FLAGS = [
    '-std=c11',
    '-O0',
    '-g',
    '-Wall',
    '-Wextra',
    '-Wpedantic',
    '-fno-common',
    '-ffunction-sections',
    '-fdata-sections',
    '-DSTM32H743xx',
    '-DUSE_HAL_DRIVER',
    '--target=arm-none-eabi',
    '-mcpu=cortex-m7',
    '-mthumb',
    '-mfpu=fpv5-d16',
    '-mfloat-abi=hard',
    '-D__CC_ARM',
    '-fms-extensions',
]

# Include directories
INCLUDE_DIRS = [
    r'D:\Keil_v5\ARM\ARMCC\include',
    os.path.join(PROJECT_ROOT, 'APP'),
    os.path.join(PROJECT_ROOT, 'APP', 'DRIVER', 'inc'),
    os.path.join(PROJECT_ROOT, 'APP', 'TASK', 'inc'),
    os.path.join(PROJECT_ROOT, 'APP', 'TOOL', 'inc'),
    os.path.join(PROJECT_ROOT, 'APP', 'UPDATE', 'inc'),
    os.path.join(PROJECT_ROOT, 'APP', 'USER', 'inc'),
    os.path.join(PROJECT_ROOT, 'Drivers'),
    os.path.join(PROJECT_ROOT, 'Drivers', 'CMSIS', 'Include'),
    os.path.join(PROJECT_ROOT, 'Drivers', 'CMSIS', 'Device', 'ST', 'STM32H7xx', 'Include'),
    os.path.join(PROJECT_ROOT, 'Drivers', 'STM32H7xx_HAL_Driver', 'Inc'),
    os.path.join(PROJECT_ROOT, 'Drivers', 'BSP'),
    os.path.join(PROJECT_ROOT, 'Drivers', 'BSP', 'ETHERNET'),
    os.path.join(PROJECT_ROOT, 'Drivers', 'BSP', 'MPU'),
    os.path.join(PROJECT_ROOT, 'Drivers', 'SYSTEM', 'sys'),
    os.path.join(PROJECT_ROOT, 'Hardware', 'inc'),
    os.path.join(PROJECT_ROOT, 'Middlewares', 'FreeRTOS', 'include'),
    os.path.join(PROJECT_ROOT, 'Middlewares', 'FreeRTOS', 'portable', 'RVDS', 'ARM_CM7', 'r0p1'),
    os.path.join(PROJECT_ROOT, 'Middlewares', 'RTT'),
    os.path.join(PROJECT_ROOT, 'Middlewares', 'USMART'),
    os.path.join(PROJECT_ROOT, 'Middlewares', 'cm_backtrace'),
    os.path.join(PROJECT_ROOT, 'Middlewares', 'easybutton'),
    os.path.join(PROJECT_ROOT, 'Middlewares', 'lwip', 'src', 'include'),
    os.path.join(PROJECT_ROOT, 'Middlewares', 'littlefs'),
    os.path.join(PROJECT_ROOT, 'USER'),
    os.path.join(PROJECT_ROOT, 'INCLUDE'),
]


def find_source_files(directory):
    """Find all source files in the given directory"""
    source_files = []
    
    for root, _, files in os.walk(directory):
        # Skip .cache directory
        if '.cache' in root:
            continue
        
        for file in files:
            if any(file.endswith(ext) for ext in SOURCE_EXTENSIONS):
                source_files.append(os.path.join(root, file))
    
    return source_files


def generate_compile_commands():
    """Generate compile_commands.json file"""
    # Find all source files
    source_files = find_source_files(PROJECT_ROOT)
    
    # Generate compile commands
    compile_commands = []
    
    for file_path in source_files:
        # Get directory
        directory = os.path.dirname(file_path)
        
        # Build include arguments
        include_args = []
        for include_dir in INCLUDE_DIRS:
            if os.path.exists(include_dir):
                include_args.append(f'-I{include_dir}')
        
        # Build complete command
        command_parts = ['clang'] + COMPILE_FLAGS + include_args + ['-c', file_path]
        command = ' '.join(command_parts)
        
        # Create entry
        entry = {
            'directory': directory,
            'command': command,
            'file': file_path
        }
        compile_commands.append(entry)
    
    # Write to compile_commands.json
    output_file = os.path.join(PROJECT_ROOT, 'compile_commands.json')
    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(compile_commands, f, indent=2, ensure_ascii=False)
    
    print(f"Generated compile_commands.json with {len(compile_commands)} entries")
    print(f"Output file: {output_file}")


if __name__ == '__main__':
    generate_compile_commands()
