!> @brief  Heat distribution code to test VELOC in Fortran

program fheatdis

  use VELOC
  use MPI

  real(8), parameter :: PREC       = 0.00001
  integer, parameter :: ITER_TIMES = 200
  integer, parameter :: ITER_OUT   = 50
  integer, parameter :: WORKTAG    = 5
  integer, parameter :: REDUCE     = 1
  integer, parameter :: MEM_MB     = 100
  integer, parameter :: CKPT_FREQ  = ITER_TIMES / 10

  integer, target :: comm, nbProcs, i, row, col, err
  integer :: rank
  integer, pointer  :: ptriter
  integer :: restart_iter, ckpt_success, restart_success
  real(8) :: wtime, memSize, localerror, globalerror
  real(8), pointer :: g(:,:), h(:,:)

  globalerror = 1

  call MPI_Init(err)
  call MPI_Comm_size(MPI_COMM_WORLD, nbProcs, err)
  call MPI_Comm_rank(MPI_COMM_WORLD, rank, err)
  comm = MPI_COMM_WORLD
  call VELOC_Init(comm, 'fheatdis.cfg', err) !see with argc argv
  row = nint(sqrt((MEM_MB * 1024.0 * 512.0 * nbProcs)/8))

  col = (row / nbProcs)+3
  allocate( g(row, col) )
  allocate( h(row, col) )

  call initData(rank, g)

  memSize = row * col * 2 * 8 / (1024 * 1024)
  if ( rank == 0 ) then
    print '("Local data size is ",I5," x ",I5," = ",F5.0," MB (",I5,").")', &
            row, col, memSize, MEM_MB
    print '("Target precision : ",F9.5)', PREC
    print '("Maximum number of iterations : ",I5)', ITER_TIMES
  endif

  ptriter => i
  call VELOC_Mem_protect(0, ptriter, err)
  call VELOC_Mem_protect(1, h, err)
  call VELOC_Mem_protect(2, g, err)
  
  ckpt_success = 1
  restart_success = 1

  wtime = MPI_Wtime()

  call VELOC_Restart_test("fheatdis", 0, restart_iter)
  print '("test restart", I5)', restart_iter 
  if (restart_iter > 0) then
      print '("Previous checkpoint found at iteration ",I5," initiating restart...")', restart_iter
      call VELOC_Restart_begin("fheatdis", restart_iter, err)
      call VELOC_Recover_mem(err)
      call VELOC_Restart_end(restart_success, err) 
  else 
      i = 1
  endif
  
  do while (i .le. ITER_TIMES)
    call doWork(nbProcs, rank, g, h, localerror)

    if ((mod(i, ITER_OUT) == 0) .and. (rank == 0)) then
      print '("Step : ",I5,", error = ",F9.5)', i, globalerror
    endif
    if (mod(i, REDUCE) == 0) then
      call MPI_Allreduce(localerror, globalerror, 1, MPI_REAL8, MPI_MAX, MPI_comm_world, err)
    endif
    if (globalerror < PREC) exit

    i = i + 1

    if (mod(i, CKPT_FREQ) == 0) then
        call VELOC_Checkpoint_wait(err)
        call VELOC_Checkpoint_begin("fheatdis", i, err)
        call VELOC_Checkpoint_mem(err) 
        
        call VELOC_Checkpoint_end(ckpt_success, err)

        if (err /= VELOC_SCES ) then
          print '("Error during checkpoint: ", I5)', err
          exit 
        endif
    endif

  enddo

  if ( rank == 0 ) then
    print '("Execution finished in ",F9.0," seconds.")', MPI_Wtime() - wtime
  endif

  deallocate(h)
  deallocate(g)

  call VELOC_Finalize(0, err)
  call MPI_Finalize(err)

contains

subroutine initData(rank, h)
  real(8), pointer :: h(:,:)
  integer, intent(in) :: rank
  integer :: i

  h(:,:) = 0

  if ( rank == 0 ) then
    do i = size(h, 1)/10, 9*size(h, 1)/10
      h(i,1) = 100
    enddo
  endif

end subroutine

subroutine doWork(numprocs, rank, g, h, localerror)

  integer, intent(in) :: numprocs
  integer, intent(in) :: rank
  real(8), pointer :: g(:,:)
  real(8), pointer :: h(:,:)
  real(8), intent(out) :: localerror

  integer :: i, j, err, req1(2), req2(2)
  integer :: status1(MPI_STATUS_SIZE,2), status2(MPI_STATUS_SIZE,2)

  localerror = 0

  h(:,:) = g(:,:)

  if ( rank > 0 ) then
    call MPI_Isend(g(1,2), size(g, 1), MPI_REAL8, rank-1, WORKTAG, MPI_comm_world, req1(1), err)
    call MPI_Irecv(h(1,1), size(h, 1), MPI_REAL8, rank-1, WORKTAG, MPI_comm_world, req1(2), err)
  endif
  if ( rank < numprocs-1 ) then
    call MPI_Isend(g(1,ubound(g, 2)-1), size(g, 1), MPI_REAL8, rank+1, WORKTAG, MPI_comm_world, req2(1), err)
    call MPI_Irecv(h(1,ubound(h, 2)), size(h, 1), MPI_REAL8, rank+1, WORKTAG, MPI_comm_world, req2(2), err)
  endif
  if ( rank > 0 ) then
    call MPI_Waitall(2, req1, status1, err)
  endif
  if ( rank < numprocs-1 ) then
    call MPI_Waitall(2, req2, status2, err)
  endif

  do j = lbound(h, 2) + 1, ubound(h, 2) - 1
    do i = lbound(h, 1), ubound(h, 1)-1
      g(i,j) = 0.25 * ( h(i-1,j)+h(i+1,j)+h(i,j-1)+h(i,j+1) )
      if ( localerror < abs( g(i,j) - h(i,j) ) ) then
        localerror = abs( g(i,j) - h(i,j) )
      endif
    enddo
  enddo

  if ( rank == (numprocs-1) ) then
    g(ubound(g,1),:) = g(ubound(g,1)-1,:)
  endif

end subroutine

end program
