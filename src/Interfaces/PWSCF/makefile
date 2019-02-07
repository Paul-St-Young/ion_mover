FFLAGS= -fPIC -qopenmp -mkl=parallel -O3 -assume byterecl -g -traceback -nomodule -fpp -D__OPENMP -D__INTEL -D__FFTW3 -DEXX -DH5_USE_16_API  -I${QE_HOME}/include -I${QE_HOME}/iotk/src -I${QE_HOME}/Modules -I${QE_HOME}/PW/src -I${MKLROOT}/include

all:
	ifort $(FFLAGS) -c pwinterfacemod.f90 
	ifort $(FFLAGS) -c pwlib_getinfo.f90 
	ifort $(FFLAGS) -c pwlib_init.f90 
	ifort $(FFLAGS) -c pwlib_scf.f90 
	
	#mpic++ -c mpitest.cpp
	
	mpiicpc -qopenmp -shared -o libpwinterface.so pwlib_getinfo.o pwlib_scf.o pwlib_init.o pwinterfacemod.o ${QE_HOME}/PW/src/libpw.a ${QE_HOME}/Modules/libqemod.a ${QE_HOME}/flib/ptools.a ${QE_HOME}/flib/flib.a ${QE_HOME}/clib/clib.a ${QE_HOME}/iotk/src/libiotk.a  ${MKLROOT}/lib/intel64/libmkl_scalapack_ilp64.a -Wl,--start-group ${MKLROOT}/lib/intel64/libmkl_intel_ilp64.a ${MKLROOT}/lib/intel64/libmkl_intel_thread.a ${MKLROOT}/lib/intel64/libmkl_core.a ${MKLROOT}/lib/intel64/libmkl_blacs_intelmpi_ilp64.a -Wl,--end-group -liomp5 -lpthread -lm -ldl -L/usr/local/lib -lhdf5 -lhdf5_hl -lifcore -lifport -fPIC

clean:
	rm *.o *.mod *.so
