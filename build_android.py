#!/usr/bin/python3
from pathlib import Path
import os
import multiprocessing
import shutil

# Temporary build directory
DIR_BUILD = 'build_android'

# Android NDK home directory 
NDK_HOME = os.environ['ANDROID_NDK_HOME']

# CMake toolchain file
TOOLCHAIN_FILE = f'{NDK_HOME}/build/cmake/android.toolchain.cmake'

# This directory
DIR_THIS = Path(__file__).parent.resolve()

# Path to app
DIR_SOURCE = (DIR_THIS / '../src').resolve()

def make_build() -> Path:
    if Path(DIR_BUILD).exists():
        shutil.rmtree(DIR_BUILD)
    os.mkdir(DIR_BUILD)
    os.chdir(DIR_BUILD)  
    
    cmd = f'cmake -DCMAKE_TOOLCHAIN_FILE={TOOLCHAIN_FILE} '
    cmd += f'-DANDROID_NDK=$NDK_HOME '
    cmd += f'-DANDROID_PLATFORM=24 '
    cmd += f'-DCMAKE_BUILD=Release '
    cmd += f'-DANDROID_ABI="arm64-v8a" '
    cmd += '../src'
    retcode = os.system(cmd)
    if retcode != 0:
        raise RuntimeError('Problem when configuring the project')
    
    cmd = f'cmake --build . -j {multiprocessing.cpu_count()}'
    retcode = os.system(cmd)
    if retcode != 0:
        raise RuntimeError('Problem when building the project')
    
if __name__ == '__main__':
    make_build()
