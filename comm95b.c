/*
 * Common routines and variables used by Prime95, Saver95, and NTPrime
 *
 * Comm95a contains information used only during setup
 * Comm95b contains information used only during execution
 * Comm95c contains information used during setup and execution
 */ 

/* Common global variables */

#define MAX_THREAD_HANDLES	128
HANDLE	THREAD_HANDLES[MAX_THREAD_HANDLES] = {0};
int	NUM_THREAD_HANDLES = 0;

/* Common routines */

/* Is this Windows 95/98 or Windows NT? */

int isWindows95 ()
{
	OSVERSIONINFO info;

	info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (GetVersionEx (&info)) {
		return (info.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);
	}
	return (FALSE);
}

/* Is this Windows 2000 or a later Windows NT version? */

int isWindows2000 ()
{
	OSVERSIONINFO info;

	info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (GetVersionEx (&info)) {
		return (info.dwPlatformId == VER_PLATFORM_WIN32_NT &&
			info.dwMajorVersion >= 5);
	}
	return (FALSE);
}

/* Is this Windows Vista or a later Windows version? */

int isWindowsVista ()
{
	OSVERSIONINFO info;

	info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (GetVersionEx (&info)) {
		return (info.dwPlatformId == VER_PLATFORM_WIN32_NT &&
			info.dwMajorVersion >= 6);
	}
	return (FALSE);
}


/* Clear the array of active thread handles */

void clearThreadHandleArray (void)
{
	NUM_THREAD_HANDLES = 0;
}

/* Set the thread priority correctly.  Most screen savers run at priority 4. */
/* Most application's run at priority 9 when in foreground, 7 when in */
/* background.  In selecting the proper thread priority I've assumed the */
/* program usually runs in the background. */ 

/* This routine is also responsible for setting the thread's CPU affinity. */
/* If there are N cpus with hyperthreading, then physical cpu 0 is logical */
/* cpu 0 and N, physical cpu 1 is logical cpu 1 and N+1, etc. */

void setThreadPriorityAndAffinity (
	int	priority,		/* Priority, 1=low, 9=high */
	int	mask)			/* Affinity mask */
{
	HANDLE	h;

/* Get and remember the thread handle */

	h = GetCurrentThread ();
	if (NUM_THREAD_HANDLES < MAX_THREAD_HANDLES)
		THREAD_HANDLES[NUM_THREAD_HANDLES++] = h;

/* Set the thread priority */

	SetPriorityClass (GetCurrentProcess (), NORMAL_PRIORITY_CLASS);
	SetThreadPriority (h,
			   priority == 1 ?
				THREAD_PRIORITY_IDLE : priority - 7);

/* Disable thread priority boost */

	SetThreadPriorityBoost (h, TRUE);

/* Windows 95 does not support affinity settings */

	if (isWindows95 ()) return;

/* Set the affinity */

	SetThreadAffinityMask (h, mask);
}

/* Register a thread termination.  We remove the thread handle from the */
/* list of active worker threads. */

void registerThreadTermination (void)
{
	int	i;
	HANDLE	h;

/* Get the thread handle and remove it from the thread handle array */

	h = GetCurrentThread ();
	for (i = 0; i < NUM_THREAD_HANDLES; i++) {
		if (THREAD_HANDLES[i] != h) break;
		THREAD_HANDLES[i] = THREAD_HANDLES[--NUM_THREAD_HANDLES];
		break;
	}
}

/* When stopping or exiting we raise the priority of all worker threads */
/* so that they can terminate in a timely fashion even if there are other */
/* CPU bound tasks running. */

void raiseAllWorkerThreadPriority (void)
{
	int	i;

	SetPriorityClass (GetCurrentProcess (), NORMAL_PRIORITY_CLASS);

/* Loop through the thread handle array raising priority and */
/* setting affinity */

	for (i = 0; i < NUM_THREAD_HANDLES; i++) {
		SetThreadPriority (THREAD_HANDLES[i],
				   THREAD_PRIORITY_ABOVE_NORMAL);
		if (! isWindows95 ())
			SetThreadAffinityMask (THREAD_HANDLES[i], -1);
	}
}


/* Enumerate processes so that we can pause if a user-specified process */
/* is running.  This code is adapted from Microsoft's knowledge base article */
/* Q175030. */

#ifndef X86_64

/* This function uses the ToolHelp32 and PSAPI.DLL functions via */
/* explicit linking, rather than the more common implicit linking.  This */
/* technique is used so that the program will be binary compatible across */
/* both Windows NT and Windows 95.  (For example, implicit linking of a */
/* ToolHelp32 function would cause an EXE to fail to load and run under */
/* Windows NT.) */

#include <tlhelp32.h>
#include <vdmdbg.h>

typedef struct {
	BOOL           match;
} EnumInfoStruct ;

BOOL WINAPI Enum16( DWORD dwThreadId, WORD hMod16, WORD hTask16,
      PSZ pszModName, PSZ pszFileName, LPARAM lpUserDefined ) ;

int checkPauseListCallback (void)
{
	int		retval = FALSE;
	OSVERSIONINFO	osver;
	HINSTANCE	hInstLib = NULL;
	HINSTANCE	hInstLib2 = NULL;
	BOOL		bFlag;
	LPDWORD		lpdwPIDs = NULL;
	DWORD		dwSize, dwSize2, dwIndex;
	char		szFileName[ MAX_PATH ];

// ToolHelp Function Pointers.

	HANDLE (WINAPI *lpfCreateToolhelp32Snapshot)(DWORD,DWORD);
	BOOL (WINAPI *lpfProcess32First)(HANDLE,LPPROCESSENTRY32);
	BOOL (WINAPI *lpfProcess32Next)(HANDLE,LPPROCESSENTRY32);

// PSAPI Function Pointers.

	BOOL (WINAPI *lpfEnumProcesses)(DWORD *, DWORD, DWORD *);
	BOOL (WINAPI *lpfEnumProcessModules)(HANDLE, HMODULE*, DWORD, LPDWORD);
	DWORD (WINAPI *lpfGetModuleFileNameEx)(HANDLE, HMODULE, LPTSTR, DWORD);

// VDMDBG Function Pointers.

	INT (WINAPI *lpfVDMEnumTaskWOWEx)(DWORD, TASKENUMPROCEX, LPARAM);

// Check to see if were running under Windows95 or Windows NT.

	osver.dwOSVersionInfoSize = sizeof( osver );
	if ( !GetVersionEx( &osver ) ) return FALSE;

// If Windows NT:

	if ( osver.dwPlatformId == VER_PLATFORM_WIN32_NT ) {

// Load library and get the procedures explicitly. We do
// this so that we don't have to worry about modules using
// this code failing to load under Windows 95, because
// it can't resolve references to the PSAPI.DLL.

		hInstLib = LoadLibraryA( "PSAPI.DLL" );
		if ( hInstLib == NULL ) goto ntdone;
		hInstLib2 = LoadLibraryA( "VDMDBG.DLL" );
		if ( hInstLib2 == NULL ) goto ntdone;

// Get procedure addresses.

		lpfEnumProcesses = (BOOL(WINAPI *)(DWORD *,DWORD,DWORD*))
			GetProcAddress( hInstLib, "EnumProcesses" );
		lpfEnumProcessModules = (BOOL(WINAPI *)(HANDLE, HMODULE *,
			DWORD, LPDWORD)) GetProcAddress( hInstLib,
			"EnumProcessModules" );
		lpfGetModuleFileNameEx =(DWORD (WINAPI *)(HANDLE, HMODULE,
			LPTSTR, DWORD )) GetProcAddress( hInstLib,
			"GetModuleFileNameExA" );
		lpfVDMEnumTaskWOWEx =(INT(WINAPI *)( DWORD, TASKENUMPROCEX,
			LPARAM))GetProcAddress( hInstLib2,
			"VDMEnumTaskWOWEx" );
		if ( lpfEnumProcesses == NULL ||
		     lpfEnumProcessModules == NULL ||
		     lpfGetModuleFileNameEx == NULL ||
		     lpfVDMEnumTaskWOWEx == NULL)
			goto ntdone;

// Call the PSAPI function EnumProcesses to get all of the
// ProcID's currently in the system.
// NOTE: In the documentation, the third parameter of
// EnumProcesses is named cbNeeded, which implies that you
// can call the function once to find out how much space to
// allocate for a buffer and again to fill the buffer.
// This is not the case. The cbNeeded parameter returns
// the number of PIDs returned, so if your buffer size is
// zero cbNeeded returns zero.
// NOTE: The "HeapAlloc" loop here ensures that we
// actually allocate a buffer large enough for all the
// PIDs in the system.

		dwSize2 = 256 * sizeof (DWORD);
		do {
			if (lpdwPIDs) {
				HeapFree (GetProcessHeap(), 0, lpdwPIDs);
				dwSize2 *= 2;
			}
			lpdwPIDs = (LPDWORD)
				HeapAlloc (GetProcessHeap(), 0, dwSize2);
			if (lpdwPIDs == NULL) goto ntdone;
			if (!lpfEnumProcesses (lpdwPIDs, dwSize2, &dwSize))
				goto ntdone;
		} while (dwSize == dwSize2);

// How many ProcID's did we get?

		dwSize /= sizeof (DWORD);

// Loop through each ProcID.

		for (dwIndex = 0; dwIndex < dwSize; dwIndex++) {
			HMODULE		hMod;
			HANDLE		hProcess;

// Open the process (if we can... security does not
// permit every process in the system).

			hProcess = OpenProcess(
				PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
				FALSE, lpdwPIDs[ dwIndex ] );
			if (!hProcess) continue;

// Here we call EnumProcessModules to get only the
// first module in the process this is important,
// because this will be the .EXE module for which we
// will retrieve the full path name in a second.

			if (!lpfEnumProcessModules (hProcess, &hMod,
					sizeof (hMod), &dwSize2)) {
				CloseHandle (hProcess);
				continue;
			}

// Get Full pathname:

			if (!lpfGetModuleFileNameEx (hProcess,
					hMod,
					szFileName,
					sizeof (szFileName))) {
				CloseHandle (hProcess);
				continue;
			}

			CloseHandle (hProcess);

// Regardless of OpenProcess success or failure, we
// still call the enum func with the ProcID.

			if (isInPauseList (szFileName)) {
				retval = TRUE;
				goto ntdone;
			}

// Did we just bump into an NTVDM?

			if (_stricmp (szFileName+(strlen(szFileName)-9),
				"NTVDM.EXE")==0) {
				EnumInfoStruct	sInfo;

// Fill in some info for the 16-bit enum proc.

				sInfo.match = FALSE;

// Enum the 16-bit stuff.

				lpfVDMEnumTaskWOWEx (lpdwPIDs[dwIndex],
					(TASKENUMPROCEX) Enum16,
					(LPARAM) &sInfo);

// Did our main enum func say quit?

				if (sInfo.match) {
					retval = TRUE;
					goto ntdone;
				}
			}
		}
ntdone:		if (lpdwPIDs) HeapFree (GetProcessHeap(), 0, lpdwPIDs);
		if (hInstLib) FreeLibrary (hInstLib);
		if (hInstLib2) FreeLibrary (hInstLib2);
		return (retval);
	}

// If Windows 95:

	else if (osver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
		HANDLE		hSnapShot = NULL;
		PROCESSENTRY32	procentry;

		hInstLib = LoadLibraryA ("Kernel32.DLL");
		if (hInstLib == NULL) goto done95;

// Get procedure addresses.
// We are linking to these functions of Kernel32
// explicitly, because otherwise a module using
// this code would fail to load under Windows NT,
// which does not have the Toolhelp32
// functions in the Kernel 32.

		lpfCreateToolhelp32Snapshot=
			(HANDLE(WINAPI *)(DWORD,DWORD))
				GetProcAddress( hInstLib,
					"CreateToolhelp32Snapshot" );
		lpfProcess32First=
			(BOOL(WINAPI *)(HANDLE,LPPROCESSENTRY32))
				GetProcAddress( hInstLib, "Process32First" );
		lpfProcess32Next=
			(BOOL(WINAPI *)(HANDLE,LPPROCESSENTRY32))
				GetProcAddress( hInstLib, "Process32Next" );
		if( lpfProcess32Next == NULL ||
		    lpfProcess32First == NULL ||
		    lpfCreateToolhelp32Snapshot == NULL ) goto done95;

// Get a handle to a Toolhelp snapshot of the systems processes.

		hSnapShot = lpfCreateToolhelp32Snapshot (
			TH32CS_SNAPPROCESS, 0);
		if (hSnapShot == INVALID_HANDLE_VALUE) {
			hSnapShot = NULL;
			goto done95;
		}

// Get the first process' information.

		procentry.dwSize = sizeof (PROCESSENTRY32);
		bFlag = lpfProcess32First (hSnapShot, &procentry);

// While there are processes, keep looping.

		while (bFlag) {

// Call the enum func with the filename and ProcID.

			if (isInPauseList (procentry.szExeFile)) {
				retval = TRUE;
				goto done95;
			}
			procentry.dwSize = sizeof(PROCESSENTRY32);
			bFlag = lpfProcess32Next (hSnapShot, &procentry);
		}

// Free the library.

done95:		if (hInstLib) FreeLibrary (hInstLib);
		if (hSnapShot) CloseHandle (hSnapShot);
		return (retval);
	}

// Unknown OS type

	return (FALSE);
}


BOOL WINAPI Enum16 (DWORD dwThreadId, WORD hMod16, WORD hTask16,
      PSZ pszModName, PSZ pszFileName, LPARAM lpUserDefined)
{
	EnumInfoStruct *psInfo = (EnumInfoStruct *) lpUserDefined;
	if (isInPauseList (pszFileName)) {
		psInfo->match = TRUE;
		return (TRUE);
	}
	return (FALSE);
}

#else

/* The 64-bit version does not need to dynamically load DLLs. */

#include <psapi.h>

int checkPauseListCallback (void)
{
	int		retval = FALSE;
	LPDWORD		lpdwPIDs = NULL;
	DWORD		dwSize, dwSize2, dwIndex;
	char		szFileName[ MAX_PATH ];

// Call the PSAPI function EnumProcesses to get all of the
// ProcID's currently in the system.

	dwSize2 = 256 * sizeof (DWORD);
	do {
		if (lpdwPIDs) {
			HeapFree (GetProcessHeap(), 0, lpdwPIDs);
			dwSize2 *= 2;
		}
		lpdwPIDs = (LPDWORD)
			HeapAlloc (GetProcessHeap(), 0, dwSize2);
		if (lpdwPIDs == NULL) goto ntdone;
		if (!EnumProcesses (lpdwPIDs, dwSize2, &dwSize))
			goto ntdone;
	} while (dwSize == dwSize2);

// How many ProcID's did we get?

	dwSize /= sizeof (DWORD);

// Loop through each ProcID.

	for (dwIndex = 0; dwIndex < dwSize; dwIndex++) {
		HMODULE	hMod;
		HANDLE	hProcess;

// Open the process (if we can... security does not
// permit every process in the system).

		hProcess = OpenProcess (
			PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
			FALSE, lpdwPIDs[ dwIndex ] );
		if (!hProcess) continue;

// Here we call EnumProcessModules to get only the
// first module in the process this is important,
// because this will be the .EXE module for which we
// will retrieve the full path name in a second.

		if (!EnumProcessModules (hProcess, &hMod,
					 sizeof (hMod), &dwSize2)) {
			CloseHandle (hProcess);
			continue;
		}

// Get Full pathname:

		if (!GetModuleFileNameEx (hProcess, hMod, szFileName,
					  sizeof (szFileName))) {
			CloseHandle (hProcess);
			continue;
		}

		CloseHandle (hProcess);

// Does this process force us to pause?

		if (isInPauseList (szFileName)) {
			retval = TRUE;
			goto ntdone;
		}

	}
ntdone:	if (lpdwPIDs) HeapFree (GetProcessHeap(), 0, lpdwPIDs);
	return (retval);
}

#endif
