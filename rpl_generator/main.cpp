#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <stdio.h>
#include "crypto.h"

#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

typedef unsigned long* SCARDCONTEXT;
typedef unsigned long* SCARDHANDLE;
typedef SCARDHANDLE *PSCARDHANDLE, *LPSCARDHANDLE;
typedef SCARDCONTEXT *PSCARDCONTEXT, *LPSCARDCONTEXT;
typedef const BYTE* LPCBYTE;

typedef struct _SCARD_IO_REQUEST{
    DWORD dwProtocol;
    DWORD cbPciLength;
} SCARD_IO_REQUEST, *PSCARD_IO_REQUEST, *LPSCARD_IO_REQUEST;

typedef const SCARD_IO_REQUEST *LPCSCARD_IO_REQUEST;

typedef LONG (__stdcall *SCardEstablishContext_t)(DWORD, LPCVOID, LPCVOID, LPSCARDCONTEXT);
SCardEstablishContext_t SCardEstablishContextOrigFunc = NULL;

typedef LONG (__stdcall *SCardConnectA_t)(SCARDCONTEXT hContext, LPCSTR szReader, DWORD dwShareMode, DWORD dwPreferredProtocols, LPSCARDHANDLE phCard, LPDWORD pdwActiveProtocol);
SCardConnectA_t SCardConnectAOrigFunc = NULL;

typedef LONG (__stdcall *SCardTransmit_t)(SCARDHANDLE, LPCSCARD_IO_REQUEST, LPCBYTE, DWORD, LPSCARD_IO_REQUEST, LPBYTE, LPDWORD);
SCardTransmit_t SCardTransmitOrigFunc = NULL;

SCardTransmit_t SCardTransmit = NULL;
SCARDHANDLE g_CardHandle = NULL;

static const BYTE g_FixedKey0[] = {0x04,0xE3,0xF1,0x04,0x33,0xD0,0xA8,0xEA};
static const BYTE g_DaniscelDEK[] = {0x9F,0xCE,0x9F,0x68,0x9E,0x2A,0x82,0x10};
static const BYTE g_EncryptedHostChallenge[] = {0x6D,0x6D,0xF1,0x76,0x43,0xC1,0xC5,0x0D};
static BYTE g_RplRequest2DEK[8];
static const DWORD HEADER_LEN = 5;
static const DWORD MARKER_REQ_LEN = 0xD;

BOOL SendTransmitRequest(const BYTE* sendBuf, DWORD sendLen, BYTE* recvBuf, DWORD* recvLen)
{
	LONG result = SCardTransmit(g_CardHandle, NULL, sendBuf, sendLen, NULL, recvBuf, recvLen);
	if (result != 0) return FALSE;

	if (*recvLen == 2 && recvBuf[0] == 0x61)
	{
		BYTE getResponseBuf[] = {0x00,0xC0,0x00,0x00,recvBuf[1]};
		LONG result = SCardTransmit(g_CardHandle, NULL, getResponseBuf, sizeof(getResponseBuf), NULL, recvBuf, recvLen);
		if (result != 0) return FALSE;
	}

	return TRUE;
}

BOOL InitSession()
{
	BYTE recvBuf[100];
	DWORD recvBufLen;

	const BYTE selectNokiaAppletBuf[] = {0x00,0xA4,0x04,0x00,0x08,0xA0,0x00,0x00,0x00,0xC0,0x43,0x4F,0x2E};
	if (!SendTransmitRequest(selectNokiaAppletBuf, sizeof(selectNokiaAppletBuf), recvBuf, &recvBufLen))
	{
		return FALSE;
	}

	const BYTE initUpdBuf[] = {0x80,0x50,0x00,0x00,0x08,0x95,0x69,0xDE,0xA3,0x7A,0x6A,0x15,0x7A};
	if (!SendTransmitRequest(initUpdBuf, sizeof(initUpdBuf), recvBuf, &recvBufLen))
	{
		return FALSE;
	}

	const BYTE handshakeBuf[] = {0x84,0x82,0x03,0x00,0x10,0xE6,0x34,0xCA,0x82,0x6C,0xD6,0x8F,0x64,0x6D,0x6D,0xF1,0x76,0x43,0xC1,0xC5,0x0D};
	if (!SendTransmitRequest(handshakeBuf, sizeof(handshakeBuf), recvBuf, &recvBufLen))
	{
		return FALSE;
	}

	return TRUE;
}

BOOL SendMarkerRequest(BYTE* request)
{
	BYTE* bufferToEncrypt = request + HEADER_LEN;

	DesEcbEncrypt(g_EncryptedHostChallenge, bufferToEncrypt, MARKER_REQ_LEN - HEADER_LEN, bufferToEncrypt);
    DesEcbEncrypt(g_FixedKey0, bufferToEncrypt, MARKER_REQ_LEN - HEADER_LEN, bufferToEncrypt);

	BYTE response[20];
	DWORD responseLen;
	return SendTransmitRequest(request, MARKER_REQ_LEN, response, &responseLen);
}

BOOL SendRplRequest(BYTE* request, DWORD requestLen, BYTE* response, BOOL isSecondRequest)
{
	BYTE* bufferToEncrypt = request + HEADER_LEN;

	DesEcbEncrypt(g_EncryptedHostChallenge, bufferToEncrypt, requestLen - HEADER_LEN, bufferToEncrypt);
    DesEcbEncrypt(g_FixedKey0, bufferToEncrypt, requestLen - HEADER_LEN, bufferToEncrypt);

	DWORD responseLen;
	if (!SendTransmitRequest(request, requestLen, response, &responseLen))
	{
		return FALSE;
	}

	responseLen -= 2;
	// for 2nd request, last 8 bytes are dek key
	if (!isSecondRequest)
		memcpy(g_RplRequest2DEK, response + responseLen - sizeof(g_RplRequest2DEK), sizeof(g_RplRequest2DEK));
	
	const BYTE* dek = isSecondRequest ? g_RplRequest2DEK : g_DaniscelDEK;
	DesEcbDecrypt(g_FixedKey0, response, responseLen, response);
	DesEcbDecrypt(dek, response, responseLen, response);
	return TRUE;
}

void WriteFailedReponse(HANDLE hOut)
{
	DWORD written;
	BYTE failedReponse[20];
	memset(failedReponse, 0xFF, sizeof(failedReponse));
	WriteFile(hOut, failedReponse, sizeof(failedReponse), &written, NULL);
}

int main()
{
	HMODULE hDani = LoadLibraryA("daniscel.dll");
	SCardEstablishContext_t establishContext = (SCardEstablishContext_t)GetProcAddress(hDani, "SCardEstablishContext");
	SCardConnectA_t cardConnect = (SCardConnectA_t)GetProcAddress(hDani, "SCardConnectA");
	SCardTransmit = (SCardTransmit_t)GetProcAddress(hDani, "SCardTransmit");

	SCARDCONTEXT cardContext = 0;
	establishContext(0,0,0, &cardContext);

	DWORD activeProto;
	cardConnect(cardContext, 0, 0, 0, &g_CardHandle, &activeProto);

	HANDLE hIn  = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

	BYTE requestBuffer[256];
	DWORD requestBufferLen = 0;

	if (!ReadFile(hIn, requestBuffer, sizeof(requestBuffer), &requestBufferLen, NULL) || requestBufferLen == 0)
	{
		return 1;
	}

	InitSession();

	const DWORD RPL1_REQ_LEN = 0x55;
	const DWORD RPL2_REQ_LEN = 0x35;
	const DWORD RPL1_RESP_LEN = 0x50;
	const DWORD RPL2_RESP_LEN = 0x30;

	BYTE responseBuffer[RPL1_RESP_LEN + RPL2_RESP_LEN + 2];

	BOOL success = SendMarkerRequest(requestBuffer);
	success = success && SendRplRequest(requestBuffer + MARKER_REQ_LEN, RPL1_REQ_LEN, responseBuffer, FALSE);
	if (!success)
	{
		WriteFailedReponse(hOut);
		return 1;
	}

	BOOL hasRplRequest2 = requestBufferLen > MARKER_REQ_LEN + RPL1_REQ_LEN;
	if (hasRplRequest2)
	{
		success = SendMarkerRequest(requestBuffer + MARKER_REQ_LEN + RPL1_REQ_LEN);
		success = success && SendRplRequest(requestBuffer + MARKER_REQ_LEN + RPL1_REQ_LEN + MARKER_REQ_LEN, RPL2_REQ_LEN, responseBuffer + RPL1_RESP_LEN, TRUE);
	}

	if (!success)
	{
		WriteFailedReponse(hOut);
		return 1;
	}

	DWORD written;
	WriteFile(hOut, responseBuffer, sizeof(responseBuffer) - 2, &written, NULL);
	return 0;
}