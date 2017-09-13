/**
 *  @file   velec_memf.h
 *  @author
 *  @date   February, 2016
 *  @brief  Header file for the VELOC_Mem Fortran interface.
 */

#ifndef _VELOC_MemF_H
#define _VELOC_MemF_H

int VELOC_Mem_init_fort_wrapper(char* configFile, int* globalComm);
int VELOC_Mem_type_wrapper(VELOCT_type** type, int size);
int VELOC_Mem_protect_wrapper(int id, void* ptr, long count, VELOCT_type* type);

#endif
