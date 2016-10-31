#!/bin/bash

place=/scratch/${USER}/QMCPACK_CI_BUILDS_DO_NOT_REMOVE

if [ -e /scratch/${USER} ]; then

if [ ! -e $place ]; then
mkdir $place
fi

if [ -e $place ]; then


for sys in build_gcc build_intel2016 build_intel2015 build_gcc_complex build_intel2016_complex build_intel2015_complex build_gcc_cuda build_intel2015_cuda build_gcc_mkl build_gcc_mkl_complex 
do

cd $place

if [ -e $sys ]; then
rm -r -f $sys
fi
mkdir $sys
cd $sys

echo --- Checkout for $sys `date`
svn checkout https://svn.qmcpack.org/svn/trunk
#svn checkout https://subversion.assembla.com/svn/qmcdev/trunk

if [ -e trunk/CMakeLists.txt ]; then
cd trunk
mkdir $sys
cd $sys
echo --- Building for $sys `date`

export PATH=/opt/local/bin:/opt/local/sbin:/usr/local/cuda/bin/:/usr/lib64/openmpi/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin
export LD_LIBRARY_PATH=/usr/lib64/openmpi/lib:/usr/local/cuda-7.0/lib64

case $sys in
    "build_gcc")
	module() { eval `/usr/bin/modulecmd sh $*`; }
	module load mpi
	export QMCPACK_TEST_SUBMIT_NAME=GCC-Release
	ctest -DCMAKE_C_COMPILER=mpicc -DCMAKE_CXX_COMPILER=mpicxx -S $PWD/../CMake/ctest_script.cmake,release -E 'long' -VV
	module unload mpi
	;;
    "build_gcc_mkl")
	export OLD_PATH=$PATH
	export OLD_LD_LIBRARY_PATH=$LD_LIBRARY_PATH
	export OLD_MANPATH=$MANPATH
	export OLD_NLSPATH=$NLSPATH
	export OLD_CPATH=$CPATH
	export OLD_LIBRARY_PATH=$LIBRARY_PATH
	export OLD_MKLROOT=$MKLROOT
	module() { eval `/usr/bin/modulecmd sh $*`; }
	module load mpi
	source /opt/intel2016/mkl/bin/mklvars.sh intel64
	export QMCPACK_TEST_SUBMIT_NAME=GCC-MKL-Release
	ctest -DCMAKE_C_COMPILER=mpicc -DCMAKE_CXX_COMPILER=mpicxx -DBLA_VENDOR=Intel10_64lp_seq -DCMAKE_PREFIX_PATH=$MKLROOT/lib -S $PWD/../CMake/ctest_script.cmake,release -E 'long' -VV
	module unload mpi
	export PATH=$OLD_PATH
	export LD_LIBRARY_PATH=$OLD_LD_LIBRARY_PATH
	export MANPATH=$OLD_MANPATH
	export NLSPATH=$OLD_NLSPATH
	export CPATH=$OLD_CPATH
	export LIBRARY_PATH=$OLD_LIBRARY_PATH
	export MKLROOT=$OLD_MKLROOT
	;;
    "build_intel2016")
	export OLD_PATH=$PATH
	export OLD_LD_LIBRARY_PATH=$LD_LIBRARY_PATH
	export OLD_MANPATH=$MANPATH
	export OLD_NLSPATH=$NLSPATH
	export OLD_CPATH=$CPATH
	export OLD_LIBRARY_PATH=$LIBRARY_PATH
	export OLD_MKLROOT=$MKLROOT
	source /opt/intel2016/bin/compilervars.sh intel64
	source /opt/intel2016/impi/5.1.1.109/bin64/mpivars.sh
	export QMCPACK_TEST_SUBMIT_NAME=Intel2016-Release
	ctest -DCMAKE_C_COMPILER=mpiicc -DCMAKE_CXX_COMPILER=mpiicpc -S $PWD/../CMake/ctest_script.cmake,release -E 'long' -VV
	export PATH=$OLD_PATH
	export LD_LIBRARY_PATH=$OLD_LD_LIBRARY_PATH
	export MANPATH=$OLD_MANPATH
	export NLSPATH=$OLD_NLSPATH
	export CPATH=$OLD_CPATH
	export LIBRARY_PATH=$OLD_LIBRARY_PATH
	export MKLROOT=$OLD_MKLROOT
	;;
    "build_intel2015")
	export OLD_PATH=$PATH
	export OLD_LD_LIBRARY_PATH=$LD_LIBRARY_PATH
	export OLD_MANPATH=$MANPATH
	export OLD_NLSPATH=$NLSPATH
	export OLD_CPATH=$CPATH
	export OLD_LIBRARY_PATH=$LIBRARY_PATH
	export OLD_MKLROOT=$MKLROOT
	source /opt/intel/bin/compilervars.sh intel64
	source /opt/intel/impi_latest/bin64/mpivars.sh
	export QMCPACK_TEST_SUBMIT_NAME=Intel2015-Release
	ctest -DCMAKE_C_COMPILER=mpiicc -DCMAKE_CXX_COMPILER=mpiicpc -S $PWD/../CMake/ctest_script.cmake,release -E 'long' -VV
	export PATH=$OLD_PATH
	export LD_LIBRARY_PATH=$OLD_LD_LIBRARY_PATH
	export MANPATH=$OLD_MANPATH
	export NLSPATH=$OLD_NLSPATH
	export CPATH=$OLD_CPATH
	export LIBRARY_PATH=$OLD_LIBRARY_PATH
	export MKLROOT=$OLD_MKLROOT
	;;
    "build_gcc_complex")
	module() { eval `/usr/bin/modulecmd sh $*`; }
	module load mpi
	export QMCPACK_TEST_SUBMIT_NAME=GCC-Complex-Release
	ctest -DQMC_COMPLEX=1 -DCMAKE_C_COMPILER=mpicc -DCMAKE_CXX_COMPILER=mpicxx -S $PWD/../CMake/ctest_script.cmake,release -E 'long' -VV
	module unload mpi
	;;
    "build_gcc_mkl_complex")
	export OLD_PATH=$PATH
	export OLD_LD_LIBRARY_PATH=$LD_LIBRARY_PATH
	export OLD_MANPATH=$MANPATH
	export OLD_NLSPATH=$NLSPATH
	export OLD_CPATH=$CPATH
	export OLD_LIBRARY_PATH=$LIBRARY_PATH
	export OLD_MKLROOT=$MKLROOT
	module() { eval `/usr/bin/modulecmd sh $*`; }
	module load mpi
	source /opt/intel2016/mkl/bin/mklvars.sh intel64
	export QMCPACK_TEST_SUBMIT_NAME=GCC-MKL-Complex-Release
	ctest -DQMC_COMPLEX=1 -DCMAKE_C_COMPILER=mpicc -DCMAKE_CXX_COMPILER=mpicxx -DBLA_VENDOR=Intel10_64lp_seq -DCMAKE_PREFIX_PATH=$MKLROOT/lib -S $PWD/../CMake/ctest_script.cmake,release -E 'long' -VV
	module unload mpi
	export PATH=$OLD_PATH
	export LD_LIBRARY_PATH=$OLD_LD_LIBRARY_PATH
	export MANPATH=$OLD_MANPATH
	export NLSPATH=$OLD_NLSPATH
	export CPATH=$OLD_CPATH
	export LIBRARY_PATH=$OLD_LIBRARY_PATH
	export MKLROOT=$OLD_MKLROOT
	;;
    "build_intel2016_complex")
	export OLD_PATH=$PATH
	export OLD_LD_LIBRARY_PATH=$LD_LIBRARY_PATH
	export OLD_MANPATH=$MANPATH
	export OLD_NLSPATH=$NLSPATH
	export OLD_CPATH=$CPATH
	export OLD_LIBRARY_PATH=$LIBRARY_PATH
	export OLD_MKLROOT=$MKLROOT
	source /opt/intel2016/bin/compilervars.sh intel64
	source /opt/intel2016/impi/5.1.1.109/bin64/mpivars.sh
	export QMCPACK_TEST_SUBMIT_NAME=Intel2016-Complex-Release
	ctest -DQMC_COMPLEX=1 -DCMAKE_C_COMPILER=mpiicc -DCMAKE_CXX_COMPILER=mpiicpc -S $PWD/../CMake/ctest_script.cmake,release -E 'long' -VV
	export PATH=$OLD_PATH
	export LD_LIBRARY_PATH=$OLD_LD_LIBRARY_PATH
	export MANPATH=$OLD_MANPATH
	export NLSPATH=$OLD_NLSPATH
	export CPATH=$OLD_CPATH
	export LIBRARY_PATH=$OLD_LIBRARY_PATH
	export MKLROOT=$OLD_MKLROOT
	;;
    "build_intel2015_complex")
	export OLD_PATH=$PATH
	export OLD_LD_LIBRARY_PATH=$LD_LIBRARY_PATH
	export OLD_MANPATH=$MANPATH
	export OLD_NLSPATH=$NLSPATH
	export OLD_CPATH=$CPATH
	export OLD_LIBRARY_PATH=$LIBRARY_PATH
	export OLD_MKLROOT=$MKLROOT
	source /opt/intel/bin/compilervars.sh intel64
	source /opt/intel/impi_latest/bin64/mpivars.sh
	export QMCPACK_TEST_SUBMIT_NAME=Intel2015-Complex-Release
	ctest -DQMC_COMPLEX=1 -DCMAKE_C_COMPILER=mpiicc -DCMAKE_CXX_COMPILER=mpiicpc -S $PWD/../CMake/ctest_script.cmake,release -E 'long' -VV
	export PATH=$OLD_PATH
	export LD_LIBRARY_PATH=$OLD_LD_LIBRARY_PATH
	export MANPATH=$OLD_MANPATH
	export NLSPATH=$OLD_NLSPATH
	export CPATH=$OLD_CPATH
	export LIBRARY_PATH=$OLD_LIBRARY_PATH
	export MKLROOT=$OLD_MKLROOT
	;;
    "build_gcc_cuda")
	module() { eval `/usr/bin/modulecmd sh $*`; }
	module load mpi
	export QMCPACK_TEST_SUBMIT_NAME=GCC-CUDA-Release
	ctest -DCMAKE_C_COMPILER=mpicc -DCMAKE_CXX_COMPILER=mpicxx -DQMC_CUDA=1 -S $PWD/../CMake/ctest_script.cmake,release -E 'long' -VV
	module unload mpi
	;;
    "build_intel2015_cuda")
	export OLD_PATH=$PATH
	export OLD_LD_LIBRARY_PATH=$LD_LIBRARY_PATH
	export OLD_MANPATH=$MANPATH
	export OLD_NLSPATH=$NLSPATH
	export OLD_CPATH=$CPATH
	export OLD_LIBRARY_PATH=$LIBRARY_PATH
	export OLD_MKLROOT=$MKLROOT
	source /opt/intel/bin/compilervars.sh intel64
	source /opt/intel/impi_latest/bin64/mpivars.sh
	export QMCPACK_TEST_SUBMIT_NAME=Intel2015-CUDA-Release
	ctest -DCMAKE_C_COMPILER=mpiicc -DCMAKE_CXX_COMPILER=mpiicpc -DQMC_CUDA=1 -S $PWD/../CMake/ctest_script.cmake,release -E 'long' -VV
	export PATH=$OLD_PATH
	export LD_LIBRARY_PATH=$OLD_LD_LIBRARY_PATH
	export MANPATH=$OLD_MANPATH
	export NLSPATH=$OLD_NLSPATH
	export CPATH=$OLD_CPATH
	export LIBRARY_PATH=$OLD_LIBRARY_PATH
	export MKLROOT=$OLD_MKLROOT
	;;
    *)
	echo "ERROR: Unknown build type $sys"
	;;
esac

else
    echo  "ERROR: No CMakeLists. Bad svn checkout."
    exit 1
fi

done

else
echo "ERROR: No directory $place"
exit 1
fi

fi

