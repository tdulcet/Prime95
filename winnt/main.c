
#include <windows.h> 
#include "service.h"
#include "main.h"
#include "prime95.h"
#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Globals
int THREAD_STOP = FALSE;
int DEBUGGING = FALSE;
int C_OPTION = FALSE;

//
//  FUNCTION: ServiceStart
//
//  PURPOSE: Actual code of the service
//           that does the work.
//
//  PARAMETERS:
//    dwArgc   - number of command line arguments
//    lpszArgv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    The default behavior is to open a
//    named pipe, \\.\pipe\simple, and read
//    from it.  It the modifies the data and
//    writes it back to the pipe.  The service
//    stops when hServerStopEvent is signalled
//
VOID ServiceStart (DWORD dwArgc, LPTSTR *lpszArgv)
{

    ///////////////////////////////////////////////////
    //
    // Service initialization
    //

    // report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr(
        SERVICE_START_PENDING, // service state
        NO_ERROR,              // exit code
        3000))                 // wait hint
        goto cleanup;

    // report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr(
        SERVICE_RUNNING,       // service state
        NO_ERROR,              // exit code
        0))                    // wait hint
        goto cleanup;

    //
    // End of initialization
    //
    ////////////////////////////////////////////////////////

	if (C_OPTION) {
		MANUAL_COMM = 3;
		CHECK_WORK_QUEUE = 1;
		communicateWithServer ();
	}
    
    ////////////////////////////////////////////////////////
    //
    // Service is now running, perform work until shutdown
    //

	else
		primeContinue ();


// free up resources

cleanup:
	if (HLIB) FreeLibrary (HLIB);

}

//
//  FUNCTION: ServiceStop
//
//  PURPOSE: Stops the service
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    If a ServiceStop procedure is going to
//    take longer than 3 seconds to execute,
//    it should spawn a thread to execute the
//    stop code, and return.  Otherwise, the
//    ServiceControlManager will believe that
//    the service has stopped responding.
//    
VOID ServiceStop()
{
	SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_NORMAL);
	THREAD_STOP = TRUE;
}

/* GetIniSettings -- Get initial settings from INI files
 *
 * Return:  None
 */
void GetIniSettings()
{
	nameIniFiles (-1);
	readIniFiles ();

	IniGetString (INI_FILE, "ServiceName", SZSERVICENAME,
		      sizeof (SZSERVICENAME), "ppp");
	if (strcmp (SZSERVICENAME, "ppp") != 0) {
		IniWriteString (INI_FILE, "ServiceName", NULL);
		IniWriteString (LOCALINI_FILE, "ServiceName", SZSERVICENAME);
	} else {
		IniGetString (LOCALINI_FILE, "ServiceName", SZSERVICENAME,
			      sizeof (SZSERVICENAME), "NTPrimeService");
	}

	IniGetString (INI_FILE, "DisplayName", SZSERVICEDISPLAYNAME,
		      sizeof (SZSERVICEDISPLAYNAME), "ppp");
	if (strcmp (SZSERVICEDISPLAYNAME, "ppp") != 0) {
		IniWriteString (INI_FILE, "DisplayName", NULL);
		IniWriteString (LOCALINI_FILE, "DisplayName",
				SZSERVICEDISPLAYNAME);
	} else {
		IniGetString (LOCALINI_FILE, "DisplayName",
			      SZSERVICEDISPLAYNAME,
			      sizeof (SZSERVICEDISPLAYNAME), "Prime Service");
	}
}

