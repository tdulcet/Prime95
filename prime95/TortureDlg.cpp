// TortureDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Prime95.h"
#include "TortureDlg.h"


// CTortureDlg dialog

IMPLEMENT_DYNAMIC(CTortureDlg, CDialog)
CTortureDlg::CTortureDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTortureDlg::IDD, pParent)
	, m_torture_type(2)
	, m_minfft(0)
	, m_maxfft(0)
	, m_in_place_fft(FALSE)
	, m_memory(0)
	, m_timefft(0)
{
}

CTortureDlg::~CTortureDlg()
{
}

void CTortureDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_L2_CACHE, m_torture_type);
	DDX_Text(pDX, IDC_MINFFT, m_minfft);
	DDV_MinMaxInt(pDX, m_minfft, 8, 4096);
	DDX_Text(pDX, IDC_MAXFFT, m_maxfft);
	DDV_MinMaxInt(pDX, m_maxfft, 8, 4096);
	DDX_Check(pDX, IDC_IN_PLACE_FFT, m_in_place_fft);
	DDX_Text(pDX, IDC_MEMORY, m_memory);
	DDX_Text(pDX, IDC_TIMEFFT, m_timefft);
	DDX_Control(pDX, IDC_MINFFT_TEXT, c_minfft_text);
	DDX_Control(pDX, IDC_MINFFT, c_minfft);
	DDX_Control(pDX, IDC_MAXFFT_TEXT, c_maxfft_text);
	DDX_Control(pDX, IDC_MAXFFT, c_maxfft);
	DDX_Control(pDX, IDC_IN_PLACE_FFT, c_in_place_fft);
	DDX_Control(pDX, IDC_MEMORY_TEXT, c_memory_text);
	DDX_Control(pDX, IDC_MEMORY, c_memory);
	DDX_Control(pDX, IDC_TIMEFFT_TEXT, c_timefft_text);
	DDX_Control(pDX, IDC_TIMEFFT, c_timefft);
	c_minfft_text.EnableWindow (m_torture_type == 3);
	c_minfft.EnableWindow (m_torture_type == 3);
	c_maxfft_text.EnableWindow (m_torture_type == 3);
	c_maxfft.EnableWindow (m_torture_type == 3);
	c_in_place_fft.EnableWindow (m_torture_type == 3);
	c_memory_text.EnableWindow (m_torture_type == 3 && !m_in_place_fft);
	c_memory.EnableWindow (m_torture_type == 3 && !m_in_place_fft);
	c_timefft_text.EnableWindow (m_torture_type == 3);
	c_timefft.EnableWindow (m_torture_type == 3);
}


BEGIN_MESSAGE_MAP(CTortureDlg, CDialog)
	ON_BN_CLICKED(IDC_L2_CACHE, OnBnClickedL2Cache)
	ON_BN_CLICKED(IDC_IN_PLACE, OnBnClickedInPlace)
	ON_BN_CLICKED(IDC_BLEND, OnBnClickedBlend)
	ON_BN_CLICKED(IDC_CUSTOM, OnBnClickedCustom)
	ON_BN_CLICKED(IDC_IN_PLACE_FFT, OnBnClickedInPlaceFft)
END_MESSAGE_MAP()


// CTortureDlg message handlers

void CTortureDlg::OnBnClickedL2Cache()
{
	UpdateData ();
	m_minfft = 8;
	if (CPU_L2_CACHE_SIZE <= 128) m_maxfft = 8;
	else if (CPU_L2_CACHE_SIZE <= 256) m_maxfft = 16;
	else if (CPU_L2_CACHE_SIZE <= 512) m_maxfft = 32;
	else m_maxfft = 64;
	m_in_place_fft = TRUE;
	m_memory = 0;
	m_timefft = 15;
	UpdateData (0);
}

void CTortureDlg::OnBnClickedInPlace()
{
	UpdateData ();
	m_minfft = 128;
	m_maxfft = 1024;
	m_in_place_fft = TRUE;
	m_memory = 8;
	m_timefft = 15;
	UpdateData (0);
}

void CTortureDlg::OnBnClickedBlend()
{
	int	mem;
	UpdateData ();
	m_minfft = 8;
	m_maxfft = 4096;
	m_in_place_fft = FALSE;
	mem = physical_memory ();
	if (mem > 104) m_memory = mem - 96;
	else m_memory = 8;
	m_timefft = 15;
	UpdateData (0);
}


void CTortureDlg::OnBnClickedCustom()
{
	UpdateData ();
}

void CTortureDlg::OnBnClickedInPlaceFft()
{
	UpdateData ();
}
