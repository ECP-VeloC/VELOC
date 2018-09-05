module veloc
  use ISO_C_BINDING

  private
  !> Token returned if a VELOC function succeeds.
  integer, parameter :: VELOC_SCES = 0
  !> Token returned if a VELOC function fails.
  integer, parameter :: VELOC_NSCS = -1

  integer, parameter :: VELOC_CHARACTER1 = int(1, C_int)
  integer, parameter :: VELOC_COMPLEX4 = int(4, C_int)
  integer, parameter :: VELOC_COMPLEX8 = int(8, C_int)
  integer, parameter :: VELOC_INTEGER1 = int(1, C_int)
  integer, parameter :: VELOC_INTEGER2 = int(2, C_int)
  integer, parameter :: VELOC_INTEGER4 = int(4, C_int)
  integer, parameter :: VELOC_INTEGER8 = int(8, C_int)
  integer, parameter :: VELOC_LOGICAL1 = int(1, C_int)
  integer, parameter :: VELOC_REAL4 = int(4, C_int)
  integer, parameter :: VELOC_REAL8 = int(8, C_int)

  public :: VELOC_SCES, VELOC_NSCS, &
      VELOC_CHARACTER1, VELOC_COMPLEX4, VELOC_COMPLEX8, VELOC_LOGICAL1, &
      VELOC_INTEGER1, VELOC_INTEGER2, VELOC_INTEGER4, VELOC_INTEGER8, VELOC_REAL4, VELOC_REAL8, &
      VELOC_Init, &
      VELOC_Finalize, & 
      VELOC_Mem_protect,  &
      VELOC_Mem_unprotect, &
      VELOC_Checkpoint_begin, &
      VELOC_Checkpoint_mem, &
      VELOC_Checkpoint_end, &
      VELOC_Checkpoint_wait, &
      VELOC_Checkpoint, &
      VELOC_Restart_test, &
      VELOC_Restart_begin, &
      VELOC_Recover_mem, &
      VELOC_Restart_end, &
      VELOC_Restart, &
      VELOC_Route_file


  interface
    function VELOC_Init_impl(comm, config_file) &
            bind(c, name='VELOC_Init_fort_wrapper')

      use ISO_C_BINDING

      integer(c_int) :: VELOC_Init_impl
      integer(c_int), intent(in) :: comm
      character(c_char), intent(in) :: config_file(*)

    end function VELOC_Init_impl
  end interface

  interface
    function VELOC_Mem_protect_impl(id_F, ptr, count_F, base_size_F) &
            bind(c, name='VELOC_Mem_protect_wrapper')

      use ISO_C_BINDING

      integer(c_int) :: VELOC_Mem_protect_impl
      integer(c_int), value :: id_F
      type(c_ptr), value :: ptr
      integer(c_long), value :: count_F
      integer(c_long), value :: base_size_F

    end function VELOC_Mem_protect_impl
  end interface

  interface
    function VELOC_Mem_unprotect_impl(id_F) &
            bind(c, name='VELOC_Mem_unprotect')

      use ISO_C_BINDING

      integer(c_int) :: VELOC_Mem_unprotect_impl
      integer(c_int), value :: id_F

    end function VELOC_Mem_unprotect_impl
  end interface

  interface
    function VELOC_Checkpoint_begin_impl(name_F, version_F) &
            bind(c, name='VELOC_Checkpoint_begin_wrapper')

      use ISO_C_BINDING

      integer(c_int) :: VELOC_Checkpoint_begin_impl
      integer(c_int), value :: version_F
      character(c_char), intent(in) :: name_F(*)

    end function VELOC_Checkpoint_begin_impl
  end interface

  interface
    function VELOC_Checkpoint_mem_impl() &
            bind(c, name='VELOC_Checkpoint_mem')

      use ISO_C_BINDING

      integer(c_int) :: VELOC_Checkpoint_mem_impl

    end function VELOC_Checkpoint_mem_impl
  end interface
      
  interface
    function VELOC_Checkpoint_end_impl(success_F) &
            bind(c, name='VELOC_Checkpoint_end')

      use ISO_C_BINDING

      integer(c_int) :: VELOC_Checkpoint_end_impl
      integer(c_int), value :: success_F

    end function VELOC_Checkpoint_end_impl
  end interface
  
  interface
    function VELOC_Checkpoint_wait_impl() &
            bind(c, name='VELOC_Checkpoint_wait')

      use ISO_C_BINDING

      integer(c_int) :: VELOC_Checkpoint_wait_impl

    end function VELOC_Checkpoint_wait_impl
  end interface

  interface
    function VELOC_Checkpoint_impl(name_F, version_F) &
            bind(c, name='VELOC_Checkpoint_wrapper')

      use ISO_C_BINDING

      integer(c_int) :: VELOC_Checkpoint_impl
      integer(c_int), value :: version_F
      character(c_char), intent(in) :: name_F(*)

    end function VELOC_Checkpoint_impl
  end interface

  interface
    function VELOC_Restart_test_impl(name_F, needed_version_F) &
            bind(c, name='VELOC_Restart_test_wrapper')

      use ISO_C_BINDING

      integer(c_int) :: VELOC_Restart_test_impl
      character(c_char), intent(in) :: name_F(*)
      integer(c_int), value :: needed_version_F

    end function VELOC_Restart_test_impl
  end interface

  interface
    function VELOC_Restart_begin_impl(name_F, version_F) &
            bind(c, name='VELOC_Restart_begin_wrapper')

      use ISO_C_BINDING

      integer(c_int) :: VELOC_Restart_begin_impl
      integer(c_int), value :: version_F
      character(c_char), intent(in) :: name_F(*)

    end function VELOC_Restart_begin_impl
  end interface

  interface
    function VELOC_Recover_mem_impl() &
            bind(c, name='VELOC_Recover_mem')

      use ISO_C_BINDING

      integer(c_int) :: VELOC_Recover_mem_impl

    end function VELOC_Recover_mem_impl
  end interface

  interface
    function VELOC_Restart_end_impl(success_F) &
            bind(c, name='VELOC_Restart_end')

      use ISO_C_BINDING

      integer(c_int) :: VELOC_Restart_end_impl
      integer(c_int), value :: success_F

    end function VELOC_Restart_end_impl
  end interface

  interface
    function VELOC_Restart_impl(name_F, version_F) &
            bind(c, name='VELOC_Restart_wrapper')

      use ISO_C_BINDING

      integer(c_int) :: VELOC_Restart_impl
      integer(c_int), value :: version_F
      character(c_char), intent(in) :: name_F(*)

    end function VELOC_Restart_impl
  end interface

  interface
    function VELOC_Route_file_impl(ckpt_file_name, routed)&
            bind(c, name='VELOC_Route_file_wrapper')

      use ISO_C_BINDING

      integer(c_int) :: VELOC_Route_file_impl
      character(c_char), intent(in) :: ckpt_file_name(*)
      character(c_char), intent(in) :: routed(*)

    end function VELOC_Route_file_impl
  end interface

  interface
    function VELOC_Finalize_impl(cleanup_F) &
            bind(c, name='VELOC_Finalize')

      use ISO_C_BINDING

      integer(c_int) :: VELOC_Finalize_impl
      integer(c_int) :: cleanup_F

    end function VELOC_Finalize_impl
  end interface


  interface VELOC_Mem_protect
    module procedure VELOC_Mem_protect_Ptr
    module procedure VELOC_Mem_protect_CHARACTER10
    module procedure VELOC_Mem_protect_CHARACTER11
    module procedure VELOC_Mem_protect_CHARACTER12
    module procedure VELOC_Mem_protect_CHARACTER13
    module procedure VELOC_Mem_protect_CHARACTER14
    module procedure VELOC_Mem_protect_CHARACTER15
    module procedure VELOC_Mem_protect_CHARACTER16
    module procedure VELOC_Mem_protect_CHARACTER17
    module procedure VELOC_Mem_protect_COMPLEX40
    module procedure VELOC_Mem_protect_COMPLEX41
    module procedure VELOC_Mem_protect_COMPLEX42
    module procedure VELOC_Mem_protect_COMPLEX43
    module procedure VELOC_Mem_protect_COMPLEX44
    module procedure VELOC_Mem_protect_COMPLEX45
    module procedure VELOC_Mem_protect_COMPLEX46
    module procedure VELOC_Mem_protect_COMPLEX47
    module procedure VELOC_Mem_protect_COMPLEX80
    module procedure VELOC_Mem_protect_COMPLEX81
    module procedure VELOC_Mem_protect_COMPLEX82
    module procedure VELOC_Mem_protect_COMPLEX83
    module procedure VELOC_Mem_protect_COMPLEX84
    module procedure VELOC_Mem_protect_COMPLEX85
    module procedure VELOC_Mem_protect_COMPLEX86
    module procedure VELOC_Mem_protect_COMPLEX87
    module procedure VELOC_Mem_protect_INTEGER10
    module procedure VELOC_Mem_protect_INTEGER11
    module procedure VELOC_Mem_protect_INTEGER12
    module procedure VELOC_Mem_protect_INTEGER13
    module procedure VELOC_Mem_protect_INTEGER14
    module procedure VELOC_Mem_protect_INTEGER15
    module procedure VELOC_Mem_protect_INTEGER16
    module procedure VELOC_Mem_protect_INTEGER17
    module procedure VELOC_Mem_protect_INTEGER20
    module procedure VELOC_Mem_protect_INTEGER21
    module procedure VELOC_Mem_protect_INTEGER22
    module procedure VELOC_Mem_protect_INTEGER23
    module procedure VELOC_Mem_protect_INTEGER24
    module procedure VELOC_Mem_protect_INTEGER25
    module procedure VELOC_Mem_protect_INTEGER26
    module procedure VELOC_Mem_protect_INTEGER27
    module procedure VELOC_Mem_protect_INTEGER40
    module procedure VELOC_Mem_protect_INTEGER41
    module procedure VELOC_Mem_protect_INTEGER42
    module procedure VELOC_Mem_protect_INTEGER43
    module procedure VELOC_Mem_protect_INTEGER44
    module procedure VELOC_Mem_protect_INTEGER45
    module procedure VELOC_Mem_protect_INTEGER46
    module procedure VELOC_Mem_protect_INTEGER47
    module procedure VELOC_Mem_protect_INTEGER80
    module procedure VELOC_Mem_protect_INTEGER81
    module procedure VELOC_Mem_protect_INTEGER82
    module procedure VELOC_Mem_protect_INTEGER83
    module procedure VELOC_Mem_protect_INTEGER84
    module procedure VELOC_Mem_protect_INTEGER85
    module procedure VELOC_Mem_protect_INTEGER86
    module procedure VELOC_Mem_protect_INTEGER87
    module procedure VELOC_Mem_protect_LOGICAL10
    module procedure VELOC_Mem_protect_LOGICAL11
    module procedure VELOC_Mem_protect_LOGICAL12
    module procedure VELOC_Mem_protect_LOGICAL13
    module procedure VELOC_Mem_protect_LOGICAL14
    module procedure VELOC_Mem_protect_LOGICAL15
    module procedure VELOC_Mem_protect_LOGICAL16
    module procedure VELOC_Mem_protect_LOGICAL17
    module procedure VELOC_Mem_protect_REAL40
    module procedure VELOC_Mem_protect_REAL41
    module procedure VELOC_Mem_protect_REAL42
    module procedure VELOC_Mem_protect_REAL43
    module procedure VELOC_Mem_protect_REAL44
    module procedure VELOC_Mem_protect_REAL45
    module procedure VELOC_Mem_protect_REAL46
    module procedure VELOC_Mem_protect_REAL47
    module procedure VELOC_Mem_protect_REAL80
    module procedure VELOC_Mem_protect_REAL81
    module procedure VELOC_Mem_protect_REAL82
    module procedure VELOC_Mem_protect_REAL83
    module procedure VELOC_Mem_protect_REAL84
    module procedure VELOC_Mem_protect_REAL85
    module procedure VELOC_Mem_protect_REAL86
    module procedure VELOC_Mem_protect_REAL87
  end interface VELOC_Mem_protect


contains

  subroutine VELOC_Init(comm, config_file, err)
    include 'mpif.h'
    integer, intent(in) :: comm
    character(len=*), intent(in) :: config_file
    integer, intent(out) :: err
    character, target, dimension(1:len_trim(config_file)+1) :: config_file_c
    integer :: ii, ll
    integer(c_int) :: comm_c

    ll = len_trim(config_file)
    do ii = 1, ll
      config_file_c(ii) = config_file(ii:ii)
    enddo
    config_file_c(ll+1) = c_null_char
    comm_c = int(comm, c_int)
    err = int(VELOC_Init_impl(comm_c, config_file_c))
    if (err /= VELOC_SCES ) then
      return
    endif
  end subroutine VELOC_Init


 subroutine VELOC_Checkpoint_begin(name_ckpt, version, err)
    integer, intent(in) :: version
    character(len=*), intent(in) :: name_ckpt
    integer, intent(out) :: err
    character, target, dimension(1:len_trim(name_ckpt)+1) :: name_ckpt_c
    integer :: ii, ll

    ll = len_trim(name_ckpt)
    do ii = 1, ll
      name_ckpt_c(ii) = name_ckpt(ii:ii)
    enddo
    name_ckpt_c(ll+1) = c_null_char
    err = int(VELOC_Checkpoint_begin_impl(name_ckpt_c, int(version, c_int)))
  end subroutine VELOC_Checkpoint_begin


  subroutine VELOC_Checkpoint_mem(err)
    integer, intent(out) :: err
    
    err = int(VELOC_Checkpoint_mem_impl())
  end subroutine VELOC_Checkpoint_mem
      

  subroutine VELOC_Checkpoint_end(success, err)
    integer, intent (inout) :: success
    integer, intent(out) :: err
    
    err = int(VELOC_Checkpoint_end_impl(int(success, c_int)))
  end subroutine VELOC_Checkpoint_end


  subroutine VELOC_Checkpoint_wait(err)
    integer, intent(out) :: err
    
    err = int(VELOC_Checkpoint_wait_impl())
  end subroutine VELOC_Checkpoint_wait

  subroutine VELOC_Checkpoint(name_ckpt, version, err)
    integer, intent(in) :: version
    character(len=*), intent(in) :: name_ckpt
    integer, intent(out) :: err
    character, target, dimension(1:len_trim(name_ckpt)+1) :: name_ckpt_c
    integer :: ii, ll

    ll = len_trim(name_ckpt)
    do ii = 1, ll
      name_ckpt_c(ii) = name_ckpt(ii:ii)
    enddo
    name_ckpt_c(ll+1) = c_null_char
    err = int(VELOC_Checkpoint_impl(name_ckpt_c, int(version, c_int)))
  end subroutine VELOC_Checkpoint


  subroutine VELOC_Mem_protect_Ptr(id_F, ptr, count_F, base_size_F, err)
    integer, intent(in) :: id_F
    type(c_ptr), value :: ptr
    integer, intent(in) :: count_F
    integer, intent(in) :: base_size_F
    integer, intent(out) :: err

    err = int(VELOC_Mem_protect_impl(int(id_F, c_int), ptr, int(count_F, c_long), &
            int(base_size_F, c_long)))
  end subroutine VELOC_Mem_protect_Ptr

  subroutine VELOC_Mem_protect_CHARACTER10(id_F, data, err)
    integer, intent(IN) :: id_F
    character(KIND=1), pointer :: data
    integer, intent(OUT) :: err

    call VELOC_Mem_protect_Ptr(id_F, c_loc(data), 1, VELOC_CHARACTER1, err)
  end subroutine VELOC_Mem_protect_CHARACTER10

  subroutine VELOC_Mem_protect_CHARACTER11(id_F, data, err)
    integer, intent(IN) :: id_F
    character(KIND=1), pointer :: data(:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1))),size(data), VELOC_CHARACTER1, err)
  end subroutine VELOC_Mem_protect_CHARACTER11

  subroutine VELOC_Mem_protect_CHARACTER12(id_F, data, err)
    integer, intent(IN) :: id_F
    character(KIND=1), pointer :: data(:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2))),size(data), VELOC_CHARACTER1, err)
  end subroutine VELOC_Mem_protect_CHARACTER12

  subroutine VELOC_Mem_protect_CHARACTER13(id_F, data, err)
    integer, intent(IN) :: id_F
    character(KIND=1), pointer :: data(:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3))), size(data), VELOC_CHARACTER1, err)
  end subroutine VELOC_Mem_protect_CHARACTER13

  subroutine VELOC_Mem_protect_CHARACTER14(id_F, data, err)
    integer, intent(IN) :: id_F
    character(KIND=1), pointer :: data(:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4))), &
            size(data), VELOC_CHARACTER1, err)
  end subroutine VELOC_Mem_protect_CHARACTER14

  subroutine VELOC_Mem_protect_CHARACTER15(id_F, data, err)
    integer, intent(IN) :: id_F
    character(KIND=1), pointer :: data(:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5))), size(data), VELOC_CHARACTER1, err)
  end subroutine VELOC_Mem_protect_CHARACTER15

  subroutine VELOC_Mem_protect_CHARACTER16(id_F, data, err)
    integer, intent(IN) :: id_F
    character(KIND=1), pointer :: data(:,:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5),lbound(data, 6))), size(data), VELOC_CHARACTER1, err)
  end subroutine VELOC_Mem_protect_CHARACTER16

  subroutine VELOC_Mem_protect_CHARACTER17(id_F, data, err)
    integer, intent(IN) :: id_F
    character(KIND=1), pointer :: data(:,:,:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5),lbound(data, 6),lbound(data, 7))), size(data), VELOC_CHARACTER1, err)
  end subroutine VELOC_Mem_protect_CHARACTER17

  subroutine VELOC_Mem_protect_COMPLEX40(id_F, data, err)
    integer, intent(IN) :: id_F
    complex(KIND=4), pointer :: data
    integer, intent(OUT) :: err

    call VELOC_Mem_protect_Ptr(id_F, c_loc(data), 1, VELOC_COMPLEX4, err)
  end subroutine VELOC_MEM_protect_COMPLEX40

  subroutine VELOC_Mem_protect_COMPLEX41(id_F, data, err)
    integer, intent(IN) :: id_F
    complex(KIND=4), pointer :: data(:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1))),size(data), VELOC_COMPLEX4, err)
  end subroutine VELOC_Mem_protect_COMPLEX41

  subroutine VELOC_Mem_protect_COMPLEX42(id_F, data, err)
    integer, intent(IN) :: id_F
    complex(KIND=4), pointer :: data(:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2))),size(data), VELOC_COMPLEX4, err)
  end subroutine VELOC_Mem_protect_COMPLEX42

  subroutine VELOC_Mem_protect_COMPLEX43(id_F, data, err)
    integer, intent(IN) :: id_F
    complex(KIND=4), pointer :: data(:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3))), &
            size(data), VELOC_COMPLEX4, err)
  end subroutine VELOC_Mem_protect_COMPLEX43

  subroutine VELOC_Mem_protect_COMPLEX44(id_F, data, err)
    integer, intent(IN) :: id_F
    complex(KIND=4), pointer :: data(:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4))), & 
            size(data), VELOC_COMPLEX4, err)
  end subroutine VELOC_Mem_protect_COMPLEX44

  subroutine VELOC_Mem_protect_COMPLEX45(id_F, data, err)
    integer, intent(IN) :: id_F
    complex(KIND=4), pointer :: data(:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5))), size(data), VELOC_COMPLEX4, err)
  end subroutine VELOC_Mem_protect_COMPLEX45

  subroutine VELOC_Mem_protect_COMPLEX46(id_F, data, err)
    integer, intent(IN) :: id_F
    complex(KIND=4), pointer :: data(:,:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5),lbound(data, 6))), size(data), VELOC_COMPLEX4, err)
  end subroutine VELOC_Mem_protect_COMPLEX46

  subroutine VELOC_Mem_protect_COMPLEX47(id_F, data, err)
    integer, intent(IN) :: id_F
    complex(KIND=4), pointer :: data(:,:,:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5),lbound(data, 6),lbound(data, 7))), size(data), VELOC_COMPLEX4, err)
  end subroutine VELOC_Mem_protect_COMPLEX47

  subroutine VELOC_Mem_protect_COMPLEX80(id_F, data, err)
    integer, intent(IN) :: id_F
    complex(KIND=8), pointer :: data
    integer, intent(OUT) :: err

    call VELOC_Mem_protect_Ptr(id_F, c_loc(data), 1, VELOC_COMPLEX8, err)
  end subroutine VELOC_MEM_protect_COMPLEX80

  subroutine VELOC_Mem_protect_COMPLEX81(id_F, data, err)
    integer, intent(IN) :: id_F
    complex(KIND=8), pointer :: data(:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1))),size(data), VELOC_COMPLEX8, err)
  end subroutine VELOC_Mem_protect_COMPLEX81

  subroutine VELOC_Mem_protect_COMPLEX82(id_F, data, err)
    integer, intent(IN) :: id_F
    complex(KIND=8), pointer :: data(:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2))),size(data), VELOC_COMPLEX8, err)
  end subroutine VELOC_Mem_protect_COMPLEX82

  subroutine VELOC_Mem_protect_COMPLEX83(id_F, data, err)
    integer, intent(IN) :: id_F
    complex(KIND=8), pointer :: data(:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3))), &
            size(data), VELOC_COMPLEX8, err)
  end subroutine VELOC_Mem_protect_COMPLEX83

  subroutine VELOC_Mem_protect_COMPLEX84(id_F, data, err)
    integer, intent(IN) :: id_F
    complex(KIND=8), pointer :: data(:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4))), &
            size(data), VELOC_COMPLEX8, err)
  end subroutine VELOC_Mem_protect_COMPLEX84

  subroutine VELOC_Mem_protect_COMPLEX85(id_F, data, err)
    integer, intent(IN) :: id_F
    complex(KIND=8), pointer :: data(:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5))), size(data), VELOC_COMPLEX8, err)
  end subroutine VELOC_Mem_protect_COMPLEX85

  subroutine VELOC_Mem_protect_COMPLEX86(id_F, data, err)
    integer, intent(IN) :: id_F
    complex(KIND=8), pointer :: data(:,:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5),lbound(data, 6))), size(data), VELOC_COMPLEX8, err)
  end subroutine VELOC_Mem_protect_COMPLEX86

  subroutine VELOC_Mem_protect_COMPLEX87(id_F, data, err)
    integer, intent(IN) :: id_F
    complex(KIND=8), pointer :: data(:,:,:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5),lbound(data, 6),lbound(data, 7))), size(data), VELOC_COMPLEX8, err)
  end subroutine VELOC_Mem_protect_COMPLEX87

  subroutine VELOC_Mem_protect_INTEGER10(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=1), pointer :: data
    integer, intent(OUT) :: err

    call VELOC_Mem_protect_Ptr(id_F, c_loc(data), 1, VELOC_INTEGER1, err)
  end subroutine VELOC_MEM_protect_INTEGER10

  subroutine VELOC_Mem_protect_INTEGER11(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=1), pointer :: data(:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1))),size(data), VELOC_INTEGER1, err)
  end subroutine VELOC_Mem_protect_INTEGER11

  subroutine VELOC_Mem_protect_INTEGER12(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=1), pointer :: data(:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2))),size(data), VELOC_INTEGER1, err)
  end subroutine VELOC_Mem_protect_INTEGER12

  subroutine VELOC_Mem_protect_INTEGER13(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=1), pointer :: data(:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3))), &
            size(data), VELOC_INTEGER1, err)
  end subroutine VELOC_Mem_protect_INTEGER13

  subroutine VELOC_Mem_protect_INTEGER14(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=1), pointer :: data(:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4))), &
            size(data), VELOC_INTEGER1, err)
  end subroutine VELOC_Mem_protect_INTEGER14

  subroutine VELOC_Mem_protect_INTEGER15(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=1), pointer :: data(:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5))), size(data), VELOC_INTEGER1, err)
  end subroutine VELOC_Mem_protect_INTEGER15

  subroutine VELOC_Mem_protect_INTEGER16(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=1), pointer :: data(:,:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5),lbound(data, 6))), size(data), VELOC_INTEGER1, err)
  end subroutine VELOC_Mem_protect_INTEGER16

  subroutine VELOC_Mem_protect_INTEGER17(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=1), pointer :: data(:,:,:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5),lbound(data, 6),lbound(data, 7))), size(data), VELOC_INTEGER1, err)
  end subroutine VELOC_Mem_protect_INTEGER17

  subroutine VELOC_Mem_protect_INTEGER20(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=2), pointer :: data
    integer, intent(OUT) :: err

    call VELOC_Mem_protect_Ptr(id_F, c_loc(data), 1, VELOC_INTEGER2, err)
  end subroutine VELOC_MEM_protect_INTEGER20

  subroutine VELOC_Mem_protect_INTEGER21(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=2), pointer :: data(:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1))),size(data), VELOC_INTEGER2, err)
  end subroutine VELOC_Mem_protect_INTEGER21

  subroutine VELOC_Mem_protect_INTEGER22(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=2), pointer :: data(:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2))),size(data), VELOC_INTEGER2, err)
  end subroutine VELOC_Mem_protect_INTEGER22

  subroutine VELOC_Mem_protect_INTEGER23(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=2), pointer :: data(:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3))), &
            size(data), VELOC_INTEGER2, err)
  end subroutine VELOC_Mem_protect_INTEGER23

  subroutine VELOC_Mem_protect_INTEGER24(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=2), pointer :: data(:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4))), &
            size(data), VELOC_INTEGER2, err)
  end subroutine VELOC_Mem_protect_INTEGER24

  subroutine VELOC_Mem_protect_INTEGER25(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=2), pointer :: data(:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5))), size(data), VELOC_INTEGER2, err)
  end subroutine VELOC_Mem_protect_INTEGER25

  subroutine VELOC_Mem_protect_INTEGER26(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=2), pointer :: data(:,:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5),lbound(data, 6))), size(data), VELOC_INTEGER2, err)
  end subroutine VELOC_Mem_protect_INTEGER26

  subroutine VELOC_Mem_protect_INTEGER27(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=2), pointer :: data(:,:,:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5),lbound(data, 6),lbound(data, 7))), size(data), VELOC_INTEGER2, err)
  end subroutine VELOC_Mem_protect_INTEGER27

  subroutine VELOC_Mem_protect_INTEGER40(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=4), pointer :: data
    integer, intent(OUT) :: err

    call VELOC_Mem_protect_Ptr(id_F, c_loc(data), 1, VELOC_INTEGER4, err)
  end subroutine VELOC_MEM_protect_INTEGER40

  subroutine VELOC_Mem_protect_INTEGER41(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=4), pointer :: data(:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1))),size(data), VELOC_INTEGER4, err)
  end subroutine VELOC_Mem_protect_INTEGER41

  subroutine VELOC_Mem_protect_INTEGER42(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=4), pointer :: data(:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2))),size(data), VELOC_INTEGER4, err)
  end subroutine VELOC_Mem_protect_INTEGER42

  subroutine VELOC_Mem_protect_INTEGER43(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=4), pointer :: data(:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3))), &
            size(data), VELOC_INTEGER4, err)
  end subroutine VELOC_Mem_protect_INTEGER43

  subroutine VELOC_Mem_protect_INTEGER44(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=4), pointer :: data(:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4))), &
            size(data), VELOC_INTEGER4, err)
  end subroutine VELOC_Mem_protect_INTEGER44

  subroutine VELOC_Mem_protect_INTEGER45(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=4), pointer :: data(:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5))), size(data), VELOC_INTEGER4, err)
  end subroutine VELOC_Mem_protect_INTEGER45

  subroutine VELOC_Mem_protect_INTEGER46(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=4), pointer :: data(:,:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5),lbound(data, 6))), size(data), VELOC_INTEGER4, err)
  end subroutine VELOC_Mem_protect_INTEGER46

  subroutine VELOC_Mem_protect_INTEGER47(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=4), pointer :: data(:,:,:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5),lbound(data, 6),lbound(data, 7))), size(data), VELOC_INTEGER4, err)
  end subroutine VELOC_Mem_protect_INTEGER47

  subroutine VELOC_Mem_protect_INTEGER80(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=8), pointer :: data
    integer, intent(OUT) :: err

    call VELOC_Mem_protect_Ptr(id_F, c_loc(data), 1, VELOC_INTEGER8, err)
  end subroutine VELOC_MEM_protect_INTEGER80

  subroutine VELOC_Mem_protect_INTEGER81(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=8), pointer :: data(:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1))),size(data), VELOC_INTEGER8, err)
  end subroutine VELOC_Mem_protect_INTEGER81

  subroutine VELOC_Mem_protect_INTEGER82(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=8), pointer :: data(:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2))),size(data), VELOC_INTEGER8, err)
  end subroutine VELOC_Mem_protect_INTEGER82

  subroutine VELOC_Mem_protect_INTEGER83(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=8), pointer :: data(:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3))), &
            size(data), VELOC_INTEGER8, err)
  end subroutine VELOC_Mem_protect_INTEGER83

  subroutine VELOC_Mem_protect_INTEGER84(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=8), pointer :: data(:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4))), &
            size(data), VELOC_INTEGER8, err)
  end subroutine VELOC_Mem_protect_INTEGER84

  subroutine VELOC_Mem_protect_INTEGER85(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=8), pointer :: data(:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5))), size(data), VELOC_INTEGER8, err)
  end subroutine VELOC_Mem_protect_INTEGER85

  subroutine VELOC_Mem_protect_INTEGER86(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=8), pointer :: data(:,:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5),lbound(data, 6))), size(data), VELOC_INTEGER8, err)
  end subroutine VELOC_Mem_protect_INTEGER86

  subroutine VELOC_Mem_protect_INTEGER87(id_F, data, err)
    integer, intent(IN) :: id_F
    integer(KIND=8), pointer :: data(:,:,:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5),lbound(data, 6),lbound(data, 7))), size(data), VELOC_INTEGER8, err)
  end subroutine VELOC_Mem_protect_INTEGER87

  subroutine VELOC_Mem_protect_LOGICAL10(id_F, data, err)
    integer, intent(IN) :: id_F
    logical(KIND=1), pointer :: data
    integer, intent(OUT) :: err

    call VELOC_Mem_protect_Ptr(id_F, c_loc(data), 1, VELOC_LOGICAL1, err)
  end subroutine VELOC_MEM_protect_LOGICAL10

  subroutine VELOC_Mem_protect_LOGICAL11(id_F, data, err)
    integer, intent(IN) :: id_F
    logical(KIND=1), pointer :: data(:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1))),size(data), VELOC_LOGICAL1, err)
  end subroutine VELOC_Mem_protect_LOGICAL11

  subroutine VELOC_Mem_protect_LOGICAL12(id_F, data, err)
    integer, intent(IN) :: id_F
    logical(KIND=1), pointer :: data(:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2))),size(data), VELOC_LOGICAL1, err)
  end subroutine VELOC_Mem_protect_LOGICAL12

  subroutine VELOC_Mem_protect_LOGICAL13(id_F, data, err)
    integer, intent(IN) :: id_F
    logical(KIND=1), pointer :: data(:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3))), size(data), &
            VELOC_LOGICAL1, err)
  end subroutine VELOC_Mem_protect_LOGICAL13

  subroutine VELOC_Mem_protect_LOGICAL14(id_F, data, err)
    integer, intent(IN) :: id_F
    logical(KIND=1), pointer :: data(:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4))), &
            size(data), VELOC_LOGICAL1, err)
  end subroutine VELOC_Mem_protect_LOGICAL14

  subroutine VELOC_Mem_protect_LOGICAL15(id_F, data, err)
    integer, intent(IN) :: id_F
    logical(KIND=1), pointer :: data(:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5))), size(data), VELOC_LOGICAL1, err)
  end subroutine VELOC_Mem_protect_LOGICAL15

  subroutine VELOC_Mem_protect_LOGICAL16(id_F, data, err)
    integer, intent(IN) :: id_F
    logical(KIND=1), pointer :: data(:,:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5),lbound(data, 6))), size(data), VELOC_LOGICAL1, err)
  end subroutine VELOC_Mem_protect_LOGICAL16

  subroutine VELOC_Mem_protect_LOGICAL17(id_F, data, err)
    integer, intent(IN) :: id_F
    logical(KIND=1), pointer :: data(:,:,:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5),lbound(data, 6),lbound(data, 7))), size(data), VELOC_LOGICAL1, err)
  end subroutine VELOC_Mem_protect_LOGICAL17

  subroutine VELOC_Mem_protect_REAL40(id_F, data, err)
    integer, intent(IN) :: id_F
    REAL(KIND=4), pointer :: data
    integer, intent(OUT) :: err

    call VELOC_Mem_protect_Ptr(id_F, c_loc(data), 1, VELOC_REAL4, err)
  end subroutine VELOC_MEM_protect_REAL40

  subroutine VELOC_Mem_protect_REAL41(id_F, data, err)
    integer, intent(IN) :: id_F
    REAL(KIND=4), pointer :: data(:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1))),size(data), VELOC_REAL4, err)
  end subroutine VELOC_Mem_protect_REAL41

  subroutine VELOC_Mem_protect_REAL42(id_F, data, err)
    integer, intent(IN) :: id_F
    REAL(KIND=4), pointer :: data(:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2))),size(data), VELOC_REAL4, err)
  end subroutine VELOC_Mem_protect_REAL42

  subroutine VELOC_Mem_protect_REAL43(id_F, data, err)
    integer, intent(IN) :: id_F
    REAL(KIND=4), pointer :: data(:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3))), size(data), &
            VELOC_REAL4, err)
  end subroutine VELOC_Mem_protect_REAL43

  subroutine VELOC_Mem_protect_REAL44(id_F, data, err)
    integer, intent(IN) :: id_F
    REAL(KIND=4), pointer :: data(:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4))), &
            size(data), VELOC_REAL4, err)
  end subroutine VELOC_Mem_protect_REAL44

  subroutine VELOC_Mem_protect_REAL45(id_F, data, err)
    integer, intent(IN) :: id_F
    REAL(KIND=4), pointer :: data(:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5))), size(data), VELOC_REAL4, err)
  end subroutine VELOC_Mem_protect_REAL45

  subroutine VELOC_Mem_protect_REAL46(id_F, data, err)
    integer, intent(IN) :: id_F
    REAL(KIND=4), pointer :: data(:,:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5),lbound(data, 6))), size(data), VELOC_REAL4, err)
  end subroutine VELOC_Mem_protect_REAL46

  subroutine VELOC_Mem_protect_REAL47(id_F, data, err)
    integer, intent(IN) :: id_F
    REAL(KIND=4), pointer :: data(:,:,:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5),lbound(data, 6),lbound(data, 7))), size(data), VELOC_REAL4, err)
  end subroutine VELOC_Mem_protect_REAL47

  subroutine VELOC_Mem_protect_REAL80(id_F, data, err)
    integer, intent(IN) :: id_F
    REAL(KIND=8), pointer :: data
    integer, intent(OUT) :: err

    call VELOC_Mem_protect_Ptr(id_F, c_loc(data), 1, VELOC_REAL8, err)
  end subroutine VELOC_MEM_protect_REAL80

  subroutine VELOC_Mem_protect_REAL81(id_F, data, err)
    integer, intent(IN) :: id_F
    REAL(KIND=8), pointer :: data(:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1))),size(data), VELOC_REAL8, err)
  end subroutine VELOC_Mem_protect_REAL81

  subroutine VELOC_Mem_protect_REAL82(id_F, data, err)
    integer, intent(IN) :: id_F
    REAL(KIND=8), pointer :: data(:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2))),size(data), VELOC_REAL8, err)
  end subroutine VELOC_Mem_protect_REAL82

  subroutine VELOC_Mem_protect_REAL83(id_F, data, err)
    integer, intent(IN) :: id_F
    REAL(KIND=8), pointer :: data(:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3))), size(data), &
            VELOC_REAL8, err)
  end subroutine VELOC_Mem_protect_REAL83

  subroutine VELOC_Mem_protect_REAL84(id_F, data, err)
    integer, intent(IN) :: id_F
    REAL(KIND=8), pointer :: data(:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4))), &
            size(data), VELOC_REAL8, err)
  end subroutine VELOC_Mem_protect_REAL84

  subroutine VELOC_Mem_protect_REAL85(id_F, data, err)
    integer, intent(IN) :: id_F
    REAL(KIND=8), pointer :: data(:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5))), size(data), VELOC_REAL8, err)
  end subroutine VELOC_Mem_protect_REAL85

  subroutine VELOC_Mem_protect_REAL86(id_F, data, err)
    integer, intent(IN) :: id_F
    REAL(KIND=8), pointer :: data(:,:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5),lbound(data, 6))), size(data), VELOC_REAL8, err)
  end subroutine VELOC_Mem_protect_REAL86

  subroutine VELOC_Mem_protect_REAL87(id_F, data, err)
    integer, intent(IN) :: id_F
    REAL(KIND=8), pointer :: data(:,:,:,:,:,:,:)
    integer, intent(OUT) :: err

    ! workaround, we take the address of the first array element and hope for
    ! the best since not much better can be done
    call VELOC_Mem_protect_Ptr(id_F, &
            c_loc(data(lbound(data, 1),lbound(data, 2),lbound(data, 3),lbound(data, 4), &
            lbound(data, 5),lbound(data, 6),lbound(data, 7))), size(data), VELOC_REAL8, err)
  end subroutine VELOC_Mem_protect_REAL87


  subroutine VELOC_Mem_unprotect(id_F, err)
    integer, intent(in) :: id_F
    integer, intent(out) :: err

    err = int(VELOC_Mem_unprotect_impl(int(id_F, c_int)))
  end subroutine VELOC_Mem_unprotect


  subroutine VELOC_Restart_test(name_F, needed_version_F, version)
    character(len=*), intent(in) :: name_F
    integer, intent(in) :: needed_version_F
    integer, intent(out) :: version
    character, target, dimension(1:len_trim(name_F)+1) :: name_c
    integer :: ii, ll

    ll = len_trim(name_F)
    do ii = 1, ll
      name_c(ii) = name_F(ii:ii)
    enddo
    name_c(ll+1) = c_null_char

    version = int(VELOC_Restart_test_impl(name_c, int(needed_version_F, c_int)))
      
  end subroutine VELOC_Restart_test


  subroutine VELOC_Restart_begin(name_F, version_F, err)
    integer, intent(in) :: version_F
    character(len=*), intent(in) :: name_F
    integer, intent(out) :: err
    character, target, dimension(1:len_trim(name_F)+1) :: name_c
    integer :: ii, ll

    ll = len_trim(name_F)
    do ii = 1, ll
      name_c(ii) = name_F(ii:ii)
    enddo
    name_c(ll+1) = c_null_char

    err = int(VELOC_Restart_begin_impl(name_c, int(version_F, c_int)))

  end subroutine VELOC_Restart_begin


  subroutine VELOC_Recover_mem(err)
    integer, intent(out) :: err

    err = int(VELOC_Recover_mem_impl())
  end subroutine VELOC_Recover_mem


  subroutine VELOC_Restart_end(success, err)
    integer, intent(inout):: success
    integer, intent(out) :: err

    err = int(VELOC_Restart_end_impl(int(success, c_int)))    

  end subroutine VELOC_Restart_end


  subroutine VELOC_Restart(name_F, version_F, err)
    integer, intent(in) :: version_F
    character(len=*), intent(in) :: name_F
    integer, intent(out) :: err
    character, target, dimension(1:len_trim(name_F)+1) :: name_c
    integer :: ii, ll

    ll = len_trim(name_F)
    do ii = 1, ll
      name_c(ii) = name_F(ii:ii)
    enddo
    name_c(ll+1) = c_null_char

    err = int(VELOC_Restart_impl(name_c, int(version_F, c_int)))

  end subroutine VELOC_Restart


  subroutine VELOC_Route_file(ckpt_file_name, routed, err)
    character(len=*), intent(in) :: ckpt_file_name
    character(len=*), intent(in) :: routed
    character, target, dimension(1:len_trim(ckpt_file_name)+1) :: ckpt_file_name_c
    character, target, dimension(1:len_trim(routed)+1) :: routed_c
    integer, intent(out) :: err
    integer :: ii, ll

    ll = len_trim(ckpt_file_name)
    do ii = 1, ll
      ckpt_file_name_c(ii) = ckpt_file_name(ii:ii)
    enddo
    ckpt_file_name_c(ll+1) = c_null_char

    ll = len_trim(routed)
    do ii = 1, ll
      routed_c(ii) = routed(ii:ii)
    enddo
    routed_c(ll+1) = c_null_char

    err = int(VELOC_Route_file_impl(ckpt_file_name_c, routed_c))

  end subroutine VELOC_Route_file

  subroutine VELOC_Finalize(cleanup_F, err)
    integer, intent(in) :: cleanup_F
    integer, intent(out) :: err

    err = int(VELOC_Finalize_impl(int(cleanup_F, c_int)))
  end subroutine VELOC_Finalize

end module veloc
