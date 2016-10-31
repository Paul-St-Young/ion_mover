!
! Copyright (C) 2001-2013 Quantum ESPRESSO group
! This file is distributed under the terms of the
! GNU General Public License. See the file `License'
! in the root directory of the present distribution,
! or http://www.gnu.org/copyleft/gpl.txt .
!
!----------------------------------------------------------------------------
SUBROUTINE pwlib_init(mpi_comm)
  !----------------------------------------------------------------------------
  !
  ! ... Main program calling one instance of Plane Wave Self-Consistent Field code
  !
  USE environment,       ONLY : environment_start
  USE mp_global,         ONLY : mp_startup
  USE read_input,        ONLY : read_input_file
  USE command_line_options, ONLY: input_file_
  USE parallel_include   

  IMPLICIT NONE
  INTEGER :: exit_status
  INTEGER, INTENT(IN) :: mpi_comm
  INTEGER :: my_comm=0
  IF(mpi_comm >= 0) THEN
     my_comm=mpi_comm
  ELSE
     my_comm=MPI_COMM_WORLD
  ENDIF
  !
  !
  CALL mp_startup (my_comm)
  !CALL mp_startup ( )
  CALL environment_start ( 'PWSCF' )
  !
  CALL read_input_file ('PW', 'scf.in' )
  !
  ! ... Perform actual calculation
  !
  !CALL run_pwscf  ( exit_status )
  !
  !CALL stop_run( exit_status )
  !CALL do_stop( exit_status )
  !
  !STOP
  !
END SUBROUTINE pwlib_init