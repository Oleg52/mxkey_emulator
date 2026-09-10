#include <windows.h>

#define MAX_PATTERN_SIZE 16

typedef struct FuncEntrySignature
{
    BYTE bytes[MAX_PATTERN_SIZE];
    int size;
} FuncEntrySignature;

BYTE* CustomGetProcAddress(HMODULE hModule, const char* funcName);

BYTE* GetAddressOfEntryPoint(HMODULE hModule);

BOOL IatHookFunction(const char* moduleName, const char* funcName, void** origFuncPtr, void* destHookFuncPtr, BOOL terminateOnFail);

BOOL EatHookFunction(HMODULE hModule, const char* funcName, void** origFuncPtr, void* destHookFuncPtr, BOOL terminateOnFail);

void HookFunctionWithFallback(HMODULE hModule, const char* funcName, FuncEntrySignature funcSigs[],
    SIZE_T totalSigsSize, void** origFuncPtr, void* destHookFuncPtr);

BOOL HookFunctionWithFallbackSafe(HMODULE hModule, const char* funcName, FuncEntrySignature funcSigs[],
    SIZE_T totalSigsSize, void** origFuncPtr, void* destHookFuncPtr, BOOL errorOnFail);