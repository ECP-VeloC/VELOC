/**
 *  @file   velec_memf.h
 *  @author
 *  @date   February, 2016
 *  @brief  Header file for the VELOC_Mem Fortran interface.
 */

#ifndef _VELOC_MemF_H
#define _VELOC_MemF_H

int VELOC_Mem_Init_fort_wrapper(char* configFile, int* globalComm);
int VELOC_Mem_InitType_wrapper(VELOCT_type** type, int size);
int VELOC_Mem_Protect_wrapper(int id, void* ptr, long count, VELOCT_type* type);

#endif
