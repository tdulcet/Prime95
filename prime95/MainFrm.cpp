// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "Prime95.h"

#include "MainFrm.h"
#include <winreg.h>
#include <pbt.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_SIZE()
	ON_WM_ENDSESSION()
	ON_WM_ACTIVATEAPP()
	//}}AFX_MSG_MAP
	// Global help commands
	ON_COMMAND(ID_HELP_FINDER, CFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_HELP, CFrameWnd::OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, CFrameWnd::OnHelpFinder)
	ON_MESSAGE(WM_POWERBROADCAST, OnPower)
	ON_MESSAGE(MYWM_TRAYMESSAGE, OnTrayMessage)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
	
}

CMainFrame::~CMainFrame()
{
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CFrameWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers
LONG CMainFrame::OnPower(UINT uID, LONG uMouseMsg)
{
	if (uID == PBT_APMPOWERSTATUSCHANGE) {
		CHECK_BATTERY = 1;
		if (STOPPED_ON_BATTERY) {
			STOPPED_ON_BATTERY = 999;
			PostMessage (WM_COMMAND, IDM_CONTINUE, 0);
		}
	}
	return 0;
}

LONG CMainFrame::OnTrayMessage(UINT uID, LONG uMouseMsg)
{
	if (uID == 352 && uMouseMsg == WM_LBUTTONDBLCLK)
		if (IsWindowVisible())
			ShowWindow(FALSE);	// hide it
		else {
			ShowWindow(TRUE);	// show it
			SetForegroundWindow();
		}

	return 0;
}


BOOL CMainFrame::DestroyWindow() 
{
	if (TRAY_ICON) ((CPrime95App *)AfxGetApp())->TrayMessage (NIM_DELETE, NULL, 0);
	return CFrameWnd::DestroyWindow();
}

void CMainFrame::OnSize(UINT nType, int cx, int cy) 
{
	if (nType == SIZE_MINIMIZED) {
		if (TRAY_ICON) ShowWindow (FALSE);// hide it
		if (HIDE_ICON) ShowWindow (FALSE);
	}
	CFrameWnd::OnSize(nType, cx, cy);
}

LRESULT CMainFrame::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	WM_ENDSESSION_LPARAM = lParam;
	return CFrameWnd::WindowProc(message, wParam, lParam);
}

void CMainFrame::OnEndSession(BOOL bEnding) 
{
	if (WINDOWS95_SERVICE && WM_ENDSESSION_LPARAM) {
		if (TRAY_ICON) WINDOWS95_TRAY_ADD = 1;
	} else
		CFrameWnd::OnEndSession(bEnding);
}

#define RSP_SIMPLE_SERVICE	1
#define RSP_UNREGISTER_SERVICE	0
void Service95 ()
{
	HMODULE	hlib;
	DWORD (__stdcall *proc)(DWORD, DWORD);
	DWORD	rc;

/* Call RegisterServiceProcess in the Kernel */

	if (isWindows95 ()) {
		hlib = LoadLibrary ("KERNEL32.DLL");
		if (!hlib) {
			OutputStr ("Unable to load KERNEL32.DLL\n");
			return;
		}
		proc = (DWORD (__stdcall *)(DWORD, DWORD))
			GetProcAddress (hlib, "RegisterServiceProcess");
		if (proc == NULL)
			OutputStr ("Unable to find RegisterServiceProcess\n");
		else {
			if (WINDOWS95_SERVICE)
				rc = (*proc) (NULL, RSP_SIMPLE_SERVICE);
			else
				rc = (*proc) (NULL, RSP_UNREGISTER_SERVICE);
			if (!rc)
				OutputStr ("RegisterServiceProcess failed\n");
		}
		FreeLibrary (hlib);
	}

/* Make sure the registry has an entry to run the service */
/* For Windows 95/98/Me create a RunServices entry */
/* For Windows NT/2000/XP create a Run entry for the local machine or current user */

	HKEY	hkey;
	DWORD	disposition;

	if (isWindows95 ()) {
		if (RegCreateKeyEx (
				HKEY_LOCAL_MACHINE,
				"Software\\Microsoft\\Windows\\CurrentVersion\\RunServices",
				0,
				NULL,
				REG_OPTION_NON_VOLATILE,
				KEY_ALL_ACCESS,
				NULL,
				&hkey,
				&disposition) != ERROR_SUCCESS) {
			OutputStr ("Can't create registry key.\n");
			return;
		}
	} else {
		if (RegCreateKeyEx (
				HKEY_LOCAL_MACHINE,
				"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
				0,
				NULL,
				REG_OPTION_NON_VOLATILE,
				KEY_ALL_ACCESS,
				NULL,
				&hkey,
				&disposition) != ERROR_SUCCESS &&
		    RegCreateKeyEx (
				HKEY_CURRENT_USER,
				"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
				0,
				NULL,
				REG_OPTION_NON_VOLATILE,
				KEY_ALL_ACCESS,
				NULL,
				&hkey,
				&disposition) != ERROR_SUCCESS) {
			OutputStr ("Can't create registry key.\n");
			return;
		}
	}

/* We create a different entry for each -A command line value */

	char prog[20];
	if (WINDOWS95_A_SWITCH < 0)
		strcpy (prog, "Prime95");
	else
		sprintf (prog, "Prime95-%d", WINDOWS95_A_SWITCH);

/* Now create or delete an entry for prime95 */

	if (WINDOWS95_SERVICE) {
		char	buf[256];
		GetModuleFileName (NULL, buf, sizeof (buf));
		if (WINDOWS95_A_SWITCH >= 0) {
			char	append[20];
			sprintf (append, " -A%d", WINDOWS95_A_SWITCH);
			strcat (buf, append);
		}
		rc = RegSetValueEx (hkey, prog, 0, REG_SZ,
				    (BYTE *) buf, strlen (buf) + 1);
	} else
		rc = RegDeleteValue (hkey, prog);
	if (rc != ERROR_SUCCESS)
		OutputStr ("Can't write registry value.\n");
	RegCloseKey (hkey);

/* Now delete and shortcuts to the program.  We do this so that as users upgrade */
/* from version 20 and try this menu choice they do not end up with both a registry */
/* entry and a StartUp menu shortcut. */

	if (WINDOWS95_SERVICE) {
		char	buf[256];
		DWORD	type;
		DWORD	bufsize = sizeof (buf);

		if (RegCreateKeyEx (
				HKEY_CURRENT_USER,
				"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
				0,
				NULL,
				REG_OPTION_NON_VOLATILE,
				KEY_ALL_ACCESS,
				NULL,
				&hkey,
				&disposition) != ERROR_SUCCESS)
			return;
		if (RegQueryValueEx (hkey, "Startup", NULL, &type,
				(BYTE *) buf, &bufsize) == ERROR_SUCCESS &&
		    type == REG_SZ) {
			strcat (buf, "\\prime95.lnk");
			_unlink (buf);
		}
		RegCloseKey (hkey);
	}
}

void CMainFrame::OnActivateApp(BOOL bActive, HTASK hTask) 
{
	if (bActive && BROADCAST_MESSAGE != NULL) {
		if (HIDE_ICON) BlinkIcon (0);
		AfxGetApp()->m_pMainWnd->PostMessage (WM_COMMAND, USR_BROADCAST, 0);
	}
	CFrameWnd::OnActivateApp(bActive, hTask);
}
