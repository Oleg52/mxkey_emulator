#include <windows.h>
#include <stdio.h>
#include <commdlg.h>

#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

WORD ReadStringFromBuffer(BYTE* buffer, char*& result)
{
    WORD length = *((WORD*)buffer);
    char* value = new char[length + 1];
    memcpy(value, buffer + 2, length);
    value[length] = 0;
    result = value;
    return 2 + length;
}

int main()
{
	HANDLE hIn  = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

	BYTE requestBuffer[2048];
	DWORD requestBufferLen = 0;

	if (!ReadFile(hIn, requestBuffer, sizeof(requestBuffer), &requestBufferLen, NULL) || requestBufferLen == 0)
	{
		return 1;
	}

	char* lpstrInitialDir;
	char* lpstrFile;
	char* lpstrFilter;

	BOOL isSaveFileDialog = requestBuffer[0];
	DWORD flags = *((DWORD*)(requestBuffer + 1));

	WORD bufferLength = 5;
	bufferLength += ReadStringFromBuffer(requestBuffer + bufferLength, lpstrInitialDir);
	bufferLength += ReadStringFromBuffer(requestBuffer + bufferLength, lpstrFile);
	bufferLength += ReadStringFromBuffer(requestBuffer + bufferLength, lpstrFilter);

    OPENFILENAMEA openFileName = {0};
	openFileName.lStructSize = sizeof(openFileName);
    openFileName.lpstrInitialDir = lpstrInitialDir;
    openFileName.lpstrFile = lpstrFile;
    openFileName.lpstrFilter = lpstrFilter;
	openFileName.nMaxFile = MAX_PATH;
    openFileName.Flags = flags;

    OleInitialize(NULL);
    BOOL success = isSaveFileDialog
		? GetSaveFileNameA(&openFileName)
		: GetOpenFileNameA(&openFileName);
    OleUninitialize();

	BYTE responseBuffer[1024];
	WORD responseBufferLength = 0;
    if(success)
	{
		responseBufferLength = strlen(openFileName.lpstrFile);
		*((WORD*)responseBuffer) = responseBufferLength;
		memcpy(responseBuffer + 2, openFileName.lpstrFile, responseBufferLength);
		responseBufferLength += 2;
		responseBuffer[responseBufferLength] = 0;
    }

	DWORD written;
	WriteFile(hOut, responseBuffer, responseBufferLength, &written, NULL);
	return 0;
}