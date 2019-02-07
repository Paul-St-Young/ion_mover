#!/bin/bash

# define quantum espresso location
# !!!! pw must be compiled with -fPIC
export QE_HOME=/home/yyang173/soft/espresso-5.1.0

# define interface location
export PWINTERFACE_HOME=./src/Interfaces/PWSCF

cwd=`pwd`

# build interface first
cd $PWINTERFACE_HOME
make
cd $cwd

# build qmcpack
mkdir build
cd build
cmake -DCMAKE_C_COMPILER=mpiicc -DCMAKE_CXX_COMPILER=mpiicpc ..
make -j 24
