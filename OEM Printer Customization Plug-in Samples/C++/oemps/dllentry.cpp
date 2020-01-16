//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:   dllentry.cpp
//
//
//  PURPOSE:  Source module for DLL entry function(s).
//

#include "precomp.h"
#include "oemps.h"
#include "debug.h"

// This indicates to Prefast that this is a usermode driver file.
_Analysis_mode_(_Analysis_code_type_user_driver_);

///////////////////////////////////////////////////////////
//
// DLL entry point
//
BOOL WINAPI DllMain(HINSTANCE hInst, WORD wReason, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(lpReserved);
    switch(wReason)
    {
        case DLL_PROCESS_ATTACH:
            log(L"Process attach.");
            break;

        case DLL_THREAD_ATTACH:
            log(L"Thread attach.");
            break;

        case DLL_PROCESS_DETACH:
            log(L"Process detach.");
            break;

        case DLL_THREAD_DETACH:
            log(L"Thread detach.");
            break;
    }

    return TRUE;
}
