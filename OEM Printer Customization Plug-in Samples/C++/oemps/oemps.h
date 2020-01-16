//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	OEMPS.H
//    
//
//  PURPOSE:	Define common data types, and external function prototypes
//				for oemps.cpp.
//
#pragma once

#include <gdevdsp.h>
#include <iapi.h>
#include <ierrors.h>

extern void* g_ghostscript_instance;
extern HANDLE g_hFile;

///////////////////////////////////////////////////////
// Warning: the following enum order must match the 
//          order in OEMHookFuncs[].
///////////////////////////////////////////////////////
typedef enum tag_Hooks {
    UD_DrvRealizeBrush,
    UD_DrvCopyBits,
    UD_DrvBitBlt,
    UD_DrvStretchBlt,
    UD_DrvTextOut,
    UD_DrvStrokePath,
    UD_DrvFillPath,
    UD_DrvStrokeAndFillPath,
    UD_DrvStartPage,
    UD_DrvSendPage,
    UD_DrvEscape,
    UD_DrvStartDoc,
    UD_DrvEndDoc,
    UD_DrvQueryFont,
    UD_DrvQueryFontTree,
    UD_DrvQueryFontData,
    UD_DrvQueryAdvanceWidths,
    UD_DrvFontManagement,
    UD_DrvGetGlyphMode,
    UD_DrvStretchBltROP,
    UD_DrvPlgBlt,
    UD_DrvTransparentBlt,
    UD_DrvAlphaBlend,
    UD_DrvGradientFill,
    UD_DrvIcmCreateColorTransform,
    UD_DrvIcmDeleteColorTransform,
    UD_DrvQueryDeviceSupport,

    MAX_DDI_HOOKS,

} ENUMHOOKS;

typedef class _OEMPDEV {

public:
	_OEMPDEV(void)
	{
		
		log(L"_OEMPDEV ++++");
		memset(m_wszPrinter,0,sizeof(m_wszPrinter));
		memset(m_wszJobPath,0,sizeof(m_wszJobPath));
		m_dwJobId = 0;
		m_dwSaveFormat = GetSaveFormat();
	}

	virtual ~_OEMPDEV(void)
	{
		log(L"~_OEMPDEV ----");
	}

    //
    // define whatever needed, such as working buffers, tracking information,
    // etc.
    //
    // This test DLL hooks out every drawing DDI. So it needs to remember
    // PS's hook function pointer so it call back.
    //
    PFN     pfnPS[MAX_DDI_HOOKS];

    //
    // define whatever needed, such as working buffers, tracking information,
    // etc.
    //
    DWORD     dwReserved[1];

	//
	DWORD m_dwJobId;
	DWORD m_dwSaveFormat;
	WCHAR m_wszPrinter[MAX_PATH];
	WCHAR m_wszJobPath[MAX_PATH];

	void SetLastJobId(DWORD dwID)
	{
		log(L"SetLastJobId %d",dwID);

		HKEY hKey = 0;
		LPCTSTR strPath = m_wszPrinter;//PROJECT_REGISTY_KEY;
		LPCTSTR strKey=_T("LastJobId");
		DWORD dwSize=sizeof(DWORD), dwType=REG_DWORD;

		m_dwJobId = dwID;

		if(::RegOpenKeyExW(HKEY_LOCAL_MACHINE,
			(LPCTSTR)strPath,
			0,
			KEY_READ | KEY_SET_VALUE,
			&hKey)!= ERROR_SUCCESS)
		{
			log(L"RegOpenKeyExW err = %d,%s",::GetLastError(),strPath);
			return ;
		}
		if(::RegSetValueExW(hKey,
			(LPCTSTR)strKey,
			NULL,
			dwType,
			(LPBYTE)&dwID,
			dwSize) != ERROR_SUCCESS)
		{
			log(L"RegSetValueExW err = %d,%s",::GetLastError(),strPath);
		}

		RegCloseKey(hKey);
	}
	WCHAR* GetJobPath()
	{
		log(L"GetJobPath,m_wszJobPath=%s", m_wszJobPath);
		return m_wszJobPath;
	}
	DWORD GetSaveFormat()
	{
		DWORD dwSaveFormat = Doc_Format_PS;
		HKEY hKey = 0;
#ifdef _AMD64_
		log(L"GetSaveFormat,_AMD64_");
		LPCTSTR strPath = PRODUCT_REGISTY_KEY_AMD64;
#else
		log(L"_X86_");
		LPCTSTR strPath = PRODUCT_REGISTY_KEY;
#endif
		LPCTSTR strKey = _T("Custom");
		DWORD dwSize = sizeof(WCHAR)*MAX_PATH;
		DWORD dwType = REG_SZ;
		WCHAR szCustom[MAX_PATH] = {0};

		if(::RegOpenKeyExW(HKEY_LOCAL_MACHINE,
			(LPCTSTR)strPath,
			0,
			KEY_READ,
			&hKey)== ERROR_SUCCESS)
		{
			if(::RegQueryValueExW(hKey,
				(LPCTSTR)strKey,
				NULL,
				&dwType,
				(LPBYTE)szCustom,
				&dwSize) == ERROR_SUCCESS)
			{
				log(L"Custom = %s",szCustom);
				if (wcscmp(szCustom, _T("pdf")) == 0)
				{
					dwSaveFormat = Doc_Format_PDF;
				}
			}
			else
			{
				log(L"GetSaveFormat, RegQueryValueExW, fail, (%s),err=%u ", strKey, GetLastError());
			}
			RegCloseKey(hKey); 
		}
		else
		{
			log(L"GetSaveFormat, RegOpenKeyExW, fail, (%s),err=%u ", strPath, GetLastError());
		}
		return dwSaveFormat;
	}

	void InitPrinterRegPath(HANDLE hPrinter)
	{
		if (!hPrinter)
		{
			log(L"!!InitPrinterRegPath,1,hPrinter=%d", hPrinter);
			return;
		}

		DWORD dwBytesNeeded = 0;
		DWORD dwBytesReturned = 0;
		GetPrinter(hPrinter, 4, NULL, 0, &dwBytesNeeded);
		PRINTER_INFO_4* p4 = (PRINTER_INFO_4*)new BYTE[dwBytesNeeded];
		if (!GetPrinter(hPrinter, 4, (LPBYTE)p4, dwBytesNeeded, &dwBytesReturned)) 
		{
			delete (BYTE*)p4;
			return;
		}
		
		int nLen = (int)wcslen(p4->pPrinterName);
		log(L"InitPrinterRegPath,2,pPrinterName=%s,nLen=%d", p4->pPrinterName, nLen);
		if (nLen <= 0)
		{
			return;
		}
		StringCchPrintf(m_wszPrinter,MAX_PATH,L"%s\\%s",PRINTER_REGISTY_KEY, p4->pPrinterName);
		log(L"InitPrinterRegPath,3,m_wszPrinter=%s", m_wszPrinter);
		delete (BYTE*)p4;
	}

	void InitJobPath()
	{
		HKEY hKey = 0;
		LPCTSTR strPath = m_wszPrinter;//PROJECT_REGISTY_KEY;
		LPCTSTR strKey=_T("LastJobId");
		DWORD dwRet, dwSize=sizeof(DWORD), dwType=REG_DWORD;

		if(::RegOpenKeyExW(HKEY_LOCAL_MACHINE,
			(LPCTSTR)strPath,
			0,
			KEY_READ,
			&hKey)!= ERROR_SUCCESS)
			return ;

		WCHAR szPath[MAX_PATH *2] = {0};
		dwType = REG_SZ;
		dwSize = MAX_PATH * 2;
		if(::RegQueryValueExW(hKey,
			(LPCTSTR)_T("JobDir"),
			NULL,
			&dwType,
			(LPBYTE)szPath,
			&dwSize) == ERROR_SUCCESS)
		{
		}

		if(::RegQueryValueExW(hKey,
			(LPCTSTR)strKey,
			NULL,
			&dwType,
			(LPBYTE)&dwRet,
			&dwSize) == ERROR_SUCCESS)
		{
			switch (m_dwSaveFormat)
			{
			case Doc_Format_PDF:
				StringCchPrintf(m_wszJobPath,MAX_PATH,L"%s\\job_%06d.pdf",szPath,m_dwJobId);
				break;
			default:
				StringCchPrintf(m_wszJobPath,MAX_PATH,L"%s\\job_%06d.ps",szPath,m_dwJobId);
				break;
			}
			StringCchPrintf(g_wszJobInfoPath,MAX_PATH,L"%s\\job_%06d.dev",szPath,m_dwJobId);
		}
		log(L"InitJobPath,m_wszJobPath = %s",m_wszJobPath);
		log(L"InitJobPath,g_wszJobInfoPath = %s",g_wszJobInfoPath);
		RegCloseKey(hKey); 
	}

	void InitOutputFile()
	{
		log(L"InitOutputFile,begin,m_wszJobPath=%s", m_wszJobPath);
		
		ReleaseOutputFile();

		size_t len = 0;
		StringCchLength(m_wszJobPath,MAX_PATH,&len);
		if (len <= 0)
		{
			log(L"!!InitOutputFile,m_wszJobPath=%s", m_wszJobPath);
			return;
		}
		if (m_dwSaveFormat == Doc_Format_PDF)
		{
			InitGhostScript();
		}
		else
		{
			if (g_hFile == INVALID_HANDLE_VALUE)
			{
				g_hFile = CreateFile(m_wszJobPath,GENERIC_WRITE|GENERIC_READ,0,0,CREATE_ALWAYS,0,0);
				if(g_hFile == INVALID_HANDLE_VALUE)
				{	
					log(L"!!InitOutputFile,CreateFile fail,err=%u,m_wszJobPath=%s", GetLastError(), m_wszJobPath);
				}
			}
		}
		log(L"InitOutputFile,end");
	}

	BOOL WriteOutputFile(PVOID pData, DWORD cbSize, PDWORD pcbWritten)
	{
		BOOL bRet = FALSE;
		*pcbWritten = 0;
		if (m_dwSaveFormat == Doc_Format_PDF)
		{
			if (ProcessGhostScript(pData, cbSize))
			{
				*pcbWritten = cbSize;
				bRet = TRUE;
			}
		}
		else
		{
			if (g_hFile != INVALID_HANDLE_VALUE)
			{
				bRet = WriteFile(g_hFile, pData, cbSize, pcbWritten, NULL);
			}
		}
		return bRet;
	}

	void ReleaseOutputFile()
	{
		log(L"ReleaseOutputFile,begin,m_dwSaveFormat=%u", m_dwSaveFormat);
		if (m_dwSaveFormat == Doc_Format_PDF)
		{
			ReleaseGhostScript();
		}
		else
		{
			if (g_hFile != INVALID_HANDLE_VALUE)
			{
				CloseHandle(g_hFile);
				g_hFile = INVALID_HANDLE_VALUE;
			}
		}
		log(L"ReleaseOutputFile,end");
	}

	int	critic_error_code (int code)
	{
		if (code >= 0)
			return FALSE;

		if (code <= -100) 
		{
			switch (code) 
			{
			case gs_error_Fatal:
				log(L"fatal internal error %d", code);
				return TRUE;
				break;
			case gs_error_ExecStackUnderflow:
				log(L"stack overflow %d", code);
				return TRUE;
				break;
				/* no error or not important */
			default:
				return FALSE;
			}
		} 
		else 
		{
			const char *errors[] = { "", ERROR_NAMES };
			int x = (-1) * code;

			if (x < (int) (sizeof (errors) / sizeof (const char*))) 
			{
				log(L"%s %d\n", errors[x], code);
			}
			return TRUE;
		}
	}

	BOOL InitGhostScript()
	{
		log(L"InitGhostScript,begin,m_wszJobPath=%s", m_wszJobPath);

		int error;
		int exit_code;
		char *args[10];
		int arg = 0;
		char outputfile[MAX_PATH];
		int nLen = (int)wcslen(m_wszJobPath);
		sprintf_s(outputfile, MAX_PATH, "-sOutputFile=%s", UnicodeToUTF8(m_wszJobPath, nLen));

		args[arg++] = "xabvprinter"; /* This value doesn't really matter */
		args[arg++] = "-dMaxBitmap=100000000";	//100M
		args[arg++] = "-dBATCH";
		args[arg++] = "-dNOPAUSE";
		args[arg++] = "-dSAFER";
		args[arg++] = "-P-";
		args[arg++] = "-sDEVICE=pdfwrite";
		args[arg++] = outputfile;
		args[arg++] = "-c";
		args[arg++] = ".setpdfwrite";

		error = gsapi_new_instance (&g_ghostscript_instance, NULL);
		log(L"InitGhostScript,g_ghostscript_instance=%p", g_ghostscript_instance);
		if (critic_error_code (error)) 
		{
			log(L"!!InitGhostScript,gsapi_new_instance,error=%d", error);
			return FALSE;
		}

		error = gsapi_set_arg_encoding(g_ghostscript_instance, GS_ARG_ENCODING_UTF8);	//参数只支持utf8格式
		if (critic_error_code (error))
		{
			printf("InitGhostScript,gsapi_set_arg_encoding fail,nCode=%d", error);
			return FALSE;
		}

		error = gsapi_init_with_args (g_ghostscript_instance, arg, args);
		if (critic_error_code (error)) 
		{
			log(L"!!InitGhostScript,gsapi_init_with_args,error=%d", error);
			return FALSE;
		}

		error = gsapi_run_string_begin (g_ghostscript_instance, 0, &exit_code);
		if (critic_error_code (error)) 
		{
			log(L"!!InitGhostScript, gsapi_run_string_begin fail,error=%d", error);
			return FALSE;
		}

		g_dwPostScriptWritten = 0;	//初始化已经处理的PostScrit的大小为0

		log(L"InitGhostScript,end");

		return TRUE;
	}

	BOOL ProcessGhostScript(PVOID  pBuf, DWORD  cbBuffer)
	{
		if (!pBuf)
		{
			log(L"!!ProcessGhostScript, pBuf=%p", pBuf);
			return FALSE;
		}

		if (cbBuffer <= 0)
		{
			log(L"!!ProcessGhostScript, cbBuffer=%u", cbBuffer);
			return FALSE;
		}

		if (!g_ghostscript_instance)
		{
			log(L"!!ProcessGhostScript, g_ghostscript_instance=%p", g_ghostscript_instance);
			return FALSE;
		}

		//尝试删除PostScript中的加密信息
		TryRemoveEncryptInformation(pBuf, cbBuffer);

		int error;
		int exit_code;
		error = gsapi_run_string_continue (g_ghostscript_instance, (const char*)pBuf, cbBuffer, 0, &exit_code);
		if (critic_error_code (error)) 
		{
			log(L"!!ProcessGhostScript, gsapi_run_string_continue fail,error=%d", error);
			return FALSE;
		}

		log(L"ProcessGhostScript, g_ghostscript_instance=%p,pBuf=%p,cbBuffer=%u", g_ghostscript_instance, pBuf, cbBuffer);

		return TRUE;
	}

	BOOL ReleaseGhostScript()
	{
		log(L"ReleaseGhostScript,begin,g_ghostscript_instance=%p", g_ghostscript_instance);

		if (g_ghostscript_instance)
		{
			int error;
			int exit_code;
			error = gsapi_run_string_end (g_ghostscript_instance, 0, &exit_code);
			if (critic_error_code (error)) 
			{
				log(L"!!ProcessGhostScript, gsapi_run_string_end fail,error=%d", error);
			}

			gsapi_exit (g_ghostscript_instance);

			gsapi_delete_instance (g_ghostscript_instance);

			g_ghostscript_instance = NULL;
		}

		log(L"ReleaseGhostScript,end");

		return TRUE;
	}

	BOOL TryRemoveEncryptInformation(PVOID pBuf, DWORD cbBuffer)
	{
		BOOL bRemove = FALSE;
		if (!g_bIsPDFDoc)
		{
			return bRemove;
		}

		g_dwPostScriptWritten += cbBuffer;

		if (g_dwPostScriptWritten > PostScriptHeadSize)
		{
			return bRemove;
		}
		
		if (!pBuf)
		{
			return bRemove;
		}

		//PostScript文件中的加密字符串，要删除，否则GS无法转换成PDF
// 		char* pBuf = new char[1024*1024];
// 		char* pBuf2("%ADOBeginClientInjection: DocumentSetup Start \"No Re-Distill\"\r\n"
// 			"%% Removing the following eleven lines is illegal, subject to the Digital Copyright Act of 1998.\r\n"
// 			"mark currentfile eexec\r\n"
// 			"54dc5232e897cbaaa7584b7da7c23a6c59e7451851159cdbf40334cc2600\r\n"
// 			"30036a856fabb196b3ddab71514d79106c969797b119ae4379c5ac9b7318\r\n"
// 			"33471fc81a8e4b87bac59f7003cddaebea2a741c4e80818b4b136660994b\r\n"
// 			"18a85d6b60e3c6b57cc0815fe834bc82704ac2caf0b6e228ce1b2218c8c7\r\n"
// 			"67e87aef6db14cd38dda844c855b4e9c46d510cab8fdaa521d67cbb83ee1\r\n"
// 			"af966cc79653b9aca2a5f91f908bbd3f06ecc0c940097ec77e210e6184dc\r\n"
// 			"2f5777aacfc6907d43f1edb490a2a89c9af5b90ff126c0c3c5da9ae99f59\r\n"
// 			"d47040be1c0336205bf3c6169b1b01cd78f922ec384cd0fcab955c0c20de\r\n"
// 			"000000000000000000000000000000000000000000000000000000000000\r\n"
// 			"cleartomark\r\n"
// 			"%ADOEndClientInjection: DocumentSetup Start \"No Re-Distill\"\r\n");
// 		strcpy(pBuf, pBuf2);

		char* pFlag = "\"No Re-Distill\"";
		char* pTemp = strstr((char*)pBuf, pFlag);
		char* pTemp2 = NULL;
		if (pTemp)
		{
			pTemp2 = strstr(pTemp+strlen(pFlag), pFlag);
#ifdef _AMD64_
			__int64 dwDiff = pTemp2 - pTemp;
#else
			DWORD dwDiff = pTemp2 - pTemp;
#endif
			if (dwDiff > cbBuffer)
			{
				return bRemove;
			}
		}
		if (pTemp && pTemp2)
		{
			bRemove = TRUE;
			for (pTemp; pTemp<pTemp2; pTemp++)
			{
				if ((pTemp[0] == '\r') || (pTemp[0] == '\n'))
				{
					continue;
				}
				pTemp[0] = '%';
			}
		}
		return bRemove;
	}

	//保存DEVMODEW
	BOOL SaveDEVMODEW(PDEVMODEW pDevModeW)
	{
		if (!pDevModeW)
		{
			log(L"!!SaveDEVMODEW,pDevModeW=%p", pDevModeW);
			return FALSE;
		}

		log(L"SaveDEVMODEW,g_wszJobInfoPath=%s", g_wszJobInfoPath);
		if (wcslen(g_wszJobInfoPath) <= 0)
		{
			return FALSE;
		}

		HANDLE hFile = CreateFile(g_wszJobInfoPath,GENERIC_WRITE|GENERIC_READ,0,0,CREATE_ALWAYS,0,0);
		if(hFile == INVALID_HANDLE_VALUE)
		{	
			log(L"!!SaveDEVMODEW,CreateFile fail,err=%u,g_wszJobInfoPath=%s", GetLastError(), g_wszJobInfoPath);
			return FALSE;
		}

		DWORD dwSize = pDevModeW->dmSize+pDevModeW->dmDriverExtra;
		DWORD dwWritten = 0;
		if (!WriteFile(hFile, pDevModeW, dwSize, &dwWritten, NULL))
		{
			log(L"!!SaveDEVMODEW,WriteFile fail,err=%u,g_wszJobInfoPath=%s", GetLastError(), g_wszJobInfoPath);
			CloseHandle(hFile);
			return FALSE;
		}

		CloseHandle(hFile);
		return TRUE;
	}

} OEMPDEV, *POEMPDEV;






