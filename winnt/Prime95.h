// Prime95.h : main header file for the PRIME95 application
//

#include <time.h>
//#define SERVER_TESTING
#define NO_GUI		1
#include "common.h"
#include "cpuid.h"
#include "commonb.h"
#include "commonc.h"
#include "comm95b.h"
#include "comm95c.h"
#include "primenet.h"

// Global variables

extern int THREAD_STOP;			// TRUE if thread should stop
extern int DEBUGGING;			// TRUE if -debug switch used
extern int C_OPTION;			// TRUE if -c switch used
