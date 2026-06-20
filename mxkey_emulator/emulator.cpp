#define _WIN32_WINNT 0x0400

#include <windows.h>
#include "logging.h"
#include "winmm_hook.h"
#include "hook_helper.h"
#include "emulator_helper.h"

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

typedef struct {
    BOOL    VirtualAlloc;
    BOOL    CreateFileA;
    BOOL    GetLocalTime;
    BOOL    GetSystemTime;
    BOOL    GetSystemTimeAsFileTime;
    BOOL    GlobalMemoryStatus;
} HookStatus;

void HookKernelFunctions(HMODULE hModule, HookStatus* status, BOOL skipFirst, BOOL errorOnFail);

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
static BOOL g_HasPatchedNokiaSaveRpl = FALSE;

static BOOL g_CollectWinscardCandidates = FALSE;
static BYTE* g_WinscardCandidates[50];
static DWORD g_WinscardCandidatesCount;

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

typedef BOOL (WINAPI *GetSaveFileNameA_t)(LPOPENFILENAMEA);
GetSaveFileNameA_t GetSaveFileNameAOrigFunc = NULL;

BOOL __stdcall GetSaveFileNameA_Hook(LPOPENFILENAMEA p)
{
    BOOL result = FALSE;
    __try
    {
        result = GetSaveFileNameAOrigFunc(p); 
    }
    __except(EXCEPTION_EXECUTE_HANDLER) // fix for win7, lpstrFile is already populated
    {
        result = p->lpstrFile != NULL;
    }

    return result;
}

void PatchVmCheck()
{
    if (g_HasPatchedVmCheck) return;
    const BYTE patch[] = {0x33,0xC0};
    const DWORD offsets[] = {0xF59BE, 0x12E6AD, 0x12E77D, 0x1262B9, 0xF9BD2, 0xFF06A, 0x10C76A}; // offsets for different versions

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

void PatchModulesSignatureCheck()
{
    const BYTE patch[] = {0xB0, 0x01, 0xC3};
    const DWORD offsets[] = {0x1AD020, 0x1852C0}; // for v3.5
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

            const DWORD offsets[] = {0x252CAF, 0x2B92DC, 0x206555};
            for (unsigned i = 0; i < sizeof(offsets) / sizeof(offsets[0]); i++)
            {
                BYTE* patchAddr = addr + offsets[i];
                if (patchAddr[0] == 0x0F && patchAddr[1] == 0x84) // v3.5
                {
                    memset(patchAddr, 0x90, 6); // nop broken check on rpl save on win7
                    g_HasPatchedNokiaSaveRpl = true;
                    return;
                }
                else if (patchAddr[0] == 0x73 && patchAddr[1] == 0x51) // v3.3
                {
                    patchAddr[0] = 0xEB;    // force jmp to rpl save
                    g_HasPatchedNokiaSaveRpl = true;
                    return;
                }
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
    PatchModulesSignatureCheck();

    HMODULE comdlg32 = GetModuleHandleA("comdlg32.dll");
    FuncEntrySignature entryBytes[] = {
        {{0x8B, 0xFF, 0x55, 0x8B, 0xEC}, 5}
    };
    HookFunctionWithFallbackSafe(comdlg32, "GetSaveFileNameA", entryBytes, sizeof(entryBytes), (void**)&GetSaveFileNameAOrigFunc , (void*)GetSaveFileNameA_Hook, FALSE);

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

typedef LPVOID (__stdcall *VirtualAlloc_t)(LPVOID, SIZE_T, DWORD, DWORD);
VirtualAlloc_t VirtualAllocOrigKBaseFunc = NULL;
VirtualAlloc_t VirtualAllocOrigK32Func = NULL;

void VirtualAlloc_Hook(LPVOID lpAddress, SIZE_T dwSize)
{
    // LogToFile("VirtualAllocOrigFunc: %p / %X", lpAddress, dwSize);

    if (g_HasHookedMobileExDll)
    {
        PatchVmCheck();
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

LPVOID __stdcall VirtualAlloc_K32Hook(LPVOID lpAddress, SIZE_T dwSize, DWORD  flAllocationType, DWORD  flProtect)
{
    VirtualAlloc_Hook(lpAddress, dwSize);
    return VirtualAllocOrigK32Func(lpAddress, dwSize, flAllocationType, flProtect);
}

LPVOID __stdcall VirtualAlloc_KBaseHook(LPVOID lpAddress, SIZE_T dwSize, DWORD  flAllocationType, DWORD  flProtect)
{
    VirtualAlloc_Hook(lpAddress, dwSize);
    return VirtualAllocOrigKBaseFunc(lpAddress, dwSize, flAllocationType, flProtect);
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

typedef LONG (NTAPI *NtQuerySystemTime_t)(PLARGE_INTEGER);
NtQuerySystemTime_t NtQuerySystemTimeOrigFunc = NULL;

LONG NTAPI NtQuerySystemTime_Hook(PLARGE_INTEGER SystemTime)
{
    LONG result = NtQuerySystemTimeOrigFunc(SystemTime);
    SystemTime->QuadPart = 123456789;
    return result;
}

void __stdcall GetLocalTime_Hook(LPSYSTEMTIME t)
{
    t->wYear=2010; t->wMonth=5; t->wDayOfWeek=5; t->wDay=5;
    t->wHour=12; t->wMinute=12; t->wSecond=12; t->wMilliseconds=12;
}

void __stdcall GetSystemTime_Hook(LPSYSTEMTIME t)
{
    t->wYear=2010; t->wMonth=5; t->wDayOfWeek=5; t->wDay=5;
    t->wHour=12; t->wMinute=12; t->wSecond=12; t->wMilliseconds=12;
}

void __stdcall GetSystemTimeAsFileTime_Hook(LPFILETIME ft)
{
    ft->dwLowDateTime=0x11111111;
    ft->dwHighDateTime=0x01C00000;
}

void __stdcall GlobalMemoryStatus_Hook(LPMEMORYSTATUS s)
{
    s->dwLength = sizeof(MEMORYSTATUS);
    s->dwMemoryLoad = 50;
    s->dwTotalPhys = 0x80000000;
    s->dwAvailPhys = 0x40000000;
    s->dwTotalPageFile = 0x80000000;
    s->dwAvailPageFile = 0x40000000;
    s->dwTotalVirtual = 0x7FFF0000;
    s->dwAvailVirtual = 0x40000000;
}

typedef void (__stdcall *RtlTimeToTimeFields_t)(PLARGE_INTEGER, TIME_FIELDS*);
RtlTimeToTimeFields_t RtlTimeToTimeFieldsOrigFunc = NULL;

void __stdcall RtlTimeToTimeFields_Hook(PLARGE_INTEGER Time, TIME_FIELDS* TimeFields)
{
    RtlTimeToTimeFieldsOrigFunc(Time, TimeFields);
    if (TimeFields && TimeFields->Year > 2020) // fix app expired error
    {
        TimeFields->Year = 2010;
        TimeFields->Month = 5;
        TimeFields->Day = 5;
        TimeFields->Weekday = 5;
        TimeFields->Hour = 12;
        TimeFields->Minute = 12;
        TimeFields->Second = 12;
        TimeFields->Milliseconds = 12;
    }
}

// case-insensitive
BOOL StringContains(LPCSTR str, LPCSTR search)
{
    if(!str || !search)
    {
        return NULL;
    }

    size_t searchLen = strlen(search);
    for(; *str; str++)
    {
        if(_strnicmp(str, search, searchLen) == 0)
        {
            return TRUE;
        }    
    }

    return FALSE;
}

BOOL IsRawDisk(LPCSTR fileName) // mxkey can wipe disk when it detects duplicated responses in a row, just to be safe
{
    if(!fileName) return FALSE;
    return StringContains(fileName, "PHYSICALDRIVE") || StringContains(fileName, "\\Harddisk");
}

typedef HANDLE (WINAPI *CreateFileA_t)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
CreateFileA_t CreateFileAKBaseOrigFunc = NULL;
CreateFileA_t CreateFileAK32OrigFunc = NULL;

HANDLE WINAPI CreateFileA_KBaseHook(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
{
    if(IsRawDisk(lpFileName))
    {
        return INVALID_HANDLE_VALUE;
    }

    return CreateFileAKBaseOrigFunc(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
        dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

HANDLE WINAPI CreateFileA_K32Hook(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
{
    if(IsRawDisk(lpFileName))
    {
        return INVALID_HANDLE_VALUE;
    }

    return CreateFileAK32OrigFunc(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
        dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

void InstallHooks()
{
    HMODULE hModule = GetModuleHandleA("ntdll.dll");

    FuncEntrySignature timeToTimeFieldBytes[] = {
        {{0x8B, 0xFF, 0x55, 0x8B, 0xEC}, 5}
    };
    HookFunctionWithFallback(hModule, "RtlTimeToTimeFields", timeToTimeFieldBytes, sizeof(timeToTimeFieldBytes), (void**)&RtlTimeToTimeFieldsOrigFunc, (void*)RtlTimeToTimeFields_Hook);

    FuncEntrySignature qtimeBytes[] = {
        {{0xB8, 0xFF, 0xFF, 0xFF, 0xFF}, 5}
    };
    HookFunctionWithFallback(hModule, "NtQuerySystemTime", qtimeBytes, sizeof(qtimeBytes), (void**)&NtQuerySystemTimeOrigFunc, (void*)NtQuerySystemTime_Hook);

    FuncEntrySignature entryBytes[] = {
        {{0x8B, 0xFF, 0x55, 0x8B, 0xEC}, 5}
    };

    HookStatus status = {0};
    hModule = GetModuleHandleA("kernelbase.dll");
    if (hModule)
    {
        status.VirtualAlloc = HookFunctionWithFallbackSafe(hModule, "VirtualAlloc", entryBytes, sizeof(entryBytes),
            (void**)&VirtualAllocOrigKBaseFunc , (void*)VirtualAlloc_KBaseHook, FALSE);
        status.CreateFileA = HookFunctionWithFallbackSafe(hModule, "CreateFileA", entryBytes, sizeof(entryBytes),
            (void**)&CreateFileAKBaseOrigFunc, (void*)CreateFileA_KBaseHook, FALSE);

        HookKernelFunctions(hModule, &status, TRUE, FALSE);
    }

    hModule = GetModuleHandleA("kernel32.dll");
    HookKernelFunctions(hModule, &status, FALSE, TRUE);
}

void HookKernelFunctions(HMODULE hModule, HookStatus* status, BOOL skipFirst, BOOL errorOnFail)
{
    FuncEntrySignature entryBytes[] = {
        {{0x8B, 0xFF, 0x55, 0x8B, 0xEC}, 5}
    };
    if (!skipFirst)
    {
        status->VirtualAlloc = HookFunctionWithFallbackSafe(hModule, "VirtualAlloc", entryBytes, sizeof(entryBytes),
            (void**)&VirtualAllocOrigK32Func , (void*)VirtualAlloc_K32Hook, !status->VirtualAlloc && errorOnFail);
        status->CreateFileA = HookFunctionWithFallbackSafe(hModule, "CreateFileA", entryBytes, sizeof(entryBytes),
            (void**)&CreateFileAK32OrigFunc, (void*)CreateFileA_K32Hook, !status->CreateFileA && errorOnFail);
    }

    status->GetLocalTime = HookFunctionWithFallbackSafe(hModule, "GetLocalTime", entryBytes, sizeof(entryBytes), NULL,
        (void*)GetLocalTime_Hook, !status->GetLocalTime && errorOnFail);
    status->GetSystemTime = HookFunctionWithFallbackSafe(hModule, "GetSystemTime", entryBytes, sizeof(entryBytes), NULL,
        (void*)GetSystemTime_Hook, !status->GetSystemTime && errorOnFail);
    status->GetSystemTimeAsFileTime = HookFunctionWithFallbackSafe(hModule, "GetSystemTimeAsFileTime", entryBytes, sizeof(entryBytes),
        NULL, (void*)GetSystemTimeAsFileTime_Hook, !status->GetSystemTimeAsFileTime && errorOnFail);
    status->GlobalMemoryStatus = HookFunctionWithFallbackSafe(hModule, "GlobalMemoryStatus", entryBytes, sizeof(entryBytes),
        NULL, (void*)GlobalMemoryStatus_Hook, !status->GlobalMemoryStatus && errorOnFail);
}

LONG DeleteRegistryTree(HKEY hRoot, LPCSTR subKey)
{
    HKEY hKey;
    LONG status = RegOpenKeyExA(hRoot, subKey, 0, KEY_READ | KEY_WRITE, &hKey);

    if (status != ERROR_SUCCESS)
    {
        return status;
    }

    char childName[256];
    DWORD childLen;

    while (TRUE)
    {
        childLen = sizeof(childName);
        status = RegEnumKeyExA(hKey, 0, childName, &childLen, NULL, NULL, NULL, NULL);
        if (status == ERROR_NO_MORE_ITEMS)
        {
            break;
        }

        if (status != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return status;
        }

        status = DeleteRegistryTree(hKey, childName);
        if (status != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return status;
        }
    }

    RegCloseKey(hKey);
    return RegDeleteKeyA(hRoot, subKey);
}

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        // sometimes app is stuck on startup, deleting registry helps
        DeleteRegistryTree(HKEY_CURRENT_USER, "Software\\mobileEx");

		SetupWinmmFunctions();
        InstallHooks();

    }
    return TRUE;
}
