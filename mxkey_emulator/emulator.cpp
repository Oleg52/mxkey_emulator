#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <objbase.h>
#include <time.h>
#include "logging.h"
#include "winmm_hook.h"
#include "hook_helper.h"
#include "emulator_helper.h"
#include "save_helper.h"
#include "buffers.h"

void HookWinscardFunctions();

typedef unsigned long* SCARDCONTEXT;
typedef unsigned long* SCARDHANDLE;
typedef SCARDHANDLE *PSCARDHANDLE, *LPSCARDHANDLE;
typedef SCARDCONTEXT *PSCARDCONTEXT, *LPSCARDCONTEXT;
typedef const BYTE* LPCBYTE;
typedef const GUID *LPCGUID;

typedef struct _SCARD_IO_REQUEST{
    DWORD dwProtocol;
    DWORD cbPciLength;
} SCARD_IO_REQUEST, *PSCARD_IO_REQUEST, *LPSCARD_IO_REQUEST;

typedef const SCARD_IO_REQUEST *LPCSCARD_IO_REQUEST;

typedef struct {
    LPCSTR      szReader;
    LPVOID      pvUserData;
    DWORD       dwCurrentState;
    DWORD       dwEventState;
    DWORD       cbAtr;
    BYTE        rgbAtr[36];
} SCARD_READERSTATEA, *PSCARD_READERSTATEA, *LPSCARD_READERSTATEA;

typedef struct {
    LPCWSTR     szReader;
    LPVOID      pvUserData;
    DWORD       dwCurrentState;
    DWORD       dwEventState;
    DWORD       cbAtr;
    BYTE        rgbAtr[36];
} SCARD_READERSTATEW, *PSCARD_READERSTATEW, *LPSCARD_READERSTATEW;

typedef struct _TIME_FIELDS {
  SHORT Year;
  SHORT Month;
  SHORT Day;
  SHORT Hour;
  SHORT Minute;
  SHORT Second;
  SHORT Milliseconds;
  SHORT Weekday;
} TIME_FIELDS;

typedef struct _VerInfo {
  ULONG  dwOSVersionInfoSize;
  ULONG  dwMajorVersion;
  ULONG  dwMinorVersion;
  ULONG  dwBuildNumber;
  ULONG  dwPlatformId;
  WCHAR  szCSDVersion[128];
  USHORT wServicePackMajor;
  USHORT wServicePackMinor;
  USHORT wSuiteMask;
  UCHAR  wProductType;
  UCHAR  wReserved;
} *PRTL_OSVERSIONINFOW;

#define SCARD_S_SUCCESS             0x00000000
#define SCARD_E_COMM_DATA_LOST      0x8010002F
#define SCARD_E_INSUFFICIENT_BUFFER 0x80100008
#define SCARD_STATE_PRESENT         0x00000020
#define SCARD_AUTOALLOCATE          ((DWORD)(-1))
#define SCARD_PROTOCOL_T0           0x00000001

static DWORD g_ContextCounter = 0xEE000000;
static DWORD g_cardHandle = 0xEA010000;

static HMODULE g_hWinscard = NULL;
static BYTE* g_MobileExDllBase = NULL;
static BYTE* g_MobileExDllEntryPoint = NULL;
static BYTE g_MobileExDllEntryPointBytes[5];
static FuncEntrySignature g_MobileExDllMainEntrySigs[] = {
    {{0xE8, 0xFF, 0xFF, 0xFF, 0xFF}, 5}
};

static DWORD g_AllocCount = 0;
static BOOL g_HasHookedMobileExDll = FALSE;
static BOOL g_HasPatchedVmCheck = FALSE;
static BOOL g_HasPatchedNokiaUfsJaf = FALSE;
static BOOL g_HasPatchedNokiaSaveRpl = FALSE;

static BOOL g_CollectWinscardCandidates = FALSE;
static BYTE* g_WinscardCandidates[50];
static DWORD g_WinscardCandidatesCount;

static BOOL g_IsOnNewerWindows = FALSE;

BOOL StringIContains(LPCSTR str, LPCSTR search)
{
    if(!str || !search) return NULL;
    size_t searchLen = strlen(search);
    for(; *str; str++)
        if(strnicmp(str, search, searchLen) == 0) return TRUE;
    return FALSE;
}

BOOL WStringIContains(LPCWSTR str, LPCWSTR search)
{
    if(!str || !search) return NULL;
    size_t searchLen = wcslen(search);
    for(; *str; str++)
        if(wcsnicmp(str, search, searchLen) == 0) return TRUE;
    return FALSE;
}

LONG WriteMultiStringA(const char* data, DWORD dataLen, LPSTR mszOut, LPDWORD pcchOut)
{
    if (!pcchOut)
    {
        return SCARD_S_SUCCESS;
    }

    if (!mszOut)
    {
        *pcchOut = dataLen;
        return SCARD_S_SUCCESS;
    }

    if (*pcchOut == SCARD_AUTOALLOCATE)
    {
        LPSTR buf = (LPSTR)HeapAlloc(GetProcessHeap(), 0, dataLen);
        memcpy(buf, data, dataLen);
        *(LPSTR*)mszOut = buf;
        *pcchOut = dataLen;
        return SCARD_S_SUCCESS;
    }

    if (*pcchOut < dataLen)
    {
        *pcchOut = dataLen;
        return SCARD_E_INSUFFICIENT_BUFFER;
    }

    memcpy(mszOut, data, dataLen);
    *pcchOut = dataLen;
    return SCARD_S_SUCCESS;
}

typedef LONG (__stdcall *SCardEstablishContext_t)(DWORD, LPCVOID, LPCVOID, LPSCARDCONTEXT);
SCardEstablishContext_t SCardEstablishContextOrigFunc = NULL;

LONG __stdcall SCardEstablishContext_Hook(DWORD dwScope, LPCVOID pvReserved1, LPCVOID pvReserved2, LPSCARDCONTEXT phContext)
{
    LogToFile("SCardEstablishContext:\ndwScope: 0x%x", dwScope);
    g_ContextCounter++;
    *phContext = (SCARDCONTEXT)(DWORD*)g_ContextCounter;
    return SCARD_S_SUCCESS;
}

typedef LONG (__stdcall *SCardReleaseContext_t)(SCARDCONTEXT hContext);
SCardReleaseContext_t SCardReleaseContextOrigFunc = NULL;
 
LONG __stdcall SCardReleaseContext_Hook(SCARDCONTEXT hContext)
{
    LogToFile("SCardReleaseContext");
    return SCARD_S_SUCCESS;
}

typedef LONG (__stdcall *SCardListReadersA_t)(SCARDCONTEXT hContext, LPCSTR mszGroups, LPSTR mszReaders, LPDWORD pcchReaders);
SCardListReadersA_t SCardListReadersAOrigFunc = NULL;
 
LONG __stdcall SCardListReadersA_Hook(SCARDCONTEXT hContext, LPCSTR mszGroups, LPSTR mszReaders, LPDWORD pcchReaders)
{
    LogToFile("SCardListReadersA: pcchReaders: %d", *pcchReaders);

    const char readerName[] = "Generic EMV Smartcard Reader 0\0";
    const DWORD readerNameLen = 32;

    LONG result = WriteMultiStringA(readerName, readerNameLen, mszReaders, pcchReaders);
    if (mszReaders && result == SCARD_S_SUCCESS)
    {
        LogBufferToFile("SCardListReadersA: mszReaders", (void*)mszReaders, *pcchReaders);
    }

    return result;
}

typedef LONG (__stdcall *SCardConnectA_t)(SCARDCONTEXT hContext, LPCSTR szReader, DWORD dwShareMode, DWORD dwPreferredProtocols, LPSCARDHANDLE phCard, LPDWORD pdwActiveProtocol);
SCardConnectA_t SCardConnectAOrigFunc = NULL;
 
LONG __stdcall SCardConnectA_Hook(SCARDCONTEXT hContext, LPCSTR szReader, DWORD dwShareMode, DWORD dwPreferredProtocols, LPSCARDHANDLE phCard, LPDWORD pdwActiveProtocol)
{
    LogToFile("SCardConnectA: 0x%X", g_cardHandle);
    *phCard = (SCARDHANDLE)(DWORD*)g_cardHandle;
    *pdwActiveProtocol = SCARD_PROTOCOL_T0;
    return SCARD_S_SUCCESS;
}

typedef LONG (__stdcall *SCardDisconnect_t)(SCARDHANDLE hCard, DWORD dwDisposition);
SCardDisconnect_t SCardDisconnectOrigFunc = NULL;
 
LONG __stdcall SCardDisconnect_Hook(SCARDHANDLE hCard, DWORD dwDisposition)
{
    LogToFile("SCardDisconnect: 0x%p , dwDisposition: %x", hCard, dwDisposition);
    return SCARD_S_SUCCESS;
}

typedef LONG (__stdcall *SCardBeginTransaction_t)(SCARDHANDLE hCard);
SCardBeginTransaction_t SCardBeginTransactionOrigFunc = NULL;
 
LONG __stdcall SCardBeginTransaction_Hook(SCARDHANDLE hCard)
{
    LogToFile("SCardBeginTransaction");
    return SCARD_S_SUCCESS;
}

typedef LONG (__stdcall *SCardEndTransaction_t)(SCARDHANDLE hCard, DWORD dwDisposition);
SCardEndTransaction_t SCardEndTransactionOrigFunc = NULL;
 
LONG __stdcall SCardEndTransaction_Hook(SCARDHANDLE hCard, DWORD dwDisposition)
{
    LogToFile("SCardEndTransaction");
    return SCARD_S_SUCCESS;
}

typedef LONG (__stdcall *SCardTransmit_t)(SCARDHANDLE, LPCSCARD_IO_REQUEST, LPCBYTE, DWORD, LPSCARD_IO_REQUEST, LPBYTE, LPDWORD);
SCardTransmit_t SCardTransmitOrigFunc = NULL;

LONG __stdcall SCardTransmit_Hook(SCARDHANDLE hCard, LPCSCARD_IO_REQUEST pioSendPci, LPCBYTE pbSendBuffer, DWORD cbSendLength, LPSCARD_IO_REQUEST pioRecvPci,
    LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength)
{
    LogBufferToFile("SCardTransmit pbSendBuffer", (void*)pbSendBuffer, cbSendLength);

    BOOL result = HandleTransmitRequest(pbSendBuffer, cbSendLength, pbRecvBuffer, pcbRecvLength);
    if (result)
    {
        LogBufferToFile("SCardTransmit pbRecvBuffer", pbRecvBuffer, *pcbRecvLength);
        LogTransmitBuffersToFile(pbSendBuffer, cbSendLength, pbRecvBuffer, *pcbRecvLength);
        return SCARD_S_SUCCESS;
    }

    LogToFile("SCardTransmit: failed to match pbSendBuffer");
    return SCARD_E_COMM_DATA_LOST;
}

typedef LONG (__stdcall *SCardGetAttrib_t)(SCARDHANDLE hCard, DWORD dwAttrId, LPBYTE pbAttr, LPDWORD pcbAttrLen);
SCardGetAttrib_t SCardGetAttribOrigFunc = NULL;
 
LONG __stdcall SCardGetAttrib_Hook(SCARDHANDLE hCard, DWORD dwAttrId, LPBYTE pbAttr, LPDWORD pcbAttrLen)
{
    LogToFile("SCardGetAttrib: dwAttrId: 0x%x", dwAttrId);
    const AttributeEntry* result = GetAttributeValue(dwAttrId);
    if (result)
    {
        *pcbAttrLen = result->size;
        memcpy(pbAttr, result->data, *pcbAttrLen);
        LogBufferToFile("SCardGetAttrib pbAttr", pbAttr, *pcbAttrLen);
        return SCARD_S_SUCCESS;
    }

    LogToFile("SCardGetAttrib: dwAttrId not found");
    return SCARD_E_COMM_DATA_LOST;
}

typedef LONG (__stdcall *SCardIsValidContext_t)(SCARDCONTEXT hContext);
SCardIsValidContext_t SCardIsValidContextOrigFunc = NULL;
 
LONG __stdcall SCardIsValidContext_Hook(SCARDCONTEXT hContext)
{
    LogToFile("SCardIsValidContext: %p", hContext);
    return hContext ? SCARD_S_SUCCESS : ERROR_INVALID_HANDLE;
}

typedef LONG (__stdcall *SCardGetStatusChangeA_t)(SCARDCONTEXT hContext, DWORD dwTimeout, LPSCARD_READERSTATEA rgReaderStates, DWORD cReaders);
SCardGetStatusChangeA_t SCardGetStatusChangeAOrigFunc = NULL;
 
LONG __stdcall SCardGetStatusChangeA_Hook(SCARDCONTEXT hContext, DWORD dwTimeout, LPSCARD_READERSTATEA rgReaderStates, DWORD cReaders)
{
    LogToFile("SCardGetStatusChangeA: %s / %d / %d", rgReaderStates->szReader, rgReaderStates->dwCurrentState, rgReaderStates->dwEventState);
    rgReaderStates->dwEventState = SCARD_STATE_PRESENT;
    return SCARD_S_SUCCESS;
}

BOOL __stdcall GetSaveFileNameA_Hook(LPOPENFILENAMEA lpOpenFileName)
{
    // obfusucator probably corrupted smth on windows 7+
    // when selecting different folder, app crashes
    // open file dialog in new process and get result path
    return CustomGetSaveFileName(lpOpenFileName);
}

BOOL __stdcall GetOpenFileNameA_Hook(LPOPENFILENAMEA lpOpenFileName)
{
    return CustomGetOpenFileName(lpOpenFileName);
}

void PatchVmCheck()
{
    if (g_HasPatchedVmCheck) return;
    const BYTE patch[] = {0x33,0xC0};
    const DWORD offsets[] = {0xF59BE, 0x12E6AD, 0x12E77D, 0x1262B9, 0xF9BD2, 0xFF06A, 0x10C76A, 0x101A19}; // offsets for different versions

    for (unsigned i = 0; i < sizeof(offsets) / sizeof(offsets[0]); i++)
    {
        BYTE* address = g_MobileExDllBase + offsets[i];
        if (address[0] == 0x8B && address[1] == 0xC3 && address[2] == 0x5B)
        {
            memcpy(address, patch, sizeof(patch));
            g_HasPatchedVmCheck = TRUE;
            return;
        }
    }

    return;    
}

// return missing ufs/jaf init code for rev 2.7
void PatchNokiaUfsJaf()
{
    if (g_HasPatchedNokiaUfsJaf) return;

    BYTE* jmpToUfsRegionAddr = g_MobileExDllBase + 0x18BA65;
    BYTE* jmpToJafRegionAddr = g_MobileExDllBase + 0x18BAD5;

    if (jmpToUfsRegionAddr[0] == 0xBA && jmpToUfsRegionAddr[1] == 0x3 &&
        jmpToJafRegionAddr[0] == 0xBA && jmpToJafRegionAddr[1] == 0x19)
    {
        BYTE* ufsRegionAddr = g_MobileExDllBase + 0x5820AC;
        BYTE* jafRegionAddr = g_MobileExDllBase + 0x5823DC;

        DWORD oldProtect;
        VirtualProtect(ufsRegionAddr, sizeof(UFS_REGION), PAGE_EXECUTE_READWRITE, &oldProtect);
        VirtualProtect(jafRegionAddr, sizeof(JAF_REGION), PAGE_EXECUTE_READWRITE, &oldProtect);
        memcpy(ufsRegionAddr, UFS_REGION, sizeof(UFS_REGION));
        memcpy(jafRegionAddr, JAF_REGION, sizeof(JAF_REGION));

        BYTE jmpToUfsRegion[] = {0xE9,0x42,0x66,0x3F,0x00};
        BYTE jmpToJafRegion[] = {0xE9,0x02,0x69,0x3F,0x00};

        memcpy(jmpToUfsRegionAddr, jmpToUfsRegion, sizeof(jmpToUfsRegion));
        memcpy(jmpToJafRegionAddr, jmpToJafRegion, sizeof(jmpToJafRegion));

        g_HasPatchedNokiaUfsJaf = TRUE;
    }

    return;    
}

void PatchModulesSignatureCheck()
{
    const BYTE patch[] = {0xB0, 0x01, 0xC3};
    const DWORD offsets[] = {0x1AD020, 0x19F9A0, 0x1852C0}; // for v3.5
    BYTE* exeBase = (BYTE*)GetModuleHandleA(NULL);

    for (unsigned i = 0; i < sizeof(offsets) / sizeof(offsets[0]); i++)
    {
        BYTE* address = exeBase + offsets[i];
        if (address[0] == 0x53 && address[1] == 0x56 && address[2] == 0x83)
        {
            memcpy(address, patch, sizeof(patch));
            return;
        }
    }
}

void PatchNokiaSaveRpl()
{
    if (g_HasPatchedNokiaSaveRpl) return;
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    BYTE* addr = (BYTE*)si.lpMinimumApplicationAddress;
    BYTE* maxAddr = (BYTE*)si.lpMaximumApplicationAddress;

    MEMORY_BASIC_INFORMATION memInfo;

    while (addr < maxAddr)
    {
        if (VirtualQuery(addr, &memInfo, sizeof(memInfo)) == 0)
        {
            break;
        }

        if (memInfo.Type == MEM_PRIVATE && memInfo.Protect == PAGE_READWRITE) // find nokia module
        {
            BOOL isNokiaModule = FALSE;
            if (((IMAGE_DOS_HEADER*)addr)->e_magic == IMAGE_DOS_SIGNATURE &&
                strcmp((char*)(addr + 0x568), "nk.mx") == 0)
            {
                isNokiaModule = TRUE;
            }

            if (!isNokiaModule)
            {
                addr += memInfo.RegionSize;
                continue;
            }

            const DWORD offsets[] = {0x252CAF, 0x2475AB, 0x2B92DC, 0x206555};
            for (unsigned i = 0; i < sizeof(offsets) / sizeof(offsets[0]); i++)
            {
                BYTE* patchAddr = addr + offsets[i];
                if (patchAddr[0] == 0x0F && patchAddr[1] == 0x84) // v3.5
                {
                    memset(patchAddr, 0x90, 6); // nop broken check on rpl save on win7
                    g_HasPatchedNokiaSaveRpl = true;
                    break;
                }
                else if (patchAddr[0] == 0x73 && patchAddr[1] == 0x51) // v3.3
                {
                    patchAddr[0] = 0xEB;    // force jmp to rpl save
                    g_HasPatchedNokiaSaveRpl = true;
                    break;
                }
            }

            if (g_HasPatchedNokiaSaveRpl) // patch interface check for gen selector on 3.5
            {
                BYTE* intfCheck = addr + 0x20FC7C;
                if (intfCheck[0] == 0x73 && intfCheck[1] == 0x54) // for 1.4
                {
                    intfCheck[0] = 0xEB; // jmp to add all options
                    BYTE* msgBoxCheck = addr + 0x1A5851;
                    msgBoxCheck[0] = 0xEB; // skip warning msg box
                }

                intfCheck = addr + 0x279446;
                if (intfCheck[0] == 0x73 && intfCheck[1] == 0x54) // for 2.7
                {
                    intfCheck[0] = 0xEB; // jmp to add all options
                    BYTE* msgBoxCheck = addr + 0x200E66;
                    msgBoxCheck[0] = 0xEB; // skip warning msg box
                }

                return;
            }
        }

        addr += memInfo.RegionSize;
    }
}

typedef BOOL (__stdcall *MobileExDllMain_t)(HINSTANCE, DWORD, LPVOID);
MobileExDllMain_t MobileExDllMainOrigFunc = NULL;

BOOL __stdcall MobileExDllMain_Hook(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    // return original bytes, VMProtect checks it
    memcpy(g_MobileExDllEntryPoint, g_MobileExDllEntryPointBytes, sizeof(g_MobileExDllEntryPointBytes));

    g_CollectWinscardCandidates = TRUE;
    BOOL result = MobileExDllMainOrigFunc(hinstDLL, fdwReason, lpReserved);
    g_CollectWinscardCandidates = FALSE;
    g_HasPatchedVmCheck = TRUE;
    g_HasPatchedNokiaUfsJaf = TRUE;
    PatchModulesSignatureCheck();

    // FuncEntrySignature entryBytes[] = {
    //     {{0x8B, 0xFF, 0x55, 0x8B, 0xEC}, 5}
    // };
    // HMODULE comdlg32 = GetModuleHandleA("comdlg32.dll");
    // HookFunctionWithFallbackSafe(comdlg32, "GetSaveFileNameA", entryBytes, sizeof(entryBytes), NULL, (void*)GetSaveFileNameA_Hook, FALSE);
    // HookFunctionWithFallbackSafe(comdlg32, "GetOpenFileNameA", entryBytes, sizeof(entryBytes), NULL, (void*)GetOpenFileNameA_Hook, FALSE);

    for (unsigned int i = 0; i < g_WinscardCandidatesCount; i++)
    {
        HMODULE hModule = (HMODULE)g_WinscardCandidates[i];
        if (!CustomGetProcAddress(hModule, "SCardTransmit"))
        {
            continue;
        }

        g_hWinscard = hModule;
        break;
    }

    if (!g_hWinscard)
    {
        MessageBoxA(NULL, "Failed to hook winscard.dll", "Error", MB_ICONERROR);
        return result;
    }

    HookWinscardFunctions();
    return result;
}

typedef LPVOID (WINAPI *VirtualAlloc_t)(LPVOID, SIZE_T, DWORD, DWORD);
VirtualAlloc_t VirtualAllocOrigFunc = NULL;

void VirtualAlloc_HookImpl(LPVOID lpAddress, SIZE_T dwSize)
{
    // LogToFile("VirtualAllocOrigFunc: %p / %X", lpAddress, dwSize);

    if (g_HasHookedMobileExDll)
    {
        PatchVmCheck();
        PatchNokiaUfsJaf();
        PatchNokiaSaveRpl();

        if (g_CollectWinscardCandidates && lpAddress)
        {
            g_WinscardCandidates[g_WinscardCandidatesCount++] = (BYTE*)lpAddress;
        }

        return;
    }

    if (g_MobileExDllBase && g_AllocCount > 5) // let it init
    {
        BYTE* entryPoint = GetAddressOfEntryPoint((HMODULE)g_MobileExDllBase);
        if (entryPoint && ((BYTE*)entryPoint)[0] != 0)
        {
            g_HasHookedMobileExDll = TRUE;
            g_MobileExDllEntryPoint = entryPoint;
            memcpy(g_MobileExDllEntryPointBytes, g_MobileExDllEntryPoint, sizeof(g_MobileExDllEntryPointBytes));

            HookFunctionWithFallback((HMODULE)g_MobileExDllBase, "DllMain", g_MobileExDllMainEntrySigs,
               sizeof(g_MobileExDllMainEntrySigs), (void**)&MobileExDllMainOrigFunc, (void*)MobileExDllMain_Hook);
        }
    }
    else if (g_MobileExDllBase)
    {
        g_AllocCount++;
    }

    if (!g_MobileExDllBase && (
            lpAddress == (LPVOID)0x10080000 ||
            dwSize == 0x45E000 || // some fallback sizes for different versions in case base address is different
            dwSize == 0x467000 ||
            dwSize == 0x583000 ||
            dwSize == 0x582000 ||
            dwSize == 0x597000 ||
            dwSize == 0x5BA000))
    {
        g_MobileExDllBase = (BYTE*)lpAddress;
    }
}

LPVOID __stdcall VirtualAlloc_Hook(LPVOID lpAddress, SIZE_T dwSize, DWORD  flAllocationType, DWORD  flProtect)
{
    VirtualAlloc_HookImpl(lpAddress, dwSize);
    return VirtualAllocOrigFunc(lpAddress, dwSize, flAllocationType, flProtect);
}

static FuncEntrySignature g_SCardFallbackBytes[] =
{
    {{0x6A, 0xFF, 0xB8, 0xFF, 0xFF, 0xFF, 0xFF}, 7},
    {{0x68, 0xFF, 0xFF, 0xFF, 0xFF}, 5},
    {{0xB8, 0xFF, 0xFF, 0xFF, 0xFF}, 5}
};

void HookSCardFunction(void** origFuncPtr, void* destHookFuncPtr, const char* funcName)
{
    HookFunctionWithFallback(g_hWinscard, funcName, g_SCardFallbackBytes, sizeof(g_SCardFallbackBytes), origFuncPtr , destHookFuncPtr);
}

void HookWinscardFunctions()
{
    HookSCardFunction((void**)&SCardEstablishContextOrigFunc, (void*)SCardEstablishContext_Hook, "SCardEstablishContext");
    HookSCardFunction((void**)&SCardReleaseContextOrigFunc, (void*)SCardReleaseContext_Hook, "SCardReleaseContext");
    HookSCardFunction((void**)&SCardIsValidContextOrigFunc, (void*)SCardIsValidContext_Hook, "SCardIsValidContext");
    HookSCardFunction((void**)&SCardListReadersAOrigFunc, (void*)SCardListReadersA_Hook, "SCardListReadersA");
    HookSCardFunction((void**)&SCardConnectAOrigFunc, (void*)SCardConnectA_Hook, "SCardConnectA");
    HookSCardFunction((void**)&SCardDisconnectOrigFunc, (void*)SCardDisconnect_Hook, "SCardDisconnect");
    HookSCardFunction((void**)&SCardBeginTransactionOrigFunc, (void*)SCardBeginTransaction_Hook, "SCardBeginTransaction");
    HookSCardFunction((void**)&SCardEndTransactionOrigFunc, (void*)SCardEndTransaction_Hook, "SCardEndTransaction");
    HookSCardFunction((void**)&SCardTransmitOrigFunc, (void*)SCardTransmit_Hook, "SCardTransmit");
    HookSCardFunction((void**)&SCardGetAttribOrigFunc, (void*)SCardGetAttrib_Hook, "SCardGetAttrib");
    HookSCardFunction((void**)&SCardGetStatusChangeAOrigFunc, (void*)SCardGetStatusChangeA_Hook, "SCardGetStatusChangeA");
}

typedef BOOL (WINAPI *GetVersionExA_t)(LPOSVERSIONINFOA);
GetVersionExA_t GetVersionExAOrigFunc = NULL;

BOOL WINAPI GetVersionExA_Hook(LPOSVERSIONINFOA lpVersionInformation)
{
    if (!g_IsOnNewerWindows)
    {
        return GetVersionExAOrigFunc(lpVersionInformation);
    }

    // windows 7
    lpVersionInformation->dwMajorVersion = 6; // 5 - xp
    lpVersionInformation->dwMinorVersion = 1;
    lpVersionInformation->dwBuildNumber = 7600;
    lpVersionInformation->dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    lpVersionInformation->dwPlatformId = 2;
    strcpy(lpVersionInformation->szCSDVersion, "Service Pack 1");
    return TRUE;
}

typedef LONG (NTAPI *NtQuerySystemTime_t)(PLARGE_INTEGER);
NtQuerySystemTime_t NtQuerySystemTimeOrigFunc = NULL;

LONG NTAPI NtQuerySystemTime_Hook(PLARGE_INTEGER SystemTime)
{
    LONG result = NtQuerySystemTimeOrigFunc(SystemTime);
    SystemTime->QuadPart = 123456789;
    return result;
}

void WINAPI GetLocalTime_Hook(LPSYSTEMTIME t)
{
    t->wYear=2010; t->wMonth=5; t->wDayOfWeek=5; t->wDay=5;
    t->wHour=12; t->wMinute=12; t->wSecond=12; t->wMilliseconds=12;
}

void WINAPI GetSystemTime_Hook(LPSYSTEMTIME t)
{
    t->wYear=2010; t->wMonth=5; t->wDayOfWeek=5; t->wDay=5;
    t->wHour=12; t->wMinute=12; t->wSecond=12; t->wMilliseconds=12;
}

void WINAPI GetSystemTimeAsFileTime_Hook(LPFILETIME ft)
{
    ft->dwLowDateTime=0x11111111;
    ft->dwHighDateTime=0x01C00000;
}

void WINAPI GlobalMemoryStatus_Hook(LPMEMORYSTATUS s)
{
    s->dwLength = sizeof(MEMORYSTATUS); s->dwMemoryLoad = 50; s->dwTotalPhys = 0x80000000; s->dwAvailPhys = 0x40000000;
    s->dwTotalPageFile = 0x80000000; s->dwAvailPageFile = 0x40000000; s->dwTotalVirtual = 0x7FFF0000; s->dwAvailVirtual = 0x40000000;
}

typedef void (NTAPI *RtlTimeToTimeFields_t)(PLARGE_INTEGER, TIME_FIELDS*);
RtlTimeToTimeFields_t RtlTimeToTimeFieldsOrigFunc = NULL;
void NTAPI RtlTimeToTimeFields_Hook(PLARGE_INTEGER Time, TIME_FIELDS* TimeFields)
{
    RtlTimeToTimeFieldsOrigFunc(Time, TimeFields);
    if (TimeFields && TimeFields->Year >= 2010) // fix app expired error
    {
        TimeFields->Year = 2010; TimeFields->Month = 5; TimeFields->Day = 5; TimeFields->Weekday = 5;
        TimeFields->Hour = 12; TimeFields->Minute = 12; TimeFields->Second = 12; TimeFields->Milliseconds = 12;
    }
}

BOOL IsRawDiskA(LPCSTR fileName) // mxkey can wipe disk when it detects duplicated responses in a row, just to be safe
{
    if(!fileName) return FALSE;
    return StringIContains(fileName, "PHYSICALDRIVE") || StringIContains(fileName, "\\Harddisk");
}
BOOL IsRawDiskW(LPCWSTR fileName)
{
    if(!fileName) return FALSE;
    return WStringIContains(fileName, L"PHYSICALDRIVE") || WStringIContains(fileName, L"\\Harddisk");
}

typedef HANDLE (WINAPI *CreateFileA_t)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
CreateFileA_t CreateFileAOrigFunc = NULL;

HANDLE WINAPI CreateFileA_Hook(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
{
    if(IsRawDiskA(lpFileName))
    {
        return INVALID_HANDLE_VALUE;
    }

    HANDLE result = CreateFileAOrigFunc(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
        dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    
    //if (lpFileName && strcmp(lpFileName, "C:\\dump_info.txt") != 0)
    //{
    //    LogToFile("CreateFileA_Hook: %x : %s", result, lpFileName);
    //}

    return result;
}

typedef HANDLE (WINAPI *CreateFileW_t)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
CreateFileW_t CreateFileWOrigFunc = NULL;

HANDLE WINAPI CreateFileW_Hook(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
{
    if(IsRawDiskW(lpFileName))
    {
        return INVALID_HANDLE_VALUE;
    }

    HANDLE result = CreateFileWOrigFunc(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
        dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

    LogToFileW(L"CreateFileW_Hook: %x : %s", result, lpFileName);
    return result;
}

static LPSTR g_CmdLineA;
static LPWSTR g_CmdLineW;

LPSTR WINAPI GetCommandLineA_Hook()
{
    return g_CmdLineA;
}

LPWSTR WINAPI GetCommandLineW_Hook()
{
    return g_CmdLineW;
}

BYTE g_LastWriteBuff[2000];
DWORD g_LastWriteBuffLength = 0;

typedef BOOL (WINAPI *ReadFile_t)(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
ReadFile_t ReadFileOrigFunc = NULL;

BOOL WINAPI ReadFile_Hook(
    HANDLE       hFile,
    LPVOID       lpBuffer,
    DWORD        nNumberOfBytesToRead,
    LPDWORD      lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped
)
{
    BOOL result = ReadFileOrigFunc(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
    if (nNumberOfBytesToRead < 1024)
    {
        if (g_LastWriteBuffLength > 0)
        {
            LogBufferToFile("WriteFile", g_LastWriteBuff, g_LastWriteBuffLength);
            g_LastWriteBuffLength = 0;
        }

        LogBufferToFile("ReadFile", lpBuffer, nNumberOfBytesToRead);
    }
    
    return result;
}

typedef BOOL (WINAPI *WriteFile_t)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
WriteFile_t WriteFileOrigFunc = NULL;

BOOL WINAPI WriteFile_Hook(
    HANDLE       hFile,
    LPCVOID      lpBuffer,
    DWORD        nNumberOfBytesToWrite,
    LPDWORD      lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped
)
{
    BOOL result = WriteFileOrigFunc(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
    if (lpBuffer && nNumberOfBytesToWrite < 1024)
    {
        memcpy(g_LastWriteBuff, lpBuffer, nNumberOfBytesToWrite);
        g_LastWriteBuffLength = nNumberOfBytesToWrite;
    }

    return result;
}

BOOL IsZeroBuffer(BYTE* InBuffer, DWORD length)
{
    for (DWORD i = 0; i < length; i++)
    {
        if (InBuffer[i] != 0) return FALSE;
    }
    return TRUE;
}

static BOOL g_IsUsingUfsFtdiDrivers = FALSE;
typedef BOOL (WINAPI *DeviceIoControl_t)(HANDLE, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
DeviceIoControl_t DeviceIoControlOrigFunc = NULL;

BOOL IsBufferContainsString(BYTE* buffer, DWORD bufferLength, const char* search, DWORD searchLength)
{
    if (bufferLength < searchLength) return FALSE;
    for (DWORD i = 0; i < bufferLength - searchLength; i++)
    {
        if (memcmp(buffer + i, search, searchLength) == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL WINAPI DeviceIoControl_Hook(
    HANDLE hDevice,
    DWORD IoControlCode,
    LPVOID InBuffer,
    DWORD InBufferSize,
    LPVOID OutBuffer,
    DWORD OutBufferSize,
    LPDWORD BytesReturned,
    LPOVERLAPPED Overlapped
)
{
    if (g_IsUsingUfsFtdiDrivers && IoControlCode != 0x220408 && (IoControlCode & 0xFFFF0000) == 0x00220000)
    {
        IoControlCode |= 0x2000;
    }

    LogToFile("Device IO Control IoControlCode: dev: %x 0x%x", hDevice, IoControlCode);
    //if (!IsZeroBuffer((BYTE*)InBuffer, InBufferSize))
    //    LogBufferToFile("Device IO Control InBuffer", InBuffer, InBufferSize);

    BOOL result = DeviceIoControlOrigFunc(hDevice, IoControlCode, InBuffer, InBufferSize, OutBuffer,
        OutBufferSize, BytesReturned, Overlapped);

    // generic ftdi drivers require 13th bit to be set
    if (!g_IsUsingUfsFtdiDrivers && IoControlCode == 0x220198 && (!result || IsZeroBuffer((BYTE*)OutBuffer, *BytesReturned)))
    {
        result = DeviceIoControlOrigFunc(hDevice, 0x222198, InBuffer, InBufferSize, OutBuffer,
        OutBufferSize, BytesReturned, Overlapped);

        if (!result || *BytesReturned == 0) return FALSE;
        const char* sarasStr = "SarasSoft";
        if (IsBufferContainsString((BYTE*)OutBuffer, *BytesReturned, sarasStr, sizeof(sarasStr)))
        {
            g_IsUsingUfsFtdiDrivers = TRUE;
        }
    }

    if (!result)
        return result;

    //if (BytesReturned && !IsZeroBuffer((BYTE*)OutBuffer, *BytesReturned))
    //    LogBufferToFile("Device IO Control OutBuffer", OutBuffer, *BytesReturned);

    return result;
}

const char KEY_PREFIX[] = "SYSTEM\\CurrentControlSet\\Enum\\USB\\";
const char GUID_DEVINTERFACE_USB_HUB[] = "{f18a0e88-c30c-11d0-8815-00a0c906bed8}";
char g_LastDevice[MAX_PATH];

typedef LONG (WINAPI *RegOpenKeyExA_t)(HKEY, LPCSTR, DWORD, REGSAM, PHKEY);
RegOpenKeyExA_t RegOpenKeyExAOrigFunc = NULL;

LONG WINAPI RegOpenKeyExA_Hook(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
{
    LONG result = RegOpenKeyExAOrigFunc(hKey, lpSubKey, ulOptions, samDesired, phkResult);
    if (result == 0 && lpSubKey && strlen(lpSubKey) > sizeof(KEY_PREFIX) && strnicmp(lpSubKey, KEY_PREFIX, sizeof(KEY_PREFIX) - 1) == 0)
    {
        const char* lastSeparator = strrchr(lpSubKey, '\\');
        const char* deviceStart = lpSubKey + sizeof(KEY_PREFIX) - 5;
        int length = lastSeparator - deviceStart;
        strncpy(g_LastDevice, deviceStart, length);
        g_LastDevice[length] = '\0';
    }

    LogToFile("RegOpenKeyExA_Hook: res 0x%x / %s", result, lpSubKey);
    return result;
}

typedef LONG (WINAPI *RegQueryValueExA_t)(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
RegQueryValueExA_t RegQueryValueExAOrigFunc = NULL;

LONG WINAPI RegQueryValueExA_Hook(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    LONG result = RegQueryValueExAOrigFunc(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
    // fix for usb3 hub
    if (result != 0 && lpValueName && strcmp(lpValueName, "SymbolicName") == 0)
    {
        *lpType = REG_SZ;
        char tmpBuffer[MAX_PATH];
        char* output = (char*)lpData;
        if (!lpData) output = tmpBuffer;
        strcpy(output, "\\??\\");
        int i = 0;
        for (; g_LastDevice[i]; i++)
        {
            output[i + 4] = g_LastDevice[i] == '\\' ? '#' : g_LastDevice[i];
        }

        output[i + 4] = '#';
        strcpy(output + i + 5, GUID_DEVINTERFACE_USB_HUB);
        *lpcbData = strlen(output) + 1;
        result = 0;
    }

    LogToFile("RegQueryValueExA_Hook: res 0x%x / %s", result, lpValueName);
    return result;
}

LONG WINAPI WinVerifyTrust_Hook(HWND hwnd, GUID *pgActionID, LPVOID pWVTData)
{
    return 0;
}

void* g_LastAlloc = NULL;
const int g_AllocSize = 4096;
int g_UsedSize = 0;
const int g_JmpLength = 11;
const int g_JmpAddressOffset = 7;

// packer checks first byte of function and scans for ret instruction
// if function is too long it crashes
// create small trampoline so it passes checks
BYTE* CreateJmpPatch()
{
    VirtualAlloc_t virtulAllocFunc = VirtualAllocOrigFunc ? VirtualAllocOrigFunc : VirtualAlloc;
    if (!g_LastAlloc)
    {
        g_LastAlloc = virtulAllocFunc(NULL, g_AllocSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    }
    else if (g_UsedSize + g_JmpLength > g_AllocSize)
    {
        g_LastAlloc = virtulAllocFunc(NULL, g_AllocSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        g_UsedSize = 0;
    }

    BYTE* mem = (BYTE*)g_LastAlloc + g_UsedSize;
    mem[0] = 0xFF;
    mem[1] = 0x25;
    *(DWORD*)(mem + 2) = (DWORD)(mem + g_JmpAddressOffset);
    mem[6] = 0xC3;

    g_UsedSize += g_JmpLength;
    return mem;
}

static DWORD GetProcAddressOrHooked(LPCSTR module, HMODULE hModule, LPCSTR funcName)
{
    FARPROC originalFunc = GetProcAddress(hModule, funcName);
    if (stricmp(module, "kernel32.dll") == 0)
    {
        if (strcmp(funcName, "GetSystemTimeAsFileTime") == 0)
        {
            return (DWORD)GetSystemTimeAsFileTime_Hook;
        }
        if (strcmp(funcName, "GetVersionExA") == 0)
        {
            GetVersionExAOrigFunc = (GetVersionExA_t)originalFunc;
            return (DWORD)GetVersionExA_Hook;
        }
        if (strcmp(funcName, "GetSystemTime") == 0)
        {
            return (DWORD)GetSystemTime_Hook;
        }
        if (strcmp(funcName, "GetLocalTime") == 0)
        {
            return (DWORD)GetLocalTime_Hook;
        }
        if (strcmp(funcName, "GlobalMemoryStatus") == 0)
        {
            return (DWORD)GlobalMemoryStatus_Hook;
        }
        if (strcmp(funcName, "GetCommandLineA") == 0)
        {
            return (DWORD)GetCommandLineA_Hook;
        }
        if (strcmp(funcName, "GetCommandLineW") == 0)
        {
            return (DWORD)GetCommandLineW_Hook;
        }
        if (strcmp(funcName, "VirtualAlloc") == 0)
        {
            VirtualAllocOrigFunc = (VirtualAlloc_t)originalFunc;
            return (DWORD)VirtualAlloc_Hook;
        }
        if (strcmp(funcName, "CreateFileA") == 0)
        {
            CreateFileAOrigFunc = (CreateFileA_t)originalFunc;
            return (DWORD)CreateFileA_Hook;
        }
        if (strcmp(funcName, "CreateFileW") == 0)
        {
            CreateFileWOrigFunc = (CreateFileW_t)originalFunc;
            return (DWORD)CreateFileW_Hook;
        }
        /*if (strcmp(funcName, "ReadFile") == 0)
        {
            ReadFileOrigFunc = (ReadFile_t)originalFunc;
            return (DWORD)ReadFile_Hook;
        }
        if (strcmp(funcName, "WriteFile") == 0)
        {
            WriteFileOrigFunc = (WriteFile_t)originalFunc;
            return (DWORD)WriteFile_Hook;
        }*/
        if (strcmp(funcName, "DeviceIoControl") == 0)
        {
            DeviceIoControlOrigFunc = (DeviceIoControl_t)originalFunc;
            return (DWORD)DeviceIoControl_Hook;
        }
    }
    else if (stricmp(module, "ntdll.dll") == 0)
    {
        if (strcmp(funcName, "RtlTimeToTimeFields") == 0)
        {
            RtlTimeToTimeFieldsOrigFunc = (RtlTimeToTimeFields_t)originalFunc;
            return (DWORD)RtlTimeToTimeFields_Hook;
        }
        if (strcmp(funcName, "NtQuerySystemTime") == 0)
        {
            NtQuerySystemTimeOrigFunc = (NtQuerySystemTime_t)originalFunc;
            return (DWORD)NtQuerySystemTime_Hook;
        }
    }
    else if (stricmp(module, "advapi32.dll") == 0)
    {
        if (strcmp(funcName, "RegQueryValueExA") == 0)
        {
            RegQueryValueExAOrigFunc = (RegQueryValueExA_t)originalFunc;
            return (DWORD)RegQueryValueExA_Hook;
        }
        if (strcmp(funcName, "RegOpenKeyExA") == 0)
        {
            RegOpenKeyExAOrigFunc = (RegOpenKeyExA_t)originalFunc;
            return (DWORD)RegOpenKeyExA_Hook;
        }
    }
    else if (stricmp(module, "wintrust.dll") == 0)
    {
        if (strcmp(funcName, "WinVerifyTrust") == 0)
        {
            return (DWORD)WinVerifyTrust_Hook;
        }
    }

    return (DWORD)originalFunc;
}

static BOOL HookExports(LPCSTR module, HMODULE hModule)
{
    BYTE* base = (BYTE*)hModule;

    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
    {
        return FALSE;
    }

    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
    {
        return FALSE;
    }

    IMAGE_DATA_DIRECTORY expDir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (expDir.VirtualAddress == 0)
    {
        return FALSE;
    }

    IMAGE_EXPORT_DIRECTORY* exports = (IMAGE_EXPORT_DIRECTORY*)(base + expDir.VirtualAddress);

    DWORD* nameRVAs = (DWORD*)(base + exports->AddressOfNames);
    WORD* ordinals = (WORD*)(base + exports->AddressOfNameOrdinals);
    DWORD* funcRVAs = (DWORD*)(base + exports->AddressOfFunctions);

    for (DWORD i = 0; i < exports->NumberOfNames; i++)
    {
        const char* name = (const char*)(base + nameRVAs[i]);
        WORD ordinal = ordinals[i];
        DWORD funcRVA = funcRVAs[ordinal];
        BYTE* origFuncAddr = base + funcRVA;

        DWORD oldProtect;
        if (!VirtualProtect(&funcRVAs[ordinal], sizeof(DWORD), PAGE_READWRITE, &oldProtect))
        {
            return FALSE;
        }

        DWORD proc = GetProcAddressOrHooked(module, hModule, name);
        BYTE* jmp = CreateJmpPatch();
        *(DWORD*)(jmp + g_JmpAddressOffset) = proc;

        funcRVAs[ordinal] = (DWORD)jmp - (DWORD)hModule;
        VirtualProtect(&funcRVAs[ordinal], sizeof(DWORD), oldProtect, &oldProtect);
    }

    return FALSE;
}

static HMODULE g_FixedDlls[200];
static int g_FixedDllsCount = 0;

BOOL HasFixedDll(HMODULE hModule)
{
    for (int i = 0; i < g_FixedDllsCount; i++)
    {
        if (hModule == g_FixedDlls[i])
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL IsDllHookIgnored(LPCSTR module)
{
    // skip vsport and winsock on xp, it doesnt like exports hooking
    return
        StringIContains(module, "ws2") ||
        StringIContains(module, "vsport");
}

typedef HMODULE(WINAPI* GetModuleHandleA_t)(LPCSTR);
GetModuleHandleA_t GetModuleHandleAOrigFunc = NULL;

typedef HMODULE(WINAPI* LoadLibraryA_t)(LPCSTR);
LoadLibraryA_t LoadLibraryAOrigFunc = NULL;

HMODULE WINAPI GetModuleHandleA_Hook(LPCSTR lpModuleName)
{
    if (lpModuleName)
        LogToFile("GetModuleHandleA_Hook: %s", lpModuleName);

    HMODULE hModule = GetModuleHandleAOrigFunc(lpModuleName);
    if (lpModuleName && !hModule)
    {
        hModule = LoadLibraryAOrigFunc(lpModuleName);
    }

    if (lpModuleName && hModule && !IsDllHookIgnored(lpModuleName) && !HasFixedDll(hModule))
    {
        char* moduleName = new char[MAX_PATH];
        strcpy(moduleName, lpModuleName);
        g_FixedDlls[g_FixedDllsCount++] = hModule;
        HookExports(lpModuleName, hModule);
    }

    return hModule;
}

typedef HRESULT(WINAPI *DllRegisterServer_t)();

HMODULE WINAPI LoadLibraryA_Hook(LPCSTR lpLibFileName)
{
    if (lpLibFileName)
        LogToFile("LoadLibraryA_Hook: %s", lpLibFileName);

    HMODULE hModule = LoadLibraryAOrigFunc(lpLibFileName);
    if (lpLibFileName && hModule && !IsDllHookIgnored(lpLibFileName) && !HasFixedDll(hModule))
    {
        char* moduleName = new char[MAX_PATH];
        strcpy(moduleName, lpLibFileName);
        g_FixedDlls[g_FixedDllsCount++] = hModule;
        HookExports(lpLibFileName, hModule);
    }

    if (lpLibFileName && hModule && StringIContains(lpLibFileName, "vsport.dll"))
    {
        DllRegisterServer_t registerFunc = (DllRegisterServer_t)GetProcAddress(hModule, "DllRegisterServer");
        registerFunc();
    }

    return hModule;
}

LPSTR GenerateGuid()
{
    char* buffer = new char[39];
    buffer[0] = '{';
    for (int i = 1; i < 37; i++)
    {
        if (i == 9 || i == 14 || i == 19 || i == 24)
        {
            buffer[i] = '-';
            continue;
        }

        sprintf(buffer + i, "%x", rand() % 0x10);
    }

    buffer[37] = '}';
    buffer[38] = '\0';
    return buffer;
}

void InstallHooks()
{
    srand((unsigned int)time(NULL));
    // cmd args should be unique guid on each run
    g_CmdLineA = GenerateGuid();
    g_CmdLineW = new wchar_t[100];
    mbstowcs(g_CmdLineW, g_CmdLineA, 100);

    g_IsOnNewerWindows = GetModuleHandleA("kernelbase.dll") != NULL;
    EatHookFunction(GetModuleHandleA("kernel32.dll"), "LoadLibraryA", (void**)&LoadLibraryAOrigFunc, (void*)LoadLibraryA_Hook, TRUE);
    EatHookFunction(GetModuleHandleA("kernel32.dll"), "GetModuleHandleA", (void**)&GetModuleHandleAOrigFunc, (void*)GetModuleHandleA_Hook, TRUE);
    IatHookFunction("kernel32.dll", "LoadLibraryA", NULL, (void*)LoadLibraryA_Hook, TRUE);
    IatHookFunction("kernel32.dll", "GetModuleHandleA", NULL, (void*)GetModuleHandleA_Hook, TRUE);
}

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
		SetupWinmmFunctions();
        InstallHooks();
    }

    return TRUE;
}