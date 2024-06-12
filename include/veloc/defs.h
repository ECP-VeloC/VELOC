#ifndef __DEFS_H
#define __DEFS_H

/*---------------------------------------------------------------------------
                                  Defines
---------------------------------------------------------------------------*/

/** Defines */

#ifndef VELOC_SUCCESS
#define VELOC_SUCCESS (0)
#endif

#ifndef VELOC_FAILURE
#define VELOC_FAILURE (-1)
#endif

#define VELOC_MAX_NAME (1024)

#define VELOC_RECOVER_ALL (0)
#define VELOC_RECOVER_SOME (1)
#define VELOC_RECOVER_REST (2)

#define VELOC_CKPT_ALL (0)
#define VELOC_CKPT_SOME (1)
#define VELOC_CKPT_REST (2)

#define VELOC_OBSERVE_CKPT_END (0)

#endif //__DEFS_H
