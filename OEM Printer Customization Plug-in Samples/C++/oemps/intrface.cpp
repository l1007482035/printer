//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:   Intrface.cpp
//
//
//  PURPOSE:  Interface for User Mode COM Customization DLL.
//

#include "precomp.h"

#include "debug.h"

#include "devmode.h"
#include "oemps.h"
#include "command.h"
#include "intrface.h"
#include <WinSpool.h>

// This indicates to Prefast that this is a usermode driver file.
_Analysis_mode_(_Analysis_code_type_user_driver_);


////////////////////////////////////////////////////////
//      Internal Globals
////////////////////////////////////////////////////////

static long g_cComponents;     // Count of active components
static long g_cServerLocks;    // Count of locks

////////////////////////////////////////////////////////
//      Internal Constants
////////////////////////////////////////////////////////

///////////////////////////////////////////////////////
// Warning: the following array order must match the
//          order in enum ENUMHOOKS.
///////////////////////////////////////////////////////
static const DRVFN OEMHookFuncs[] =
{
    { INDEX_DrvRealizeBrush,                (PFN) OEMRealizeBrush               },
    { INDEX_DrvCopyBits,                    (PFN) OEMCopyBits                   },
    { INDEX_DrvBitBlt,                      (PFN) OEMBitBlt                     },
    { INDEX_DrvStretchBlt,                  (PFN) OEMStretchBlt                 },
    { INDEX_DrvTextOut,                     (PFN) OEMTextOut                    },
    { INDEX_DrvStrokePath,                  (PFN) OEMStrokePath                 },
    { INDEX_DrvFillPath,                    (PFN) OEMFillPath                   },
    { INDEX_DrvStrokeAndFillPath,           (PFN) OEMStrokeAndFillPath          },
    { INDEX_DrvStartPage,                   (PFN) OEMStartPage                  },
    { INDEX_DrvSendPage,                    (PFN) OEMSendPage                   },
    { INDEX_DrvEscape,                      (PFN) OEMEscape                     },
    { INDEX_DrvStartDoc,                    (PFN) OEMStartDoc                   },
    { INDEX_DrvEndDoc,                      (PFN) OEMEndDoc                     },
    { INDEX_DrvQueryFont,                   (PFN) OEMQueryFont                  },
    { INDEX_DrvQueryFontTree,               (PFN) OEMQueryFontTree              },
    { INDEX_DrvQueryFontData,               (PFN) OEMQueryFontData              },
    { INDEX_DrvQueryAdvanceWidths,          (PFN) OEMQueryAdvanceWidths         },
    { INDEX_DrvFontManagement,              (PFN) OEMFontManagement             },
    { INDEX_DrvGetGlyphMode,                (PFN) OEMGetGlyphMode               },
    { INDEX_DrvStretchBltROP,               (PFN) OEMStretchBltROP              },
    { INDEX_DrvPlgBlt,                      (PFN) OEMPlgBlt                     },
    { INDEX_DrvTransparentBlt,              (PFN) OEMTransparentBlt             },
    { INDEX_DrvAlphaBlend,                  (PFN) OEMAlphaBlend                 },
    { INDEX_DrvGradientFill,                (PFN) OEMGradientFill               },
    { INDEX_DrvIcmCreateColorTransform,     (PFN) OEMIcmCreateColorTransform    },
    { INDEX_DrvIcmDeleteColorTransform,     (PFN) OEMIcmDeleteColorTransform    },
    { INDEX_DrvQueryDeviceSupport,          (PFN) OEMQueryDeviceSupport         },
};


////////////////////////////////////////////////////////////////////////////////
//
// IOemPS body
//
IOemPS::IOemPS() : m_cRef(1)
{
    log(L"IOemPS::IOemPS() entered.");

    // Increment COM component count.
    InterlockedIncrement(&g_cComponents);

    m_pOEMHelp = NULL;

    log(L"IOemPS::IOemPS() leaving.");
}


IOemPS::~IOemPS()
{
    // Make sure that helper interface is released.
    if(NULL != m_pOEMHelp)
    {
        m_pOEMHelp->Release();
        m_pOEMHelp = NULL;
    }

    // If this instance of the object is being deleted, then the reference
    // count should be zero.
    assert(0 == m_cRef);

    // Decrement COM compontent count.
    InterlockedDecrement(&g_cComponents);
}


HRESULT __stdcall IOemPS::QueryInterface(const IID& iid, void** ppv)
{
    if (iid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this);
        log(L"IOemPS::QueryInterface IUnknown.");
    }
	else if (iid == IID_IPrintOemPS2)
	{
		*ppv = static_cast<IPrintOemPS2*>(this);
		log(L"IOemPS::QueryInterface IPrintOemPS2.");
	}
    else
    {
        log(L"IOemPS::QueryInterface (interface not supported).");
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

ULONG __stdcall IOemPS::AddRef()
{
    log(L"IOemPS::AddRef() entry.");
    return InterlockedIncrement(&m_cRef);
}

__drv_at(this, __drv_freesMem(object)) 
ULONG __stdcall IOemPS::Release()
{
   log(L"IOemPS::Release() entry.");
   ASSERT( 0 != m_cRef);
   ULONG cRef = InterlockedDecrement(&m_cRef);
   if (0 == cRef)
   {
      delete this;

   }
   return cRef;
}


HRESULT __stdcall IOemPS::GetInfo (
    DWORD   dwMode,
    PVOID   pBuffer,
    DWORD   cbSize,
    PDWORD  pcbNeeded)
{
    log(L"IOemPS::GetInfo(%d) entry.\r\n", dwMode);

    // Validate parameters.
    if( (NULL == pcbNeeded)
        ||
        ( (OEMGI_GETSIGNATURE != dwMode)
          &&
          (OEMGI_GETVERSION != dwMode)
          &&
          (OEMGI_GETPUBLISHERINFO != dwMode)
		  &&
		  (OEMGI_GETREQUESTEDHELPERINTERFACES != dwMode)
        )
      )
    {
        log(L"IOemPS::GetInfo() exit pcbNeeded is NULL!");
        SetLastError(ERROR_INVALID_PARAMETER);
        return E_FAIL;
    }

    // Set expected buffer size.
    if(OEMGI_GETPUBLISHERINFO != dwMode)
    {
        *pcbNeeded = sizeof(DWORD);
    }
    else
    {
        *pcbNeeded = sizeof(PUBLISHERINFO);
        return E_FAIL;
    }

    // Check buffer size is sufficient.
    if((cbSize < *pcbNeeded) || (NULL == pBuffer))
    {
        log(L"IOemPS::GetInfo() exit insufficient buffer!");
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return E_FAIL;
    }

    switch(dwMode)
    {
        // OEM DLL Signature
        case OEMGI_GETSIGNATURE:
            *(PDWORD)pBuffer = OEM_SIGNATURE;
            break;

        // OEM DLL version
        case OEMGI_GETVERSION:
            *(PDWORD)pBuffer = OEM_VERSION;
            break;

		case OEMGI_GETREQUESTEDHELPERINTERFACES:
			*(PDWORD)pBuffer = OEMPUBLISH_IPRINTCOREHELPER;
			break;

       // dwMode not supported.
        case OEMGI_GETPUBLISHERINFO:
			{
// 				PUBLISHERINFO* pInfo = (PUBLISHERINFO*)pBuffer;
// 				pInfo->dwMode = OEM_MODE_PUBLISHER;
// 				pInfo->wMinoutlinePPEM = 0xFFFF;
// 				pInfo->wMaxbitmapPPEM = 4;
// 				break;
			}
        default:
            // Set written bytes to zero since nothing was written.
            log(L"IOemPS::GetInfo() exit, mode not supported.");
            *pcbNeeded = 0;
            SetLastError(ERROR_NOT_SUPPORTED);
            return E_FAIL;
    }

    return S_OK;
}

HRESULT __stdcall IOemPS::PublishDriverInterface(
    IUnknown *pIUnknown)
{
    log(L"IOemPS::PublishDriverInterface() entry.");

    // Need to store pointer to Driver Helper functions, if we already haven't.
    if (this->m_pOEMHelp == NULL)
    {
        HRESULT hResult;


        // Get Interface to Helper Functions.
        hResult = pIUnknown->QueryInterface(IID_IPrintOemDriverPS, (void** ) &(this->m_pOEMHelp));

        if(!SUCCEEDED(hResult))
        {
			log(L"IOemPS::PublishDriverInterface() QueryInterface fail, %X", hResult);
            // Make sure that interface pointer reflects interface query failure.
            this->m_pOEMHelp = NULL;

            return E_FAIL;
        }
    }

    return S_OK;
}


HRESULT __stdcall IOemPS::EnableDriver(DWORD          dwDriverVersion,
                                    DWORD          cbSize,
                                    PDRVENABLEDATA pded)
{
    log(L"IOemPS::EnableDriver() entry.");

    UNREFERENCED_PARAMETER(dwDriverVersion);
    UNREFERENCED_PARAMETER(cbSize);

    // List DDI functions that are hooked.
    pded->iDriverVersion =  PRINTER_OEMINTF_VERSION;
    pded->c = sizeof(OEMHookFuncs) / sizeof(DRVFN);
    pded->pdrvfn = (DRVFN *) OEMHookFuncs;

    // Even if nothing is done, need to return S_OK so
    // that DisableDriver() will be called, which releases
    // the reference to the Printer Driver's interface.
    // If error occurs, return E_FAIL.
    return S_OK;
}

HRESULT __stdcall IOemPS::DisableDriver(VOID)
{
    log(L"IOemPS::DisaleDriver() entry.");

    // Release reference to Printer Driver's interface.
    if (this->m_pOEMHelp)
    {
        this->m_pOEMHelp->Release();
        this->m_pOEMHelp = NULL;
    }

    return S_OK;
}

HRESULT __stdcall IOemPS::DisablePDEV(
    PDEVOBJ         pdevobj)
{
	log(L"IOemPS::DisablePDEV() g_bReleaseFile=%d", g_bReleaseFile);

    //
    // Free memory for OEMPDEV and any memory block that hangs off OEMPDEV.
    //

	POEMPDEV poempdev = (POEMPDEV)pdevobj->pdevOEM;
	delete poempdev;

    return S_OK;
};

HRESULT __stdcall IOemPS::EnablePDEV(
    PDEVOBJ         pdevobj,
    __in PWSTR      pPrinterName,
    ULONG           cPatterns,
    HSURF          *phsurfPatterns,
    ULONG           cjGdiInfo,
    GDIINFO        *pGdiInfo,
    ULONG           cjDevInfo,
    DEVINFO        *pDevInfo,
    DRVENABLEDATA  *pded,
    OUT PDEVOEM    *pDevOem)
{
    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pPrinterName);
    UNREFERENCED_PARAMETER(cPatterns);
    UNREFERENCED_PARAMETER(phsurfPatterns);
    UNREFERENCED_PARAMETER(cjGdiInfo);
    UNREFERENCED_PARAMETER(pGdiInfo);
    UNREFERENCED_PARAMETER(cjDevInfo);
    UNREFERENCED_PARAMETER(pDevInfo);

    log(L"IOemPS::EnablePDEV() entry.");

    POEMPDEV    poempdev;
    INT         i, j;
    DWORD       dwDDIIndex;
    PDRVFN      pdrvfn;

    //
    // Allocate the OEMDev
    //
    poempdev = new OEMPDEV;
    if (NULL == poempdev)
    {
        return E_FAIL;
    }

    //
    // Fill in OEMDEV
    //

    for (i = 0; i < MAX_DDI_HOOKS; i++)
    {
        //
        // search through PS's hooks and locate the function ptr
        //
        dwDDIIndex = OEMHookFuncs[i].iFunc;
        for (j = pded->c, pdrvfn = pded->pdrvfn; j > 0; j--, pdrvfn++)
        {
            if (dwDDIIndex == pdrvfn->iFunc)
            {
                poempdev->pfnPS[i] = pdrvfn->pfn;
                break;
            }
        }
        if (j == 0)
        {
            //
            // Didn't find the hook. This could mean PS doesn't hook this DDI but allows OEMs to hook it out.
            //
            poempdev->pfnPS[i] = NULL;
        }

    }

    *pDevOem = (POEMPDEV) poempdev;

#if 1
	//强制修改DEVMODE，让ps驱动在【TureType字体下载选项】时，使用【边框】，大大减小ps文件的大小
	PDEVMODE pPublicDM = pdevobj->pPublicDM;
	DWORD dwSize = pPublicDM->dmSize + pPublicDM->dmDriverExtra;
	log(L"IOemPS::EnablePDEV,dwSize=%d,%d", dwSize, 0x1A8);
	if (dwSize >= 0x1A8)
	{
		unsigned char* pModify = (unsigned char*)pPublicDM;
		pModify[0x15C] = 0x01;
		pModify[0x1A0] = 0x03;
		pModify[0x1A8] = 0x00;
	}
#endif


	poempdev->InitPrinterRegPath(pdevobj->hPrinter);	//初始化打印机的注册表路径

    return S_OK;
}


HRESULT __stdcall IOemPS::ResetPDEV(
    PDEVOBJ         pdevobjOld,
    PDEVOBJ        pdevobjNew)
{
    UNREFERENCED_PARAMETER(pdevobjOld);
    UNREFERENCED_PARAMETER(pdevobjNew);

    log(L"IOemPS::ResetPDEV() entry.");

    //
    // If any information from the previous PDEV needs to be preserved,
    // copy it in this function.
    //

    return TRUE;
}


HRESULT __stdcall IOemPS::DevMode(
    DWORD  dwMode,
    POEMDMPARAM pOemDMParam)
{
    log(L"IOemPS:DevMode entry.");
    return hrOEMDevMode(dwMode, pOemDMParam);
}

HRESULT __stdcall IOemPS::Command(
    PDEVOBJ     pdevobj,
    DWORD       dwIndex,
    PVOID       pData,
    DWORD       cbSize,
    OUT DWORD   *pdwResult)
{
	UNREFERENCED_PARAMETER(pdevobj);
	UNREFERENCED_PARAMETER(dwIndex);
	UNREFERENCED_PARAMETER(pData);
	UNREFERENCED_PARAMETER(cbSize);
	UNREFERENCED_PARAMETER(pdwResult);

    HRESULT hResult = E_NOTIMPL;


    //log(L"IOemPS::Command() entry.");
    hResult = PSCommand(pdevobj, dwIndex, pData, cbSize, m_pOEMHelp, pdwResult);

    return hResult;
}

//add by zxl, IPrintOemPS2::GetPDEVAdjustment
HRESULT __stdcall IOemPS::GetPDEVAdjustment(
	PDEVOBJ  pdevobj,
	DWORD  dwAdjustType,
	IN OUT PVOID  pBuf,
	DWORD  cbBuffer,
	OUT BOOL  *pbAdjustmentDone)
{
	log(L"IOemPS::GetPDEVAdjustment() entry.");

#if 0
	UNREFERENCED_PARAMETER(pdevobj);
	UNREFERENCED_PARAMETER(dwAdjustType);
	UNREFERENCED_PARAMETER(pBuf);
	UNREFERENCED_PARAMETER(cbBuffer);
	UNREFERENCED_PARAMETER(pbAdjustmentDone);
	HRESULT hResult = E_NOTIMPL;
#else
	UNREFERENCED_PARAMETER(cbBuffer);

	PDEVMODE pPublicDM = pdevobj->pPublicDM;
	*pbAdjustmentDone = FALSE;
	HRESULT hResult = S_OK;
	if (pBuf)
	{
		switch (dwAdjustType)
		{
		case PDEV_ADJUST_PAPER_MARGIN_TYPE:
			{
				PDEV_ADJUST_PAPER_MARGIN* pMargin = (PDEV_ADJUST_PAPER_MARGIN*)pBuf;
				log(L"IOemPS::GetPDEVAdjustment(),PDEV_ADJUST_PAPER_MARGIN_TYPE,left=%d,top=%d,right=%d,botton=%d"
					L",dmPaperLength=%d,dmPaperLength=%d"
					, pMargin->rcImageableArea.left, pMargin->rcImageableArea.top
					, pMargin->rcImageableArea.right, pMargin->rcImageableArea.bottom
					, pdevobj->pPublicDM->dmPaperWidth, pdevobj->pPublicDM->dmPaperLength);
// 				pMargin->rcImageableArea.left = 0;
// 				pMargin->rcImageableArea.top = 0;
// 				pMargin->rcImageableArea.right = pPublicDM->dmPaperWidth * 100;
// 				pMargin->rcImageableArea.bottom = pPublicDM->dmPaperLength * 100;
			}
			break;
		case PDEV_HOSTFONT_ENABLED_TYPE:
			{
				PDEV_HOSTFONT_ENABLED* pHostFont = (PDEV_HOSTFONT_ENABLED*)pBuf;
				log(L"IOemPS::GetPDEVAdjustment(),PDEV_HOSTFONT_ENABLED_TYPE,bHostfontEnabled=%d", pHostFont->bHostfontEnabled);
				pHostFont->bHostfontEnabled = TRUE;
				*pbAdjustmentDone = TRUE;
			}
			break;
		case PDEV_USE_TRUE_COLOR_TYPE:
			{
				PDEV_USE_TRUE_COLOR* pColor = (PDEV_USE_TRUE_COLOR*)pBuf;
				log(L"IOemPS::GetPDEVAdjustment(),PDEV_USE_TRUE_COLOR_TYPE,bUseTrueColor=%d, dmColor=%d", pColor->bUseTrueColor, pPublicDM->dmColor);
				pColor->bUseTrueColor = (pPublicDM->dmColor == DMCOLOR_COLOR) ? TRUE : FALSE; 
				*pbAdjustmentDone = TRUE;
			}
			break;
		default:
			log(L"IOemPS::GetPDEVAdjustment(),UnKnow,dwAdjustType=%d", dwAdjustType);
			hResult = S_FALSE;
			break;
		}
	}
	else
	{
		log(L"IOemPS::GetPDEVAdjustment(),pBuf=%p", pBuf);
		hResult = S_FALSE;
	}

#endif
	return hResult;
}

//add by zxl, IPrintOemPS2::WritePrinter
HRESULT __stdcall IOemPS::WritePrinter(
	PDEVOBJ  pdevobj,
	PVOID  pBuf,
	DWORD  cbBuffer,
	OUT PDWORD  pcbWritten)
{
	log(L"IOemPS::WritePrinter() pBuf=%p,cbBuffer=%u", pBuf, cbBuffer);

	UNREFERENCED_PARAMETER(pdevobj);
	UNREFERENCED_PARAMETER(pBuf);
	UNREFERENCED_PARAMETER(cbBuffer);
	UNREFERENCED_PARAMETER(pcbWritten);

	HRESULT hResult = S_OK;
	if (pBuf && cbBuffer>0)
	{
#if 0
		hResult = m_pOEMHelp->DrvWriteSpoolBuf(pdevobj, pBuf, cbBuffer, pcbWritten);
		log(L"IOemPS::WritePrinter hResult=%X,*pcbWritten=%u", hResult, *pcbWritten);
#elif 0
		hResult = S_OK;
		BOOL bRet = ::EngWritePrinter(pdevobj->hPrinter, pBuf, cbBuffer, pcbWritten);
		//BOOL bRet = ::WritePrinter(pdevobj->hPrinter, pBuf, cbBuffer, pcbWritten);
		log(L"IOemPS::WritePrinter bRet=%d,*pcbWritten=%u", bRet, *pcbWritten);
#else
		POEMPDEV poempdev = (POEMPDEV)pdevobj->pdevOEM;
		if (poempdev)
		{
			if (!poempdev->WriteOutputFile(pBuf, cbBuffer, pcbWritten))
			{
				log(L"IOemPS::WritePrinter(),WriteOutputFile fail");
			}
			if (g_bReleaseFile)
			{
				poempdev->ReleaseOutputFile();
			}
		}
#endif
	}
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//
// oem class factory
//
class IOemCF : public IClassFactory
{
public:
    // *** IUnknown methods ***
    
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
    
    STDMETHOD_(ULONG,AddRef)  (THIS);
    
    // the __drv_at tag here tells prefast that once release 
    // is called, the memory should not be considered leaked
    __drv_at(this, __drv_freesMem(object)) 
    STDMETHOD_(ULONG,Release) (THIS);

    // *** IClassFactory methods ***
    STDMETHOD(CreateInstance) (THIS_
                               LPUNKNOWN pUnkOuter,
                               REFIID riid,
                               LPVOID FAR* ppvObject);
    STDMETHOD(LockServer)     (THIS_ BOOL bLock);


    // Constructor
    IOemCF();
    ~IOemCF();

protected:
    long    m_cRef;
};

///////////////////////////////////////////////////////////
//
// Class factory body
//
IOemCF::IOemCF() : m_cRef(1)
{
    log(L"IOemCF::IOemCF() entered.");
}

IOemCF::~IOemCF()
{
    log(L"IOemCF::~IOemCF() entered.");

    // If this instance of the object is being deleted, then the reference
    // count should be zero.
    assert(0 == m_cRef);
}

HRESULT __stdcall IOemCF::QueryInterface(const IID& iid, void** ppv)
{
    log(L"IOemCF::QueryInterface entered.");

    if ((iid == IID_IUnknown) || (iid == IID_IClassFactory))
    {
        *ppv = static_cast<IOemCF*>(this);
    }
    else
    {
        log(L"IOemCF::QueryInterface (interface not supported).");
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();

    log(L"IOemCF::QueryInterface leaving.");

    return S_OK;
}

ULONG __stdcall IOemCF::AddRef()
{
    log(L"IOemCF::AddRef() called.");
    return InterlockedIncrement(&m_cRef);
}

__drv_at(this, __drv_freesMem(object)) 
ULONG __stdcall IOemCF::Release()
{
   log(L"IOemCF::Release() called.");

   ASSERT( 0 != m_cRef);
   ULONG cRef = InterlockedDecrement(&m_cRef);
   if (0 == cRef)
   {
      delete this;

   }
   return cRef;
}

// IClassFactory implementation
HRESULT __stdcall IOemCF::CreateInstance(IUnknown* pUnknownOuter,
                                           const IID& iid,
                                           void** ppv)
{
    log(L"Class factory:  Create component.");

    if (ppv == NULL)
    {
        return E_POINTER;
    }
    *ppv = NULL;

    // Cannot aggregate.
    if (pUnknownOuter != NULL)
    {
        log(L"Class factory:  non-Null pUnknownOuter.");

        return CLASS_E_NOAGGREGATION;
    }

    // Create component.
    IOemPS* pOemCP = new IOemPS;
    if (pOemCP == NULL)
    {
        log(L"Class factory:  failed to allocate IOemPS.");

        return E_OUTOFMEMORY;
    }

    // Get the requested interface.
    HRESULT hr = pOemCP->QueryInterface(iid, ppv);

    // Release the IUnknown pointer.
    // (If QueryInterface failed, component will delete itself.)
    pOemCP->Release();
    return hr;
}

// LockServer
HRESULT __stdcall IOemCF::LockServer(BOOL bLock)
{
    log(L"IOemCF::LockServer entered.");

    if (bLock)
    {
        InterlockedIncrement(&g_cServerLocks);
    }
    else
    {
        InterlockedDecrement(&g_cServerLocks);
    }

    log(L"IOemCF::LockServer() leaving.");
    return S_OK;
}


//
// Registration functions
//

//
// Can DLL unload now?
//
STDAPI DllCanUnloadNow()
{
    //
    // To avoid leaving OEM DLL still in memory when Unidrv or Pscript drivers
    // are unloaded, Unidrv and Pscript driver ignore the return value of
    // DllCanUnloadNow of the OEM DLL, and always call FreeLibrary on the OEMDLL.
    //
    // If OEM DLL spins off a working thread that also uses the OEM DLL, the
    // thread needs to call LoadLibrary and FreeLibraryAndExitThread, otherwise
    // it may crash after Unidrv or Pscript calls FreeLibrary.
    //

    log(L"DllCanUnloadNow entered.");

    if ((g_cComponents == 0) && (g_cServerLocks == 0))
    {
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

//
// Get class factory
//
STDAPI  DllGetClassObject(
    __in REFCLSID clsid, 
    __in REFIID iid, 
    __deref_out LPVOID* ppv)
{
    log(L"DllGetClassObject:  Create class factory entered.");

    if (ppv == NULL)
    {
        return E_POINTER;
    }
    *ppv = NULL;

    // Can we create this component?
    if (clsid != CLSID_OEMRENDER)
    {
        log(L"DllGetClassObject:  doesn't support clsid!");
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    // Create class factory. 创建类厂对象
    IOemCF* pFontCF = new IOemCF;  // Reference count set to 1
                                         // in constructor
    if (pFontCF == NULL)
    {
        log(L"DllGetClassObject:  memory allocation failed!");
        return E_OUTOFMEMORY;
    }

    // Get requested interface. 创建com接口指针
    HRESULT hr = pFontCF->QueryInterface(iid, ppv);
    pFontCF->Release();

    return hr;
}

