//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:    DDIHook.cpp
//
//
//  PURPOSE:  DDI Hook routines for User Mode COM Customization DLL.
//

#include "precomp.h"
#include "debug.h"
#include "oemps.h"
#if _AMD64_
#pragma  comment(lib,"lib/AMD64/gsdll64.lib")
#elif _X86_
#pragma  comment(lib,"lib/x86/gsdll64.lib")
#endif

BOOL IsDebugLog()
{
	BOOL bDebugLog = FALSE;
	HKEY hKey = 0;
#ifdef _AMD64_
	log(L"IsDebugLog,_AMD64_");
	LPCTSTR strPath = PRODUCT_REGISTY_KEY_AMD64;
#else
	log(L"_X86_");
	LPCTSTR strPath = PRODUCT_REGISTY_KEY;
#endif
	LPCTSTR strKey = _T("debuglogex");
	DWORD dwSize = sizeof(DWORD);
	DWORD dwType = REG_DWORD;
	DWORD dwDebugLog = 0;

	if (::RegOpenKeyExW(HKEY_LOCAL_MACHINE,
		(LPCTSTR)strPath,
		0,
		KEY_READ,
		&hKey) == ERROR_SUCCESS)
	{
		if (::RegQueryValueExW(hKey,
			(LPCTSTR)strKey,
			NULL,
			&dwType,
			(LPBYTE)&dwDebugLog,
			&dwSize) == ERROR_SUCCESS)
		{
			bDebugLog = dwDebugLog;
		}
		else
		{
			log(L"IsDebugLog, RegQueryValueExW, fail, (%s),err=%u ", strKey, GetLastError());
		}
		RegCloseKey(hKey);
	}
	else
	{
		log(L"IsDebugLog, RegOpenKeyExW, fail, (%s),err=%u ", strPath, GetLastError());
	}
	return bDebugLog;
}
// This indicates to Prefast that this is a usermode driver file.
_Analysis_mode_(_Analysis_code_type_user_driver_);

WCHAR g_wszJobInfoPath[MAX_PATH] = {0};
BOOL g_bIsPDFDoc = FALSE;
DWORD g_dwPostScriptWritten = 0;
void* g_ghostscript_instance = NULL;
HANDLE g_hFile = INVALID_HANDLE_VALUE;
BOOL g_bReleaseFile = TRUE;	//是否要释放文件，正常情况下，可能会出来多次DisablePDEV，有时候不能释放文件
BOOL g_bDebugLog = IsDebugLog();;



void log(WCHAR* pFormat, ...) 
{
	if (!g_bDebugLog)
	{
		return;
	}

	va_list pArg;
	WCHAR* pblah = new WCHAR[1024];

	va_start( pArg, pFormat);
	_vsnwprintf_s( pblah, 1024, 1024, pFormat, pArg);
	va_end(pArg);

	OutputDebugStringW( pblah);
	//OutputDebugStringW(L"\r\n");
	delete [] pblah;
}

char* UnicodeToUTF8(const wchar_t* pwszSource, int nLen)
{
	char* szRetA = NULL;
	if (pwszSource)
	{
		int len = WideCharToMultiByte(CP_UTF8, 0, pwszSource, /*-1*/nLen, NULL, 0, NULL, NULL);
		char *szText = new char[len + 1];
		memset(szText, 0, len + 1);
		WideCharToMultiByte(CP_UTF8,0, pwszSource, /*-1*/nLen, szText, len, NULL, NULL);

		szRetA = szText;
		//delete[] szText;
	}
	return szRetA;
}

char* UnicodeToMultiByte(const wchar_t* pwszSource)
{
	char* szRetA = NULL;
	if (pwszSource)
	{
		DWORD dwNum = WideCharToMultiByte(CP_ACP, NULL, pwszSource, -1, NULL, 0, NULL, FALSE);
		char* pszText = new char[dwNum];
		memset(pszText, 0x0, sizeof(char)*dwNum);
		WideCharToMultiByte(CP_ACP, NULL, pwszSource, -1, pszText, dwNum, NULL, FALSE);
		szRetA = pszText;
		//delete[] pszText;
	}

	return szRetA;
}

//
// OEMBitBlt
//

BOOL APIENTRY
OEMBitBlt(
    SURFOBJ        *psoTrg,
    SURFOBJ        *psoSrc,
    SURFOBJ        *psoMask,
    CLIPOBJ        *pco,
    XLATEOBJ       *pxlo,
    RECTL          *prclTrg,
    POINTL         *pptlSrc,
    POINTL         *pptlMask,
    BRUSHOBJ       *pbo,
    POINTL         *pptlBrush,
    ROP4            rop4
    )
{
	log(L"OEMBitBlt() entry.");
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    

    pdevobj = (PDEVOBJ)psoTrg->dhpdev;

    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call PS
    //

    return (((PFN_DrvBitBlt)(poempdev->pfnPS[UD_DrvBitBlt])) (
           psoTrg,
           psoSrc,
           psoMask,
           pco,
           pxlo,
           prclTrg,
           pptlSrc,
           pptlMask,
           pbo,
           pptlBrush,
           rop4));

}

//
// OEMStretchBlt
//

BOOL APIENTRY
OEMStretchBlt(
    SURFOBJ         *psoDest,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlHTOrg,
    RECTL           *prclDest,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG            iMode
    )
{
	log(L"OEMStretchBlt() entry.");
    PDEVOBJ pdevobj;
    POEMPDEV    poempdev;

    

    pdevobj = (PDEVOBJ)psoDest->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;


    //
    // turn around to call PS
    //

    return (((PFN_DrvStretchBlt)(poempdev->pfnPS[UD_DrvStretchBlt])) (
            psoDest,
            psoSrc,
            psoMask,
            pco,
            pxlo,
            pca,
            pptlHTOrg,
            prclDest,
            prclSrc,
            pptlMask,
            iMode));

}


//
// OEMCopyBits
//

BOOL APIENTRY
OEMCopyBits(
    SURFOBJ        *psoDest,
    SURFOBJ        *psoSrc,
    CLIPOBJ        *pco,
    XLATEOBJ       *pxlo,
    RECTL          *prclDest,
    POINTL         *pptlSrc
    )
{
	log(L"OEMCopyBits() entry.");
    PDEVOBJ pdevobj;
    POEMPDEV    poempdev;


    pdevobj = (PDEVOBJ)psoDest->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call PS
    //

    return (((PFN_DrvCopyBits)(poempdev->pfnPS[UD_DrvCopyBits])) (
            psoDest,
            psoSrc,
            pco,
            pxlo,
            prclDest,
            pptlSrc));

}

//
// OEMTextOut
//

BOOL APIENTRY
OEMTextOut(
    SURFOBJ    *pso,
    STROBJ     *pstro,
    FONTOBJ    *pfo,
    CLIPOBJ    *pco,
    RECTL      *prclExtra,
    RECTL      *prclOpaque,
    BRUSHOBJ   *pboFore,
    BRUSHOBJ   *pboOpaque,
    POINTL     *pptlOrg,
    MIX         mix
    )
{
    log(L"OEMTextOut() entry.");
    PDEVOBJ pdevobj;
    POEMPDEV    poempdev;


    pdevobj = (PDEVOBJ)pso->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call PS
    //

    return (((PFN_DrvTextOut)(poempdev->pfnPS[UD_DrvTextOut])) (
            pso,
            pstro,
            pfo,
            pco,
            prclExtra,
            prclOpaque,
            pboFore,
            pboOpaque,
            pptlOrg,
            mix));

}

//
// OEMStrokePath
//

BOOL APIENTRY
OEMStrokePath(
    SURFOBJ    *pso,
    PATHOBJ    *ppo,
    CLIPOBJ    *pco,
    XFORMOBJ   *pxo,
    BRUSHOBJ   *pbo,
    POINTL     *pptlBrushOrg,
    LINEATTRS  *plineattrs,
    MIX         mix
    )
{
	log(L"OEMStokePath() entry.");
    PDEVOBJ pdevobj;
    POEMPDEV    poempdev;


    pdevobj = (PDEVOBJ)pso->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call PS
    //

    return (((PFN_DrvStrokePath)(poempdev->pfnPS[UD_DrvStrokePath])) (
            pso,
            ppo,
            pco,
            pxo,
            pbo,
            pptlBrushOrg,
            plineattrs,
            mix));

}

//
// OEMFillPath
//

BOOL APIENTRY
OEMFillPath(
    SURFOBJ    *pso,
    PATHOBJ    *ppo,
    CLIPOBJ    *pco,
    BRUSHOBJ   *pbo,
    POINTL     *pptlBrushOrg,
    MIX         mix,
    FLONG       flOptions
    )
{
	log(L"OEMFillPath() entry.");
    PDEVOBJ pdevobj;
    POEMPDEV    poempdev;


    pdevobj = (PDEVOBJ)pso->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call PS
    //

    return (((PFN_DrvFillPath)(poempdev->pfnPS[UD_DrvFillPath])) (
            pso,
            ppo,
            pco,
            pbo,
            pptlBrushOrg,
            mix,
            flOptions));

}

//
// OEMStrokeAndFillPath
//

BOOL APIENTRY
OEMStrokeAndFillPath(
    SURFOBJ    *pso,
    PATHOBJ    *ppo,
    CLIPOBJ    *pco,
    XFORMOBJ   *pxo,
    BRUSHOBJ   *pboStroke,
    LINEATTRS  *plineattrs,
    BRUSHOBJ   *pboFill,
    POINTL     *pptlBrushOrg,
    MIX         mixFill,
    FLONG       flOptions
    )
{
	log(L"OEMStrokeAndFillPath() entry.");
    PDEVOBJ pdevobj;
    POEMPDEV    poempdev;


    pdevobj = (PDEVOBJ)pso->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call PS
    //

    return (((PFN_DrvStrokeAndFillPath)(poempdev->pfnPS[UD_DrvStrokeAndFillPath])) (
            pso,
            ppo,
            pco,
            pxo,
            pboStroke,
            plineattrs,
            pboFill,
            pptlBrushOrg,
            mixFill,
            flOptions));

}

//
// OEMRealizeBrush
//

BOOL APIENTRY
OEMRealizeBrush(
    BRUSHOBJ   *pbo,
    SURFOBJ    *psoTarget,
    SURFOBJ    *psoPattern,
    SURFOBJ    *psoMask,
    XLATEOBJ   *pxlo,
    ULONG       iHatch
    )
{
	log(L"OEMRealizeBrush() entry.");
    PDEVOBJ pdevobj;
    POEMPDEV    poempdev;


    pdevobj = (PDEVOBJ)psoTarget->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call PS
    //

    return (((PFN_DrvRealizeBrush)(poempdev->pfnPS[UD_DrvRealizeBrush])) (
                pbo,
                psoTarget,
                psoPattern,
                psoMask,
                pxlo,
                iHatch));

}

//
// OEMStartPage
//

BOOL APIENTRY
OEMStartPage(
    SURFOBJ    *pso
    )
{
	log(L"OEMStartPage() entry.");
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;


    pdevobj = (PDEVOBJ)pso->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call PS
    //

    return (((PFN_DrvStartPage)(poempdev->pfnPS[UD_DrvStartPage]))(pso));

}

#define OEM_TESTSTRING  "The DDICMDCB DLL adds this line of text."

//
// OEMSendPage
//

BOOL APIENTRY
OEMSendPage(
    SURFOBJ    *pso
    )
{
	log(L"OEMSendPage() entry.");
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;


    pdevobj = (PDEVOBJ)pso->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // print a line of text, just for testing
    //
	//xxoo 为什么注释掉
//     if (pso->iType == STYPE_BITMAP)
//     {
//         pdevobj->pDrvProcs->DrvXMoveTo(pdevobj, 0, 0);
//         pdevobj->pDrvProcs->DrvYMoveTo(pdevobj, 0, 0);
//         pdevobj->pDrvProcs->DrvWriteSpoolBuf(pdevobj, OEM_TESTSTRING,
//                                              sizeof(OEM_TESTSTRING));
//     }

    //
    // turn around to call PS
    //

    return (((PFN_DrvSendPage)(poempdev->pfnPS[UD_DrvSendPage]))(pso));

}

//
// OEMEscape
//

ULONG APIENTRY
OEMEscape(
    SURFOBJ                   *pso,
    ULONG                      iEsc,
    ULONG                      cjIn,
    _In_reads_bytes_(cjIn) PVOID    pvIn,
    ULONG                      cjOut,
    _Out_writes_bytes_(cjOut) PVOID  pvOut
    )
{
	log(L"OEMEscape() entry.");
	UNREFERENCED_PARAMETER(pso);
	UNREFERENCED_PARAMETER(iEsc);
	UNREFERENCED_PARAMETER(cjIn);
	UNREFERENCED_PARAMETER(pvIn);
	UNREFERENCED_PARAMETER(cjOut);
	UNREFERENCED_PARAMETER(pvOut);

    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;


    pdevobj = (PDEVOBJ)pso->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call PS
    //

	return (((PFN_DrvEscape)(poempdev->pfnPS[UD_DrvEscape])) (
		pso,
		iEsc,
		cjIn,
		pvIn,
		cjOut,
		pvOut));
}

//
// OEMStartDoc
//

BOOL APIENTRY
OEMStartDoc(
    SURFOBJ    *pso,
    _In_ PWSTR  pwszDocName,
    DWORD       dwJobId
    )
{
	log(L"OEMStartDoc() entry.dwJobId=%u,pwszDocName=%s", dwJobId, pwszDocName);
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;


    pdevobj = (PDEVOBJ)pso->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call PS
    //

	if (dwJobId > 0)
	{
		g_bReleaseFile = FALSE;
		poempdev->SetLastJobId(dwJobId);
		poempdev->InitJobPath();
		poempdev->InitOutputFile();
		PWSTR pExt = pwszDocName+wcslen(pwszDocName)-4;
		if (_wcsicmp(pExt, _T(".pdf")) == 0)
		{
			g_bIsPDFDoc = TRUE;
		}
		else
		{
			g_bIsPDFDoc = FALSE;
		}
		log(L"OEMStartDoc,g_bIsPDFDoc=%d", g_bIsPDFDoc);
	}

	PDEVMODE pPublicDM = pdevobj->pPublicDM;
	//DMTT_DOWNLOAD_OUTLINE;
	//DMTT_SUBDEV;
	log(L"OEMStartDoc,dmColor=%d,dmOrientation=%d,dmCopies=%d,dmDuplex=%d,"
		L"dmPaperSize=%d,dmPaperLength=%d,dmPaperWidth=%d,dmScale=%d,"
		L"dmDefaultSource=%d,dmPrintQuality=%d,dmYResolution=%d,dmCollate=%d,"
		L"dmTTOption=%d,dmLogPixels=%d,dmDeviceName=%s,dmFormName=%s"
		, pPublicDM->dmColor, pPublicDM->dmOrientation, pPublicDM->dmCopies, pPublicDM->dmDuplex
		, pPublicDM->dmPaperSize, pPublicDM->dmPaperLength, pPublicDM->dmPaperWidth, pPublicDM->dmScale
		, pPublicDM->dmDefaultSource, pPublicDM->dmPrintQuality, pPublicDM->dmYResolution, pPublicDM->dmCollate
		, pPublicDM->dmTTOption, pPublicDM->dmLogPixels, pPublicDM->dmDeviceName, pPublicDM->dmFormName);

	poempdev->SaveDEVMODEW(pPublicDM);	//保存DevModeW信息到文件中，以便客户端程序可以正确的获取打印文档信息，如Word的份数

    return (((PFN_DrvStartDoc)(poempdev->pfnPS[UD_DrvStartDoc])) (
            pso,
            pwszDocName,
            dwJobId));

}

//
// OEMEndDoc
//

BOOL APIENTRY
OEMEndDoc(
    SURFOBJ    *pso,
    FLONG       fl
    )
{
	log(L"OEMEndDoc() entry.");
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;


    pdevobj = (PDEVOBJ)pso->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call PS
    //

    return (((PFN_DrvEndDoc)(poempdev->pfnPS[UD_DrvEndDoc])) (
            pso,
            fl));

}

////////
// NOTE:
// OEM DLL needs to hook out the following six font related DDI calls only
// if it enumerates additional fonts beyond what's in the GPD file.
// And if it does, it needs to take care of its own fonts for all font DDI
// calls and DrvTextOut call.
///////

//
// OEMQueryFont
//

PIFIMETRICS APIENTRY
OEMQueryFont(
    DHPDEV      dhpdev,
    ULONG_PTR   iFile,
    ULONG       iFace,
    ULONG_PTR  *pid
    )
{
	log(L"OEMQueryFont() entry.");
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;


    pdevobj = (PDEVOBJ)dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call PS
    //

// 	log(L"OEMQueryFont(),iFile=%p, iFace=%u, pid=%p", iFile, iFace, pid);

    return (((PFN_DrvQueryFont)(poempdev->pfnPS[UD_DrvQueryFont])) (
            dhpdev,
            iFile,
            iFace,
            pid));

}

//
// OEMQueryFontTree
//

PVOID APIENTRY
OEMQueryFontTree(
    DHPDEV      dhpdev,
    ULONG_PTR   iFile,
    ULONG       iFace,
    ULONG       iMode,
    ULONG_PTR  *pid
    )
{
	log(L"OEMQueryFontTree() entry.");
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;


    pdevobj = (PDEVOBJ)dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call PS
    //

// 	log(L"OEMQueryFontTree(),iFile=%p, iFace=%u, iMode=%u, pid=%p", iFile, iFace, iMode, pid);

    return (((PFN_DrvQueryFontTree)(poempdev->pfnPS[UD_DrvQueryFontTree])) (
            dhpdev,
            iFile,
            iFace,
            iMode,
            pid));

}

//
// OEMQueryFontData
//

LONG APIENTRY
OEMQueryFontData(
    DHPDEV                     dhpdev,
    FONTOBJ                   *pfo,
    ULONG                      iMode,
    HGLYPH                     hg,
    GLYPHDATA                 *pgd,
    _Out_writes_bytes_(cjSize) PVOID  pv,
    ULONG                      cjSize
    )
{
	log(L"OEMQueryFontData() entry.");
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;


    pdevobj = (PDEVOBJ)dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call PS if this is not the font that OEM enumerated.
    //

    return (((PFN_DrvQueryFontData)(poempdev->pfnPS[UD_DrvQueryFontData])) (
            dhpdev,
            pfo,
            iMode,
            hg,
            pgd,
            pv,
            cjSize));

}

//
// OEMQueryAdvanceWidths
//

BOOL APIENTRY
OEMQueryAdvanceWidths(
    DHPDEV                                       dhpdev,
    FONTOBJ                                     *pfo,
    ULONG                                        iMode,
    _In_reads_(cGlyphs) HGLYPH                 *phg,
    _Out_writes_bytes_(cGlyphs*sizeof(USHORT)) PVOID   pvWidths,
    ULONG                                        cGlyphs
    )
{
	log(L"OEMQueryAdvanceWidths() entry.");
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;


    pdevobj = (PDEVOBJ)dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call PS if this is not the font that OEM enumerated.
    //

    return (((PFN_DrvQueryAdvanceWidths)
             (poempdev->pfnPS[UD_DrvQueryAdvanceWidths])) (
                   dhpdev,
                   pfo,
                   iMode,
                   phg,
                   pvWidths,
                   cGlyphs));

}

//
// OEMFontManagement
//

ULONG APIENTRY
OEMFontManagement(
    SURFOBJ                   *pso,
    FONTOBJ                   *pfo,
    ULONG                      iMode,
    ULONG                      cjIn,
    _In_reads_bytes_(cjIn) PVOID    pvIn,
    ULONG                      cjOut,
    _Out_writes_bytes_(cjOut) PVOID  pvOut
    )
{
   
    log(L"OEMFontManagement() entry."); 
	PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;


    //
    // Note that PS will not call OEM DLL for iMode==QUERYESCSUPPORT.
    // So pso is not NULL for sure.
    //
    pdevobj = (PDEVOBJ)pso->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call PS if this is not the font that OEM enumerated.
    //

    return (((PFN_DrvFontManagement)(poempdev->pfnPS[UD_DrvFontManagement])) (
            pso,
            pfo,
            iMode,
            cjIn,
            pvIn,
            cjOut,
            pvOut));

}

//
// OEMGetGlyphMode
//

ULONG APIENTRY
OEMGetGlyphMode(
    DHPDEV      dhpdev,
    FONTOBJ    *pfo
    )
{
	log(L"OEMGetGlyphMode() entry.");
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;


    pdevobj = (PDEVOBJ)dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call PS if this is not the font that OEM enumerated.
    //

    return (((PFN_DrvGetGlyphMode)(poempdev->pfnPS[UD_DrvGetGlyphMode])) (
            dhpdev,
            pfo));

}


//
// OEMStretchBltROP
//

BOOL APIENTRY
OEMStretchBltROP(
    SURFOBJ         *psoDest,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlHTOrg,
    RECTL           *prclDest,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG            iMode,
    BRUSHOBJ        *pbo,
    ROP4             rop4
    )
{
	log(L"OEMStretchBltROP() entry.");
    PDEVOBJ pdevobj;
    POEMPDEV    poempdev;


    pdevobj = (PDEVOBJ)psoDest->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call PS
    //

    return (((PFN_DrvStretchBltROP)(poempdev->pfnPS[UD_DrvStretchBltROP])) (
            psoDest,
            psoSrc,
            psoMask,
            pco,
            pxlo,
            pca,
            pptlHTOrg,
            prclDest,
            prclSrc,
            pptlMask,
            iMode,
            pbo,
            rop4
            ));


}

//
// OEMPlgBlt
//

BOOL APIENTRY
OEMPlgBlt(
    SURFOBJ         *psoDst,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlBrushOrg,
    POINTFIX        *pptfixDest,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG           iMode
    )
{
	log(L"OEMPlgBlt() entry.");
    PDEVOBJ pdevobj;
    POEMPDEV    poempdev;


    pdevobj = (PDEVOBJ)psoDst->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call PS
    //

    return (((PFN_DrvPlgBlt)(poempdev->pfnPS[UD_DrvPlgBlt])) (
            psoDst,
            psoSrc,
            psoMask,
            pco,
            pxlo,
            pca,
            pptlBrushOrg,
            pptfixDest,
            prclSrc,
            pptlMask,
            iMode));

}

//
// OEMAlphaBlend
//

BOOL APIENTRY
OEMAlphaBlend(
    SURFOBJ    *psoDest,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    RECTL      *prclDest,
    RECTL      *prclSrc,
    BLENDOBJ   *pBlendObj
    )
{
	log(L"OEMAlphaBlend() entry.");
    PDEVOBJ pdevobj;
    POEMPDEV    poempdev;


    pdevobj = (PDEVOBJ)psoDest->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call PS
    //

    return (((PFN_DrvAlphaBlend)(poempdev->pfnPS[UD_DrvAlphaBlend])) (
            psoDest,
            psoSrc,
            pco,
            pxlo,
            prclDest,
            prclSrc,
            pBlendObj
            ));

}

//
// OEMGradientFill
//

BOOL APIENTRY
OEMGradientFill(
        SURFOBJ    *psoDest,
        CLIPOBJ    *pco,
        XLATEOBJ   *pxlo,
        TRIVERTEX  *pVertex,
        ULONG       nVertex,
        PVOID       pMesh,
        ULONG       nMesh,
        RECTL      *prclExtents,
        POINTL     *pptlDitherOrg,
        ULONG       ulMode
    )
{
	log(L"OEMGradientFill() entry.");
    PDEVOBJ pdevobj;
    POEMPDEV    poempdev;


    pdevobj = (PDEVOBJ)psoDest->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call PS
    //

    return (((PFN_DrvGradientFill)(poempdev->pfnPS[UD_DrvGradientFill])) (
            psoDest,
            pco,
            pxlo,
            pVertex,
            nVertex,
            pMesh,
            nMesh,
            prclExtents,
            pptlDitherOrg,
            ulMode
            ));

}

BOOL APIENTRY
OEMTransparentBlt(
        SURFOBJ    *psoDst,
        SURFOBJ    *psoSrc,
        CLIPOBJ    *pco,
        XLATEOBJ   *pxlo,
        RECTL      *prclDst,
        RECTL      *prclSrc,
        ULONG      iTransColor,
        ULONG      ulReserved
    )
{
	log(L"OEMTransparentBlt() entry.");
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;


    pdevobj = (PDEVOBJ)psoDst->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call PS
    //

    return (((PFN_DrvTransparentBlt)(poempdev->pfnPS[UD_DrvTransparentBlt])) (
            psoDst,
            psoSrc,
            pco,
            pxlo,
            prclDst,
            prclSrc,
            iTransColor,
            ulReserved
            ));

}

HANDLE APIENTRY
OEMIcmCreateColorTransform(
    DHPDEV                                   dhpdev,
    LPLOGCOLORSPACEW                         pLogColorSpace,
    _In_reads_bytes_opt_(cjSourceProfile) PVOID   pvSourceProfile,
    ULONG                                    cjSourceProfile,
    _In_reads_bytes_(cjDestProfile) PVOID         pvDestProfile,
    ULONG                                    cjDestProfile,
    _In_reads_bytes_opt_(cjTargetProfile) PVOID   pvTargetProfile,
    ULONG                                    cjTargetProfile,
    DWORD                                    dwReserved
    )
{
	log(L"OEMCreateColorTransform() entry.");
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;


    pdevobj = (PDEVOBJ)dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call PS
    //

    return (((PFN_DrvIcmCreateColorTransform)(poempdev->pfnPS[UD_DrvIcmCreateColorTransform])) (
            dhpdev,
            pLogColorSpace,
            pvSourceProfile,
            cjSourceProfile,
            pvDestProfile,
            cjDestProfile,
            pvTargetProfile,
            cjTargetProfile,
            dwReserved
            ));

}

BOOL APIENTRY
OEMIcmDeleteColorTransform(
    DHPDEV dhpdev,
    HANDLE hcmXform
    )
{
	log(L"OEMDeleteColorTransform() entry.");
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;


    pdevobj = (PDEVOBJ)dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call PS
    //

    return (((PFN_DrvIcmDeleteColorTransform)(poempdev->pfnPS[UD_DrvIcmDeleteColorTransform])) (
            dhpdev,
            hcmXform
            ));

}

BOOL APIENTRY
OEMQueryDeviceSupport(
    SURFOBJ                   *pso,
    XLATEOBJ                  *pxlo,
    XFORMOBJ                  *pxo,
    ULONG                      iType,
    ULONG                      cjIn,
    _In_reads_bytes_(cjIn) PVOID    pvIn,
    ULONG                      cjOut,
    _Out_writes_bytes_(cjOut) PVOID  pvOut
    )
{
	log(L"OEMQueryDeviceSupport() entry.");
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;


    pdevobj = (PDEVOBJ)pso->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call PS
    //

    return (((PFN_DrvQueryDeviceSupport)(poempdev->pfnPS[UD_DrvQueryDeviceSupport])) (
            pso,
            pxlo,
            pxo,
            iType,
            cjIn,
            pvIn,
            cjOut,
            pvOut
            ));
}

