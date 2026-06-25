#include <windows.h>
#include <stdio.h>
#include "logging.h"

WORD WriteStringToBuffer(BYTE* buffer, const char* value, BOOL isDoubleTerminated)
{
    WORD length = 0;
    if (value)
    {
        BOOL foundFirstNull = FALSE;
        for (unsigned int i = 0;;i++)
        {
            if (value[i] != 0)
            {
                foundFirstNull = FALSE;
                continue;
            }

            if (!isDoubleTerminated || (isDoubleTerminated && foundFirstNull))
            {
                length = i;
                break;
            }

            foundFirstNull = TRUE;
        }
    }

    *((WORD*)buffer) = length;
    memcpy(buffer + 2, value, length);
    return 2 + length;
}

BOOL CustomFileDialog(LPOPENFILENAMEA lpOpenFileName, BOOL isSaveFileDialog)
{
    SECURITY_ATTRIBUTES sa;
    HANDLE hStdInRead, hStdInWrite;
    HANDLE hStdOutRead, hStdOutWrite;

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    CreatePipe(&hStdInRead, &hStdInWrite, &sa, 0);
    CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0);

    SetHandleInformation(hStdInWrite, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));

    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput  = hStdInRead;
    si.hStdOutput = hStdOutWrite;
    si.hStdError  = hStdOutWrite;

    char generatorDir[MAX_PATH];
    char generatorPath[MAX_PATH];
	GetModuleFileNameA(NULL, generatorPath, MAX_PATH);
    char *appExeNameStart = strrchr(generatorPath, '\\');
    if (appExeNameStart) *appExeNameStart = '\0';

	strcpy(generatorDir, generatorPath);
	strcat(generatorDir, "\\tools");
	strcat(generatorPath, "\\tools\\save_helper.exe");

    if (!CreateProcessA(NULL, generatorPath, NULL, NULL, TRUE, 0, NULL, generatorDir, &si, &pi))
    {
        LogToFile("Failed to create save helper proc: %d", GetLastError());
        return FALSE;
    }

    CloseHandle(hStdInRead);
    CloseHandle(hStdOutWrite);

    DWORD written;
    BYTE buffer[2048];

    buffer[0] = isSaveFileDialog;
    lpOpenFileName->Flags &= ~(OFN_ENABLEHOOK | OFN_ENABLETEMPLATE | OFN_ENABLETEMPLATEHANDLE | OFN_ENABLEINCLUDENOTIFY);
    lpOpenFileName->Flags |= OFN_EXPLORER;
    *((DWORD*)(buffer + 1)) = lpOpenFileName->Flags;

    WORD bufferLength = 5;
    bufferLength += WriteStringToBuffer(buffer + bufferLength, lpOpenFileName->lpstrInitialDir, FALSE);
    bufferLength += WriteStringToBuffer(buffer + bufferLength, lpOpenFileName->lpstrFile, FALSE);
    bufferLength += WriteStringToBuffer(buffer + bufferLength, lpOpenFileName->lpstrFilter, TRUE);

    LogBufferToFile("File name request", buffer, bufferLength);

    WriteFile(hStdInWrite, buffer, bufferLength, &written, NULL);

    CloseHandle(hStdInWrite); // EOF for child

    DWORD read;
    ReadFile(hStdOutRead, buffer, 1024, &read, NULL);

    LogBufferToFile("File name response", buffer, 1024);

    BOOL success = FALSE;
    if (read > 2)
    {
        WORD responseLength = *((WORD*)buffer);
        success = responseLength > 0;
        if (success)
        {
            buffer[2 + responseLength] = 0;
            strcpy(lpOpenFileName->lpstrFile, (char*)(buffer + 2));
            LogToFile("File name %s", lpOpenFileName->lpstrFile);
        }
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(hStdOutRead);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return success;
}

BOOL CustomGetOpenFileName(LPOPENFILENAMEA lpOpenFileName)
{
    return CustomFileDialog(lpOpenFileName, FALSE);
}

BOOL CustomGetSaveFileName(LPOPENFILENAMEA lpOpenFileName)
{
    return CustomFileDialog(lpOpenFileName, TRUE);
}