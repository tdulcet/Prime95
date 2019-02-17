// TortureDlg.cpp : implementation file
//
// Copyright 1995-2019 Mersenne Research, Inc.  All rights reserved
//

#include "stdafx.h"
#include "Prime95.h"
#include "TortureDlg.h"


// CTortureDlg dialog

IMPLEMENT_DYNAMIC(CTortureDlg, CDialog)
CTortureDlg::CTortureDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTortureDlg::IDD, pParent)
	, m_torture_type(4)
	, m_thread (1)
	, m_minfft(0)
	, m_maxfft(0)
	, m_in_place(FALSE)
	, m_memory(0)
	, m_timefft(0)
	, m_blendmemory(0)
	, m_avx512(0)
	, m_fma3(0)
	, m_avx(0)
	, m_sse2(0)
{
}

CTortureDlg::~CTortureDlg()
{
}

void CTortureDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_THREAD, m_thread);
	DDV_MinMaxUInt(pDX, m_thread, 1, NUM_CPUS * CPU_HYPERTHREADS);
	DDX_Radio(pDX, IDC_L2_CACHE, m_torture_type);
	DDX_Text(pDX, IDC_MINFFT, m_minfft);
	DDV_MinMaxInt(pDX, m_minfft, 4,
		(CPU_FLAGS & CPU_AVX512F) ? MAX_FFTLEN_AVX512 / 1024 :
		(CPU_FLAGS & CPU_FMA3) ? MAX_FFTLEN_FMA3 / 1024 :
		(CPU_FLAGS & CPU_SSE2) ? MAX_FFTLEN_SSE2 / 1024 :
					 MAX_FFTLEN / 1024);
	DDX_Text(pDX, IDC_MAXFFT, m_maxfft);
	DDV_MinMaxInt(pDX, m_maxfft, 4,
		(CPU_FLAGS & CPU_AVX512F) ? MAX_FFTLEN_AVX512 / 1024 :
		(CPU_FLAGS & CPU_FMA3) ? MAX_FFTLEN_FMA3 / 1024 :
		(CPU_FLAGS & CPU_SSE2) ? MAX_FFTLEN_SSE2 / 1024 :
					 MAX_FFTLEN / 1024);
	DDX_Check(pDX, IDC_IN_PLACE_FFT, m_in_place);
	DDX_Text(pDX, IDC_MEMORY, m_memory);
	DDX_Text(pDX, IDC_TIMEFFT, m_timefft);
	DDX_Check(pDX, IDC_AVX512, m_avx512);
	DDX_Check(pDX, IDC_FMA3, m_fma3);
	DDX_Check(pDX, IDC_AVX, m_avx);
#ifndef X86_64
	DDX_Check(pDX, IDC_SSE2, m_sse2);
#endif

	DDX_Control(pDX, IDC_THREAD_TEXT, c_thread_text);
	DDX_Control(pDX, IDC_THREAD, c_thread);
	DDX_Control(pDX, IDC_L3_CACHE, c_L3_cache);
	DDX_Control(pDX, IDC_L4_CACHE, c_L4_cache);
	DDX_Control(pDX, IDC_MINFFT_TEXT, c_minfft_text);
	DDX_Control(pDX, IDC_MINFFT, c_minfft);
	DDX_Control(pDX, IDC_MAXFFT_TEXT, c_maxfft_text);
	DDX_Control(pDX, IDC_MAXFFT, c_maxfft);
	DDX_Control(pDX, IDC_IN_PLACE_FFT, c_in_place_fft);
	DDX_Control(pDX, IDC_MEMORY_TEXT, c_memory_text);
	DDX_Control(pDX, IDC_MEMORY, c_memory);
	DDX_Control(pDX, IDC_TIMEFFT_TEXT, c_timefft_text);
	DDX_Control(pDX, IDC_TIMEFFT, c_timefft);
	DDX_Control(pDX, IDC_AVX512, c_avx512);
	DDX_Control(pDX, IDC_FMA3, c_fma3);
	DDX_Control(pDX, IDC_AVX, c_avx);
#ifndef X86_64
	DDX_Control(pDX, IDC_SSE2, c_sse2);
#endif

	if (m_torture_type != 5) {		// Custom
		// Calculate default FFT sizes
		tortureTestDefaultSizes (m_torture_type, m_thread, &m_minfft, &m_maxfft);
		if (m_minfft < 4) m_minfft = 4;
		if (m_maxfft < m_minfft) m_maxfft = m_minfft;
		if (m_maxfft > 32768) m_maxfft = 32768;
		// Assign other options
		m_in_place = (m_torture_type <= 2);		// TRUE for L2/L3/L4 cache
		m_memory = m_in_place ? 0 : m_blendmemory;
		m_timefft = (m_thread > NUM_CPUS ? 6 : 3);
	}

	c_thread_text.EnableWindow (NUM_CPUS * CPU_HYPERTHREADS > 1);
	c_thread.EnableWindow (NUM_CPUS * CPU_HYPERTHREADS > 1);
	c_L3_cache.EnableWindow (CPU_TOTAL_L3_CACHE_SIZE > 0);
	c_L4_cache.EnableWindow (CPU_TOTAL_L4_CACHE_SIZE > 0);
	c_minfft_text.EnableWindow (m_torture_type == 5);
	c_minfft.EnableWindow (m_torture_type == 5);
	c_maxfft_text.EnableWindow (m_torture_type == 5);
	c_maxfft.EnableWindow (m_torture_type == 5);
	c_in_place_fft.EnableWindow (m_torture_type == 5);
	c_memory_text.EnableWindow (m_torture_type == 5 && !m_in_place);
	c_memory.EnableWindow (m_torture_type == 5 && !m_in_place);
	c_timefft_text.EnableWindow (m_torture_type == 5);
	c_timefft.EnableWindow (m_torture_type == 5);
	c_avx512.EnableWindow (CPU_FLAGS & CPU_AVX512F);
	c_fma3.EnableWindow (CPU_FLAGS & CPU_FMA3);
	c_avx.EnableWindow (CPU_FLAGS & CPU_AVX);
#ifndef X86_64
	c_sse2.EnableWindow (CPU_FLAGS & CPU_SSE2);
#endif
}


BEGIN_MESSAGE_MAP(CTortureDlg, CDialog)
	ON_EN_KILLFOCUS(IDC_THREAD, OnEnKillfocusThread)
	ON_BN_CLICKED(IDC_L2_CACHE, OnBnClickedL2Cache)
	ON_BN_CLICKED(IDC_L3_CACHE, OnBnClickedL3Cache)
	ON_BN_CLICKED(IDC_L4_CACHE, OnBnClickedL4Cache)
	ON_BN_CLICKED(IDC_LARGE_FFT, OnBnClickedLargeFFT)
	ON_BN_CLICKED(IDC_BLEND, OnBnClickedBlend)
	ON_BN_CLICKED(IDC_CUSTOM, OnBnClickedCustom)
	ON_BN_CLICKED(IDC_IN_PLACE_FFT, OnBnClickedInPlaceFFT)
END_MESSAGE_MAP()


// CTortureDlg message handlers

void CTortureDlg::OnEnKillfocusThread()
{
	UpdateData ();
	UpdateData (0);
}

void CTortureDlg::OnBnClickedL2Cache()
{
	UpdateData ();
	UpdateData (0);
}

void CTortureDlg::OnBnClickedL3Cache()
{
	UpdateData ();
	UpdateData (0);
}

void CTortureDlg::OnBnClickedL4Cache()
{
	UpdateData ();
	UpdateData (0);
}

void CTortureDlg::OnBnClickedLargeFFT()
{
	UpdateData ();
	UpdateData (0);
}

void CTortureDlg::OnBnClickedBlend()
{
	UpdateData ();
	UpdateData (0);
}


void CTortureDlg::OnBnClickedCustom()
{
	UpdateData ();
}

void CTortureDlg::OnBnClickedInPlaceFFT()
{
	UpdateData ();
}

