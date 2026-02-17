#!/usr/bin/env python3

# This script is just to check if the library is buildable

from pathlib import Path
import os
import multiprocessing
import shutil

# Temporary build directory
DIR_BUILD = 'build_windows'

# This directory
DIR_THIS = Path(__file__).parent.resolve()

# Path to app
DIR_SOURCE = (DIR_THIS / '../src').resolve()

CMAKE_GENERATOR = '-G "Visual Studio 17 2022" -A x64'

# Not used yet
CMAKE_BUILD_TYPE = 'Debug'

def make_build() -> Path:
    if Path(DIR_BUILD).exists():
        shutil.rmtree(DIR_BUILD)
    os.mkdir(DIR_BUILD)
    os.chdir(DIR_BUILD)  
    
    if os.environ['VCPKG_ROOT']:
        vcpkg_root = os.environ['VCPKG_ROOT']
    else:
        vcpkg_root = 'C:\\tools\\vcpkg'

    if not Path(vcpkg_root).exists():
        print(f'Failed to find vcpkg (OpenSSL libraries needed)')
        exit(1)
    
    cmd = f'cmake ../src {CMAKE_GENERATOR} -D CMAKE_TOOLCHAIN_FILE="{vcpkg_root}\\scripts\\buildsystems\\vcpkg.cmake"'
    print(cmd)
    retcode = os.system(cmd)
    if retcode != 0:
        raise RuntimeError('Problem when configuring the project')
    
    cmd = f'cmake --build . -j {multiprocessing.cpu_count()}'
    retcode = os.system(cmd)
    if retcode != 0:
        raise RuntimeError('Problem when building the project')
    
    os.chdir('..')
    return Path(DIR_BUILD) / 'Debug' / 'rtphone.lib'

if __name__ == '__main__':
    p = make_build()
    print (f'Built: {p}')
