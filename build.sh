#!/bin/bash

export PWINTERFACE_HOME=/home/yyang173/soft/IonMover/src/Interfaces/PWSCF

mkdir build
cd build

cmake -DCMAKE_C_COMPILER=mpiicc -DCMAKE_CXX_COMPILER=mpiicpc ..
make -j 24
