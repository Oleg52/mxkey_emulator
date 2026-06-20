#include <windows.h>
#include <stdio.h>
#include "logging.h"

BOOL SendRequest(const BYTE* request, DWORD requestLen, BYTE* response);

static BOOL g_IsNokiaSessionActive = FALSE;

static const DWORD REQ_HEADER_LEN = 0x5;
static const DWORD MARKER_BUFF_LEN = REQ_HEADER_LEN + 0x8;
static const DWORD REQ1_BUFF_LEN = REQ_HEADER_LEN + 0x50;
static const DWORD REQ2_BUFF_LEN = REQ_HEADER_LEN + 0x30;

static const DWORD REQ1_RESPONSE_LEN = 0x50;
static const DWORD REQ2_RESPONSE_LEN = 0x30;

static const DWORD g_PipeBufferLength = MARKER_BUFF_LEN * 2 + REQ1_BUFF_LEN + REQ2_BUFF_LEN;

static BYTE g_PrevRplRequest[REQ1_BUFF_LEN];

static BYTE g_NokiaMarker1[MARKER_BUFF_LEN];
static BYTE g_NokiaMarker2[MARKER_BUFF_LEN];

void SetNokiaSessionActive(BOOL value)
{
    g_IsNokiaSessionActive = value;
}

void SetNokiaMarker(const BYTE* header, BYTE* body)
{
    BYTE* markerBuffer = !g_IsNokiaSessionActive ? g_NokiaMarker1 : g_NokiaMarker2;
    memcpy(markerBuffer, header, REQ_HEADER_LEN);
    memcpy(markerBuffer + REQ_HEADER_LEN, body, sizeof(g_NokiaMarker1) - REQ_HEADER_LEN);
    markerBuffer[3] = 0x3;
    g_IsNokiaSessionActive = TRUE;
}

BOOL GenerateRplData(const BYTE* header, BYTE* rplRequest, BYTE* response)
{
    BYTE fullRequest[g_PipeBufferLength];
    BYTE fullResponse[g_PipeBufferLength];

    memcpy(g_PrevRplRequest, header, REQ_HEADER_LEN);
    memcpy(g_PrevRplRequest + REQ_HEADER_LEN, rplRequest, sizeof(g_PrevRplRequest) - REQ_HEADER_LEN);

    memcpy(fullRequest, g_NokiaMarker1, sizeof(g_NokiaMarker1));
    memcpy(fullRequest + sizeof(g_NokiaMarker1), g_PrevRplRequest, sizeof(g_PrevRplRequest));

    BOOL result = SendRequest(fullRequest, MARKER_BUFF_LEN + REQ1_BUFF_LEN, fullResponse);
    if (!result) return FALSE;
    memcpy(response, fullResponse, REQ1_RESPONSE_LEN);
    return TRUE;
}

BOOL ParseRplData(const BYTE* header, BYTE* rplRequest, BYTE* response)
{
    BYTE fullRequest[g_PipeBufferLength];
    BYTE fullResponse[g_PipeBufferLength];

    memcpy(fullRequest, g_NokiaMarker1, sizeof(g_NokiaMarker1));
    memcpy(fullRequest + sizeof(g_NokiaMarker1), g_PrevRplRequest, sizeof(g_PrevRplRequest));

    const DWORD req2StartPos = sizeof(g_NokiaMarker1) + sizeof(g_PrevRplRequest);

    memcpy(fullRequest + req2StartPos, g_NokiaMarker2, sizeof(g_NokiaMarker2));
    memcpy(fullRequest + req2StartPos + sizeof(g_NokiaMarker2), header, REQ_HEADER_LEN);
    memcpy(fullRequest + req2StartPos + sizeof(g_NokiaMarker2) + REQ_HEADER_LEN, rplRequest, REQ2_BUFF_LEN - REQ_HEADER_LEN);

    BOOL result = SendRequest(fullRequest, g_PipeBufferLength, fullResponse);
    if (!result) return FALSE;
    memcpy(response, fullResponse + REQ1_RESPONSE_LEN, REQ2_RESPONSE_LEN);
    return TRUE;
}

BOOL SendRequest(const BYTE* request, DWORD requestLen, BYTE* response)
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

    char generatorPath[MAX_PATH];
	GetModuleFileNameA(NULL, generatorPath, MAX_PATH);
    char *appExeNameStart = strrchr(generatorPath, '\\');
    if (appExeNameStart) *appExeNameStart = '\0';
	strcat(generatorPath, "\\tools\\rpl_generator.exe");

    if (!CreateProcessA(NULL, generatorPath, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
    {
        LogToFile("Failed to create proc: %d", GetLastError());
        return FALSE;
    }

    CloseHandle(hStdInRead);
    CloseHandle(hStdOutWrite);

    LogBufferToFile("Full request:", (void*)request, requestLen);

    DWORD written;
    WriteFile(hStdInWrite, request, requestLen, &written, NULL);

    CloseHandle(hStdInWrite); // EOF for child

    DWORD read;
    ReadFile(hStdOutRead, response, g_PipeBufferLength, &read, NULL);

    BYTE failedResponse[20];
    memset(failedResponse, 0xFF, sizeof(failedResponse));
    BOOL success = memcmp(response, failedResponse, sizeof(failedResponse)) != 0;

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(hStdOutRead);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return success;
}