#!/usr/bin/python3
from pathlib import Path
import os
import multiprocessing
import shutil

# Temporary build directory
DIR_BUILD = 'build_linux'

# This directory
DIR_THIS = Path(__file__).parent.resolve()

# Path to app
DIR_SOURCE = (DIR_THIS / '../src').resolve()

def make_build() -> Path:
    if Path(DIR_BUILD).exists():
        shutil.rmtree(DIR_BUILD)
    os.mkdir(DIR_BUILD)
    os.chdir(DIR_BUILD)  
    
    cmd = f'cmake ../src -G Ninja'
    retcode = os.system(cmd)
    if retcode != 0:
        raise RuntimeError('Problem when configuring the project')
    
    cmd = f'cmake --build . -j {multiprocessing.cpu_count()}'
    retcode = os.system(cmd)
    if retcode != 0:
        raise RuntimeError('Problem when building the project')
    
    os.chdir('..')
    return Path(DIR_BUILD) / 'librtphone.a'

if __name__ == '__main__':
    p = make_build()
    print (f'Built: {p}')
