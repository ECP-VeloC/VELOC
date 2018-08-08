/**
 *  @file   VELOCf.h
 *  @author
 *  @date   February, 2018
 *  @brief  Header file for the VELOC Fortran interface.
 */

#ifndef _VELOCF_H
#define _VELOCF_H

int VELOC_Init_fort_wrapper(int* comm, char* configFile);
int VELOC_Checkpoint_begin_wrapper(char *name, int version);
int VELOC_Checkpoint_wrapper(char *name, int version);
int VELOC_Mem_protect_wrapper(int id, void* ptr, long count, long base_size);
int VELOC_Restart_test_wrapper(char *name, int needed_version);  
int VELOC_Restart_begin_wrapper(char *name, int version);
int VELOC_Restart_wrapper(char *name, int version);
int VELOC_Route_file_wrapper(char *ckpt_file_name);

#endif
