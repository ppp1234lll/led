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
    '-D__ARMCC_VERSION=6010050',
    '-D__GNUC__',
    '-include', 'arm_acle.h',
]


def find_include_dirs(root_dir):
    """Find all directories containing header files"""
    include_dirs = set()
    
    # Add Keil system headers
    keil_path = r'D:\Keil_v5\ARM\ARMCLANG\include'
    if os.path.exists(keil_path):
        include_dirs.add(keil_path)

    for root, dirs, files in os.walk(root_dir):
        # Skip hidden directories like .git or .cache
        if any(part.startswith('.') for part in root.split(os.sep)):
            continue
            
        for file in files:
            if file.endswith('.h'):
                include_dirs.add(root)
                break  # Found a header, add dir and move to next
                
    return list(include_dirs)

# Generate include directories dynamically
INCLUDE_DIRS = find_include_dirs(PROJECT_ROOT)


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
            # Always add if it exists (it should, as we just found it)
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
    print(f"Found {len(INCLUDE_DIRS)} include directories")
    print(f"Output file: {output_file}")


if __name__ == '__main__':
    generate_compile_commands()
