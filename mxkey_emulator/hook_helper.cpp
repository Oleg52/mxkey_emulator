#include <windows.h>
#include "hook_helper.h"
#include "logging.h"

static const char* DLL_MAIN_FUNC_NAME = "DllMain";

BOOL IsMemoryReadable(HMODULE hModule)
{
    MEMORY_BASIC_INFORMATION mbi;
    if (!VirtualQuery(hModule, &mbi, sizeof(mbi)))
    {
        return FALSE;
    }

    if (mbi.State != MEM_COMMIT)
    {
        return FALSE;
    }

    if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))
    {
        return FALSE;
    }

    return TRUE;
}

BYTE* GetAddressOfEntryPoint(HMODULE hModule)
{
    if (!IsMemoryReadable(hModule))
    {
        return FALSE;
    }

    BYTE* base = (BYTE*)hModule;

    IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) // "MZ"
        return NULL;

    IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) // "PE\0\0"
        return NULL;

    return (BYTE*)hModule + nt->OptionalHeader.AddressOfEntryPoint;
}

BYTE* CustomGetProcAddress(HMODULE hModule, const char* funcName)
{
    if (strcmp(funcName, DLL_MAIN_FUNC_NAME) == 0)
    {
        return GetAddressOfEntryPoint(hModule);
    }

    if (!IsMemoryReadable(hModule))
    {
        return FALSE;
    }

    BYTE* base = (BYTE*)hModule;

    // 1. DOS header sits at the very start of the image.
    IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) // "MZ"
        return NULL;

    // 2. e_lfanew is the offset (from base) to the NT headers.
    IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) // "PE\0\0"
        return NULL;

    // 3. Grab the export directory's RVA from the data directory.
    IMAGE_DATA_DIRECTORY expDir =
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (expDir.VirtualAddress == 0)
        return NULL; // no exports

    IMAGE_EXPORT_DIRECTORY* exports =
        (IMAGE_EXPORT_DIRECTORY*)(base + expDir.VirtualAddress);

    // 4. Three parallel arrays, all stored as RVAs:
    DWORD* nameRVAs   = (DWORD*)(base + exports->AddressOfNames);        // char* names
    WORD*  ordinals   = (WORD*) (base + exports->AddressOfNameOrdinals); // name -> func index
    DWORD* funcRVAs   = (DWORD*)(base + exports->AddressOfFunctions);    // func addresses

    // 5. Linear search the name table.
    for (DWORD i = 0; i < exports->NumberOfNames; i++) {
        const char* name = (const char*)(base + nameRVAs[i]);
        if (strcmp(name, funcName) == 0) {
            WORD  ordinal  = ordinals[i];          // index into funcRVAs
            DWORD funcRVA  = funcRVAs[ordinal];
            BYTE* funcAddr = base + funcRVA;

            // 6. Forwarder check: if the RVA points back inside the export
            //    directory, it's a forwarder string ("NTDLL.RtlExitUserThread"),
            //    not real code. GetProcAddress would resolve it further.
            if (funcRVA >= expDir.VirtualAddress &&
                funcRVA <  expDir.VirtualAddress + expDir.Size) {
                // funcAddr is a "DllName.FunctionName" string here.
                return NULL; // (or recurse: LoadLibrary + resolve the target)
            }

            return funcAddr;
        }
    }

    return NULL; // not found
}

void ShowErrorMessageAndTerminate(const char* format, const char* funcName)
{
    char buffer[256];
    wsprintfA(buffer, format, funcName);
    MessageBoxA(NULL, buffer, "Error", MB_OK | MB_ICONERROR);
    TerminateProcess(GetCurrentProcess(), 1);
}

BOOL HookFunction(HMODULE hModule, const char* funcName, BYTE* origFuncEntryBytes,
    SIZE_T origFuncBufferLength, void** origFuncPtr, void* destHookFuncPtr, BOOL terminateOnFail)
{
    DWORD oldProtect;
    const DWORD jmpLength = 5;

    BYTE* targetFunc = CustomGetProcAddress(hModule, funcName);
    if (!targetFunc)
    {
        if (terminateOnFail)
            ShowErrorMessageAndTerminate("Bad driver. GetProcAddress failed for function %s", funcName);
        return FALSE;
    }

    VirtualProtect(targetFunc, origFuncBufferLength, PAGE_EXECUTE_READWRITE, &oldProtect);

    LogToFile("Function %s signature:", funcName);
    LogBufferToFile("Buffer:", (void*)targetFunc, 10);

    SIZE_T i;
    for (i = 0; i < origFuncBufferLength; i++)
    {
        if (origFuncEntryBytes[i] != 0xFF && origFuncEntryBytes[i] != targetFunc[i])
        {
            if (terminateOnFail)
                ShowErrorMessageAndTerminate("Bad driver. %s signature mismatch", funcName);
            return FALSE;
        }
    }

    BYTE* trampoline = (BYTE*)VirtualAlloc(NULL, origFuncBufferLength + jmpLength, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (origFuncPtr) *origFuncPtr = (void*)trampoline;
    memcpy(trampoline, targetFunc, origFuncBufferLength);

    if ((trampoline[0] == 0xE9 || trampoline[0] == 0xE8) && origFuncBufferLength >= 5)
    {
        DWORD origRel = *(DWORD*)(trampoline + 1);
        BYTE* origTarget = targetFunc + 5 + (LONG)origRel;
        DWORD newRel = origTarget - (trampoline + 5);
        *(DWORD*)(trampoline + 1) = newRel;
    }

    DWORD jmpBackAddr = (DWORD)(targetFunc + origFuncBufferLength);
    DWORD relJmpBack = jmpBackAddr - ((DWORD)(trampoline + origFuncBufferLength) + 5);
    trampoline[origFuncBufferLength] = 0xE9;
    *(DWORD*)(trampoline + origFuncBufferLength + 1) = relJmpBack;

    DWORD tmpProtect;
    VirtualProtect(trampoline, origFuncBufferLength + jmpLength, PAGE_EXECUTE_READWRITE, &tmpProtect);

    DWORD relJmpHook = (DWORD)destHookFuncPtr - ((DWORD)targetFunc + 5);
    targetFunc[0] = 0xE9;
    *(DWORD*)(targetFunc + 1) = relJmpHook;
    for (i = 5; i < origFuncBufferLength; i++) targetFunc[i] = 0x90;

    VirtualProtect(targetFunc, origFuncBufferLength, oldProtect, &oldProtect);
    return TRUE;
}

void HookFunctionWithFallback(HMODULE hModule, const char* funcName, FuncEntrySignature funcSigs[],
    SIZE_T totalSigsSize, void** origFuncPtr, void* destHookFuncPtr)
{
    int sigsCount = totalSigsSize / sizeof(funcSigs[0]);
    for (int i = 0; i < sigsCount; i++)
    {
        BOOL success = HookFunction(hModule, funcName, funcSigs[i].bytes, funcSigs[i].size, origFuncPtr, destHookFuncPtr, i == sigsCount - 1);
        if (success) break;
    }
}

BOOL HookFunctionWithFallbackSafe(HMODULE hModule, const char* funcName, FuncEntrySignature funcSigs[],
    SIZE_T totalSigsSize, void** origFuncPtr, void* destHookFuncPtr, BOOL errorOnFail)
{
    int sigsCount = totalSigsSize / sizeof(funcSigs[0]);
    for (int i = 0; i < sigsCount; i++)
    {
        BOOL success = HookFunction(hModule, funcName, funcSigs[i].bytes, funcSigs[i].size, origFuncPtr, destHookFuncPtr, errorOnFail && i == sigsCount - 1);
        if (success) return TRUE;
    }

    return FALSE;
}