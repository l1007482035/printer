//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	precomp.h
//    
//  PURPOSE:	Header files that should be in the precompiled header.
//
#pragma once

// Necessary for compiling under VC.
#if(!defined(WINVER) || (WINVER < 0x0500))
	#undef WINVER
	#define WINVER          0x0500
#endif
#if(!defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0500))
	#undef _WIN32_WINNT
	#define _WIN32_WINNT    0x0500
#endif


// Required header files that shouldn't change often.

#include <STDDEF.H>
#include <STDLIB.H>
#include <OBJBASE.H>
#include <STDARG.H>
#include <STDIO.H>
#include <WINDEF.H>
#include <WINERROR.H>
#include <WINBASE.H>
#include <WINGDI.H>
#include <WINDDI.H>
#include <TCHAR.H>
#include <EXCPT.H>
#include <ASSERT.H>
#include <PRINTOEM.H>
#include <INITGUID.H>
#include <PRCOMOEM.H>

// StrSafe.h needs to be included last
// to disallow unsafe string functions.
#include <STRSAFE.H>

#include <WinSpool.h>

#define PROJECT_REGISTY_KEY	L"SYSTEM\\CurrentControlSet\\Control\\Print\\Printers\\XabVPrinter"
#define PRINTER_REGISTY_KEY	L"SYSTEM\\CurrentControlSet\\Control\\Print\\Printers"

#define PRODUCT_REGISTY_KEY			_T("SOFTWARE\\iSecStar") 
#define PRODUCT_REGISTY_KEY_AMD64	_T("SOFTWARE\\Wow6432Node\\iSecStar") 

#define PostScriptHeadSize	(262144)	//256KB

extern WCHAR g_wszJobInfoPath[MAX_PATH];
extern BOOL g_bIsPDFDoc;
extern DWORD g_dwPostScriptWritten;
extern BOOL g_bReleaseFile;
extern BOOL g_bDebugLog;
extern void log(WCHAR* pFormat, ...);
extern char* UnicodeToUTF8(const wchar_t* pwszSource, int nLen);
extern char* UnicodeToMultiByte(const wchar_t* pwszSource);

//�����ɴ�ӡ�ĵ���ʽ��ʶ��
#define Doc_Format_UnKnow		0		//δ֪�ɴ�ӡ�ĵ�
#define Doc_Format_Word			1		//office word
#define Doc_Format_PowerPoint	2		//office PowerPoint
#define Doc_Format_Excel		3		//office Excel
#define Doc_Format_PDF			4		//Adobe Portable Document Format (PDF)
#define Doc_Format_XPS			5		//Microsoft XML Paper Specification (XPS)
#define Doc_Format_WMF			6		//Windows Metafile Format (WMF)
#define Doc_Format_EMF			7		//Windows Enhanced MetaFile format (EMF)
#define Doc_Format_PS			8		//Adobe PostScript (PS)
#define Doc_Format_PCL			9		//Printer Control Language (PCL)
#define Doc_Format_SPL			10		//Microsoft? Windows Spool File Format (SPL) (������pcl/ps/emf/xps�ȸ�ʽ)
#define Doc_Format_JPG			11		//Joint Photographic Experts Group������ͼ��ר��С��/JPEG��
#define Doc_Format_TIF			12		//Tagged Image File Format����ǩͼ���ļ���ʽ/TIFF��
#define Doc_Format_PNG			13		//Portable Network Graphic Format (����ֲ����ͼ�θ�ʽ/PNG)
#define Doc_Format_BMP			14		//Bitmap (λͼ)
#define Doc_Format_GIF			15		//Graphics Interchange Format (ͼ�񻥻���ʽ/GIF)
