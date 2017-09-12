/**
 *  @file   interface.h
 *  @author
 *  @date   February, 2016
 *  @brief  Header file for the VELOC_Mem library private functions.
 */

#ifndef _VELOC_Mem_INTERFACE_H
#define _VELOC_Mem_INTERFACE_H

#include "veloc_mem.h"

#include "iniparser.h"
#include "dictionary.h"

#include "galois.h"
#include "jerasure.h"

#ifdef ENABLE_SIONLIB // --> If SIONlib is installed
    #include <sion.h>
#endif

#include <stdint.h>
#include "md5.h"

#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include <limits.h>

/*---------------------------------------------------------------------------
                                  Defines
---------------------------------------------------------------------------*/

/** Malloc macro.                                                          */
#define talloc(type, num) (type *)malloc(sizeof(type) * (num))

/*---------------------------------------------------------------------------
                            VELOC_Mem private functions
---------------------------------------------------------------------------*/
void VELOC_Mem_PrintMeta(VELOCT_execution* VELOC_Mem_Exec, VELOCT_topology* VELOC_Mem_Topo);
void VELOC_Mem_Abort();
int VELOC_Mem_FloatBitFlip(float *target, int bit);
int VELOC_Mem_DoubleBitFlip(double *target, int bit);
void VELOC_Mem_Print(char *msg, int priority);

int VELOC_Mem_UpdateIterTime(VELOCT_execution* VELOC_Mem_Exec);
int VELOC_Mem_WriteCkpt(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                  VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt,
                  VELOCT_dataset* VELOC_Mem_Data);
#ifdef ENABLE_SIONLIB // --> If SIONlib is installed
int VELOC_Mem_WriteSionlib(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                VELOCT_topology* VELOC_Mem_Topo,VELOCT_dataset* VELOC_Mem_Data);
#endif
int VELOC_Mem_WriteMPI(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                VELOCT_topology* VELOC_Mem_Topo,VELOCT_dataset* VELOC_Mem_Data);
int VELOC_Mem_WritePar(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                VELOCT_topology* VELOC_Mem_Topo,VELOCT_dataset* VELOC_Mem_Data);
int VELOC_Mem_WritePosix(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                    VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt,
                    VELOCT_dataset* VELOC_Mem_Data);
int VELOC_Mem_GroupClean(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_topology* VELOC_Mem_Topo,
                   VELOCT_checkpoint* VELOC_Mem_Ckpt, int level, int group, int pr);
int VELOC_Mem_PostCkpt(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                 VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt,
                 int group, int fo, int pr);
int VELOC_Mem_Listen(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
               VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt);

int VELOC_Mem_UpdateConf(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                   int restart);
int VELOC_Mem_ReadConf(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                 VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt,
                 VELOCT_injection *VELOC_Mem_Inje);
int VELOC_Mem_TestConfig(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_topology* VELOC_Mem_Topo,
                   VELOCT_checkpoint* VELOC_Mem_Ckpt);
int VELOC_Mem_TestDirectories(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_topology* VELOC_Mem_Topo);
int VELOC_Mem_CreateDirs(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                   VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt);
int VELOC_Mem_LoadConf(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                 VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt,
                 VELOCT_injection *VELOC_Mem_Inje);

int VELOC_Mem_GetChecksums(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_topology* VELOC_Mem_Topo,
                     VELOCT_checkpoint* VELOC_Mem_Ckpt, char* checksum, char* ptnerChecksum,
                     char* rsChecksum, int group, int level);
int VELOC_Mem_WriteRSedChecksum(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                             VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt,
                              int rank, char* checksum);
int VELOC_Mem_GetPtnerSize(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_topology* VELOC_Mem_Topo,
                      VELOCT_checkpoint* VELOC_Mem_Ckpt, unsigned long* pfs, int group, int level);
int VELOC_Mem_LoadTmpMeta(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
              VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt);
int VELOC_Mem_LoadMeta(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
              VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt);
int VELOC_Mem_WriteMetadata(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_topology* VELOC_Mem_Topo,
                      unsigned long *fs, unsigned long mfs, char* fnl, char* checksums);
int VELOC_Mem_CreateMetadata(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                       VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt);

int VELOC_Mem_Local(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
              VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt);
int VELOC_Mem_Ptner(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
              VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt);
int VELOC_Mem_RSenc(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
              VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt);
int VELOC_Mem_Flush(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
              VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt, int level);
int VELOC_Mem_FlushPosix(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                  VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt, int level);
int VELOC_Mem_FlushMPI(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                  VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt, int level);
#ifdef ENABLE_SIONLIB // --> If SIONlib is installed
int VELOC_Mem_FlushSionlib(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                    VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt, int level);
#endif
int VELOC_Mem_Decode(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
               VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt, int *erased);
int VELOC_Mem_RecoverL1(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                  VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt);
int VELOC_Mem_RecoverL2(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                  VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt);
int VELOC_Mem_RecoverL3(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                  VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt);
int VELOC_Mem_RecoverL4(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                  VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt);
int VELOC_Mem_RecoverL4Posix(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                  VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt);
int VELOC_Mem_RecoverL4Mpi(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                  VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt);
#ifdef ENABLE_SIONLIB // --> If SIONlib is installed
int VELOC_Mem_RecoverL4Sionlib(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                  VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt);
#endif
int VELOC_Mem_CheckFile(char *fn, unsigned long fs, char* checksum);
int VELOC_Mem_CheckErasures(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                      VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt,
                      int *erased);
int VELOC_Mem_RecoverFiles(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                     VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt);

int VELOC_Mem_Checksum(char* fileName, char* checksum);
int VELOC_Mem_VerifyChecksum(char* fileName, char* checksumToCmp);
int VELOC_Mem_Try(int result, char* message);
void VELOC_Mem_MallocMeta(VELOCT_execution* VELOC_Mem_Exec, VELOCT_topology* VELOC_Mem_Topo);
void VELOC_Mem_FreeMeta(VELOCT_execution* VELOC_Mem_Exec);
int VELOC_Mem_InitBasicTypes(VELOCT_dataset* VELOC_Mem_Data);
int VELOC_Mem_RmDir(char path[VELOC_Mem_BUFS], int flag);
int VELOC_Mem_Clean(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_topology* VELOC_Mem_Topo,
              VELOCT_checkpoint* VELOC_Mem_Ckpt, int level, int group, int rank);

int VELOC_Mem_SaveTopo(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_topology* VELOC_Mem_Topo, char *nameList);
int VELOC_Mem_ReorderNodes(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_topology* VELOC_Mem_Topo,
                     int *nodeList, char *nameList);
int VELOC_Mem_BuildNodeList(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                      VELOCT_topology* VELOC_Mem_Topo, int *nodeList, char *nameList);
int VELOC_Mem_CreateComms(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                    VELOCT_topology* VELOC_Mem_Topo, int *userProcList,
                    int *distProcList, int* nodeList);
int VELOC_Mem_Topology(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                 VELOCT_topology* VELOC_Mem_Topo);

#endif
