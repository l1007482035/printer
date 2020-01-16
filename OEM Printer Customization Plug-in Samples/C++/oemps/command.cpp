//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:   command.cpp
//
//  PURPOSE:  Source module for OEM customized Command(s).
//

#include "precomp.h"
#include "oemps.h"
#include "debug.h"
#include "command.h"
#include "resource.h"

// This indicates to Prefast that this is a usermode driver file.
_Analysis_mode_(_Analysis_code_type_user_driver_);

////////////////////////////////////////////////////////
//      Internal String Literals
////////////////////////////////////////////////////////

#define Enable_Test	0	//是否启用测试，0-禁用，1-启用

static int g_nCurrentPage = 0;
static CHAR g_cBuffer[MAX_PATH] = {0};

#if Enable_Test
const CHAR TEST_BEGINSTREAM[]                   = "%%Test: Before begin stream\r\n";
const CHAR TEST_PSADOBE[]                       = "%%Test: Before %!PS-Adobe\r\n";
const CHAR TEST_PAGESATEND[]                    = "%%Test: Replace driver's %%PagesAtend\r\n";
const CHAR TEST_PAGES[]                         = "%%Test: Replace driver's %%Pages: (atend)\r\n";
const CHAR TEST_DOCUMENTPROCESSCOLORS[]         = "%%Test: Replace driver's %%DocumentProcessColors: (atend)\r\n";
const CHAR TEST_COMMENTS[]                      = "%%Test: Before %%EndComments\r\n";
const CHAR TEST_DEFAULTS[]                      = "%%Test: Before %%BeginDefaults and %%EndDefaults\r\n";
const CHAR TEST_BEGINPROLOG[]                   = "%%Test: After %%BeginProlog\r\n";
const CHAR TEST_ENDPROLOG[]                     = "%%Test: Before %%EndProlog\r\n";
const CHAR TEST_BEGINSETUP[]                    = "%%Test: After %%BeginSetup\r\n";
const CHAR TEST_ENDSETUP[]                      = "%%Test: Before %%EndSetup\r\n";
const CHAR TEST_BEGINPAGESETUP[]                = "%%Test: After %%BeginPageSetup\r\n";
const CHAR TEST_ENDPAGESETUP[]                  = "%%Test: Before %%EndpageSetup\r\n";
const CHAR TEST_PAGETRAILER[]                   = "%%Test: After %%PageTrailer\r\n";
const CHAR TEST_TRAILER[]                       = "%%Test: After %%Trailer\r\n";
const CHAR TEST_PAGENUMBER[]                    = "%%Test: Replace driver's %%Page:\r\n";
const CHAR TEST_PAGEORDER[]                     = "%%Test: Replace driver's %%PageOrder:\r\n";
const CHAR TEST_ORIENTATION[]                   = "%%Test: Replace driver's %%Orientation:\r\n";
const CHAR TEST_BOUNDINGBOX[]                   = "%%Test: Replace driver's %%BoundingBox:\r\n";
const CHAR TEST_DOCNEEDEDRES[]                  = "%%Test: Append to driver's %%DocumentNeededResourc\r\n";
const CHAR TEST_DOCSUPPLIEDRES[]                = "%%Test: Append to driver's %%DocumentSuppliedResou\r\n";
const CHAR TEST_EOF[]                           = "%%Test: After %%EOF\r\n";
const CHAR TEST_ENDSTREAM[]                     = "%%Test: After the last byte of job stream\r\n";
const CHAR TEST_DOCUMENTPROCESSCOLORSATEND[]    = "%%Test: DocumentProcessColorsAtend\r\n";
const CHAR TEST_VMSAVE[]                        = "%%Test: %%VMSave\r\n";
const CHAR TEST_VMRESTORE[]                     = "%%Test: %%VMRestore\r\n";
const CHAR TEST_PLATECOLOR[]                    = "%%Test: %%PlateColor:\r\n";
const CHAR TEST_SHOWPAGE[]                      = "%%Test: %%SowPage:\r\n";
const CHAR TEST_PAGEBBOX[]                      = "%%Test: %%PageBox:\r\n";
const CHAR TEST_ENDPAGECOMMENTS[]               = "%%Test: %%EndPageComments:\r\n";
#else
PCSTR TEST_BEGINSTREAM                   = "\033%-12345X@PJL JOB\r\n";
PCSTR TEST_PSADOBE                       = NULL;
PCSTR TEST_PAGESATEND                    = NULL;
PCSTR TEST_PAGES                         = "%%Pages: (atend)\r\n";
PCSTR TEST_DOCUMENTPROCESSCOLORS         = NULL;
PCSTR TEST_COMMENTS                      = NULL;
PCSTR TEST_DEFAULTS                      = NULL;
PCSTR TEST_BEGINPROLOG                   = NULL;
PCSTR TEST_ENDPROLOG                     = NULL;
PCSTR TEST_BEGINSETUP                    = NULL;
PCSTR TEST_ENDSETUP                      = NULL;
PCSTR TEST_BEGINPAGESETUP                = NULL;
PCSTR TEST_ENDPAGESETUP                  = NULL;
PCSTR TEST_PAGETRAILER                   = NULL;
PCSTR TEST_TRAILER                       = NULL;
PCSTR TEST_PAGENUMBER                    = "%%%%Page: %d %d\r\n";
PCSTR TEST_PAGEORDER                     = "%%PageOrder: Special\r\n";
PCSTR TEST_ORIENTATION                   = "%%Orientation: Portrait\r\n";
PCSTR TEST_BOUNDINGBOX                   = "%%BoundingBox: 0 0 595 842\r\n";
PCSTR TEST_DOCNEEDEDRES                  = NULL;
PCSTR TEST_DOCSUPPLIEDRES                = NULL;
PCSTR TEST_EOF                           = NULL;
PCSTR TEST_ENDSTREAM                     = NULL;
PCSTR TEST_DOCUMENTPROCESSCOLORSATEND    = NULL;
PCSTR TEST_VMSAVE                        = NULL;
PCSTR TEST_VMRESTORE                     = NULL;
PCSTR TEST_PLATECOLOR                    = NULL;
PCSTR TEST_SHOWPAGE                      = NULL;
PCSTR TEST_PAGEBBOX                      = NULL;
PCSTR TEST_ENDPAGECOMMENTS               = NULL;
#endif


////////////////////////////////////////////////////////////////////////////////////
//    The PSCRIPT driver calls this OEM function at specific points during output
//    generation. This gives the OEM DLL an opportunity to insert code fragments
//    at specific injection points in the driver's code. It should use
//    DrvWriteSpoolBuf for generating any output it requires.

HRESULT PSCommand(PDEVOBJ pdevobj, DWORD dwIndex, PVOID pData, DWORD cbSize,
                  IPrintOemDriverPS* pOEMHelp, PDWORD pdwReturn)
{
    PCSTR   pProcedure  = NULL;
    DWORD   dwLen       = 0;
    DWORD   dwSize      = 0;
    HRESULT hResult     = S_OK;

    //log(L"Entering OEMCommand...");

    UNREFERENCED_PARAMETER(cbSize);
    UNREFERENCED_PARAMETER(pData);

    switch (dwIndex)
    {
        case PSINJECT_BEGINSTREAM:
            log(L"OEMCommand PSINJECT_BEGINSTREAM");
			g_nCurrentPage = 0;
            pProcedure = TEST_BEGINSTREAM;
            break;

        case PSINJECT_PSADOBE:
            log(L"OEMCommand PSINJECT_PSADOBE");
            pProcedure = TEST_PSADOBE;
            break;

        case PSINJECT_PAGESATEND:
            log(L"OEMCommand PSINJECT_PAGESATEND");
            pProcedure = TEST_PAGESATEND;
            break;

        case PSINJECT_PAGES:
            log(L"OEMCommand PSINJECT_PAGES");
            pProcedure = TEST_PAGES;
            break;

        case PSINJECT_DOCNEEDEDRES:
            log(L"OEMCommand PSINJECT_DOCNEEDEDRES");
            pProcedure = TEST_DOCNEEDEDRES;
            break;

        case PSINJECT_DOCSUPPLIEDRES:
            log(L"OEMCommand PSINJECT_DOCSUPPLIEDRES");
            pProcedure = TEST_DOCSUPPLIEDRES;
            break;

        case PSINJECT_PAGEORDER:
            log(L"OEMCommand PSINJECT_PAGEORDER");
            pProcedure = TEST_PAGEORDER;
            break;

        case PSINJECT_ORIENTATION:
            log(L"OEMCommand PSINJECT_ORIENTATION");
            pProcedure = TEST_ORIENTATION;
            break;

        case PSINJECT_BOUNDINGBOX:
            log(L"OEMCommand PSINJECT_BOUNDINGBOX");
            pProcedure = TEST_BOUNDINGBOX;
            break;

        case PSINJECT_DOCUMENTPROCESSCOLORS:
            log(L"OEMCommand PSINJECT_DOCUMENTPROCESSCOLORS");
            pProcedure = TEST_DOCUMENTPROCESSCOLORS;
            break;

        case PSINJECT_COMMENTS:
            log(L"OEMCommand PSINJECT_COMMENTS");
            pProcedure = TEST_COMMENTS;
            break;

        case PSINJECT_BEGINDEFAULTS:
            log(L"OEMCommand PSINJECT_BEGINDEFAULTS");
            pProcedure = TEST_DEFAULTS;
            break;

        case PSINJECT_ENDDEFAULTS:
            log(L"OEMCommand PSINJECT_BEGINDEFAULTS");
            pProcedure = TEST_DEFAULTS;
            break;

        case PSINJECT_BEGINPROLOG:
            log(L"OEMCommand PSINJECT_BEGINPROLOG");
            pProcedure = TEST_BEGINPROLOG;
            break;

        case PSINJECT_ENDPROLOG:
            log(L"OEMCommand PSINJECT_ENDPROLOG");
            pProcedure = TEST_ENDPROLOG;
            break;

        case PSINJECT_BEGINSETUP:
            log(L"OEMCommand PSINJECT_BEGINSETUP");
            pProcedure = TEST_BEGINSETUP;
            break;

        case PSINJECT_ENDSETUP:
            log(L"OEMCommand PSINJECT_ENDSETUP");
            pProcedure = TEST_ENDSETUP;
            break;

        case PSINJECT_BEGINPAGESETUP:
            log(L"OEMCommand PSINJECT_BEGINPAGESETUP");
            pProcedure = TEST_BEGINPAGESETUP;
            break;

        case PSINJECT_ENDPAGESETUP:
            log(L"OEMCommand PSINJECT_ENDPAGESETUP");
            pProcedure = TEST_ENDPAGESETUP;
            break;

        case PSINJECT_PAGETRAILER:
            log(L"OEMCommand PSINJECT_PAGETRAILER");
            pProcedure = TEST_PAGETRAILER;
            break;

        case PSINJECT_TRAILER:
            log(L"OEMCommand PSINJECT_TRAILER");
            pProcedure = TEST_TRAILER;
            break;

        case PSINJECT_PAGENUMBER:
            log(L"OEMCommand PSINJECT_PAGENUMBER");
#if Enable_Test
			pProcedure = TEST_PAGENUMBER;
#else
			g_nCurrentPage++;
			sprintf_s(g_cBuffer, MAX_PATH, TEST_PAGENUMBER, g_nCurrentPage, g_nCurrentPage);
			pProcedure = g_cBuffer;
#endif
            break;

        case PSINJECT_EOF:
            log(L"OEMCommand PSINJECT_EOF");
            pProcedure = TEST_EOF;
            break;

        case PSINJECT_ENDSTREAM:
            log(L"OEMCommand PSINJECT_ENDSTREAM");
			g_bReleaseFile = TRUE;
            pProcedure = TEST_ENDSTREAM;
            break;

        case PSINJECT_DOCUMENTPROCESSCOLORSATEND:
            log(L"OEMCommand PSINJECT_DOCUMENTPROCESSCOLORSATEND");
            pProcedure = TEST_DOCUMENTPROCESSCOLORSATEND;
            break;

        case PSINJECT_VMSAVE:
            log(L"OEMCommand PSINJECT_VMSAVE");
            pProcedure = TEST_VMSAVE;
            break;

        case PSINJECT_VMRESTORE:
            log(L"OEMCommand PSINJECT_VMRESTORE");
            pProcedure = TEST_VMRESTORE;
            break;

        case PSINJECT_PLATECOLOR:
            log(L"OEMCommand PSINJECT_PLATECOLOR");
            pProcedure = TEST_PLATECOLOR;
            break;

        case PSINJECT_SHOWPAGE:
            log(L"OEMCommand PSINJECT_SHOWPAGE");
            pProcedure = TEST_SHOWPAGE;
            break;

        case PSINJECT_PAGEBBOX:
            log(L"OEMCommand PSINJECT_PAGEBBOX");
            pProcedure = TEST_PAGEBBOX;
            break;

        case PSINJECT_ENDPAGECOMMENTS:
            log(L"OEMCommand PSINJECT_ENDPAGECOMMENTS");
            pProcedure = TEST_ENDPAGECOMMENTS;
            break;

        default:
            log(L"!!Undefined PSCommand!");
            *pdwReturn = ERROR_NOT_SUPPORTED;
            return E_NOTIMPL;
    }

    // INVARIANT: should have injection string.

    if(NULL != pProcedure)
    {
        // Write PostScript to spool file.
        dwLen = (DWORD)strlen(pProcedure);
        if (dwLen > 0)
        {
            hResult = pOEMHelp->DrvWriteSpoolBuf(pdevobj, const_cast<PSTR>(pProcedure), dwLen, &dwSize);
        }

        // Set return values.
        if(SUCCEEDED(hResult) && (dwLen == dwSize))
        {
            *pdwReturn = ERROR_SUCCESS;
        }
        else
        {
            // Return a meaningful
            // error value.
            *pdwReturn = GetLastError();
            if(ERROR_SUCCESS == *pdwReturn)
            {
                *pdwReturn = ERROR_WRITE_FAULT;
            }

            // Make sure we return failure
            // if the write didn't succeded.
            if(SUCCEEDED(hResult))
            {
                hResult = HRESULT_FROM_WIN32(*pdwReturn);
            }
        }
    }

    return hResult;
}



