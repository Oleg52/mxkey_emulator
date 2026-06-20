#include <windows.h>
#include "logging.h"
#include "crypto.h"
#include "rpl_helper.h"

// ------------------------------------ ATTRIBUTES ----------------------------------------
#define MAX_ATTR_ENTRY_DATA 20
typedef struct _ATTR_ENTRY
{
    DWORD id;
    BYTE data[MAX_ATTR_ENTRY_DATA];
    DWORD size;
} AttributeEntry;

static const AttributeEntry g_CardAttrbiutesDict[] =
{
    { 0x90303, {0x3B,0xEA,0x00,0x00,0x81,0x31,0xFE,0x45,0x52,0x00,0x20,0xD6,0x59,0x00,0x00,0x00,0x00,0x00,0x1C}, 19 },
    { 0x10100, {0x47,0x65,0x6E,0x65,0x72,0x69,0x63}, 7 },
    { 0x10103, {0x8A}, 1 },
    { 0x80201, {0x02,0,0,0}, 4 },
    { 0x90304, {1}, 1 },
};

static const DWORD g_CardAttrbiutesDictLength = sizeof(g_CardAttrbiutesDict) / sizeof(g_CardAttrbiutesDict[0]);

const AttributeEntry* GetAttributeValue(DWORD id)
{
    for (int i = 0; i < g_CardAttrbiutesDictLength; i++)
    {
        if (g_CardAttrbiutesDict[i].id == id)
        {
            return &g_CardAttrbiutesDict[i];
        }
    }

    return NULL;
}
// ------------------------------------ ATTRIBUTES ----------------------------------------

// -------------------------------------- TRANSMIT ----------------------------------------
#define MAX_TRANSMIT_ENTRY_DATA 100
typedef struct _TRANSMIT_ENTRY
{
    BYTE sendBuf[MAX_TRANSMIT_ENTRY_DATA];
    BYTE recvBuf[MAX_TRANSMIT_ENTRY_DATA];
    DWORD recvSize;
} TransmitEntry;

static const DWORD CARD_SERIAL_OFFSET = 10;
static BYTE g_CardInfo[] = {0x4D,0x58,0x4B,0x45,0x59,0x00,0x00,0x00,0x00,0x20,0x6B,0x98,0x4D,0x60,0xAA,0xAA,0xAA,0xAA,0x3C,0x00,0x3C,0x00,0x6B,0x98,0x4D,0x60,0x61,0x6E,0x64,0x72,0x65,0x79,0x2D,0x37,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x10,0x03,0x10,0x01};

static const DWORD RAW_INIT_UPDATE_RESPONSE_LENGTH = 16;
static BYTE g_InitUpdateRawResponses[][RAW_INIT_UPDATE_RESPONSE_LENGTH] =
{
    { 0x5D,0x01,0x38,0x7C,0x2F,0x21,0xD2,0x22,0x12,0x93,0xD9,0x03,0x15,0x08,0x95,0xDE },
    { 0xBC,0x63,0x77,0x23,0xD1,0x15,0x86,0x24,0x44,0x48,0xF8,0xEA,0xC0,0x4A,0x06,0x87 },
    { 0x08,0x11,0x62,0xFC,0xA3,0x03,0x4F,0xAD,0xB8,0x0F,0x6A,0x3C,0xC0,0x26,0xE6,0xC7 },
    { 0x7C,0xC2,0x96,0xA5,0x41,0x8A,0xD6,0xB8,0x93,0x5C,0x5F,0x92,0xB3,0x7C,0x55,0xF5 },
};
static const DWORD g_InitUpdateRawResponsesLenght = sizeof(g_InitUpdateRawResponses) / sizeof(g_InitUpdateRawResponses[0]);
static DWORD g_ActiveInitUpdateRawResponseIndex = 0;

static const DWORD INIT_UPDATE_SEQ_CHALLENGE_OFFSET = 12;
static BYTE g_ActiveInitUpdateResponse[] = {0x00,0x00,0x73,0x04,0x00,0x30,0x19,0x91,0x28,0x28,0xFF,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

static const BYTE g_FixedKey0[] = {0x04,0xE3,0xF1,0x04,0x33,0xD0,0xA8,0xEA};
static const BYTE g_FixedKey1[] = {0x6C,0x3A,0x0D,0x2E,0xC0,0x23,0x43,0x9A};
static const BYTE g_FixedKey2[] = {0xE3,0xF1,0xC4,0x63,0x6A,0xAC,0x04,0xD2};
static const BYTE g_FixedKey3[] = {0x07,0x63,0xA5,0xDB,0x45,0x96,0x19,0x2D};

static const BYTE g_C4_30_ConstKey[] = {0x06,0xB7,0x8B,0x5B,0xDC,0xDC,0xBC,0xC1};
// "MXKEY\0\0\0" — plaintext encrypted in the C4 2B 01 03 response
static const BYTE g_MxkeyPlain[] = {0x4D,0x58,0x4B,0x45,0x59,0x00,0x00,0x00};

static const BYTE g_ActiveLicenseRawResponse[] = {0x55,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const BYTE g_NokiaAppletVersionRawResponse[] = {0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00};
static const BYTE g_CardVersionRawResponse[] = {0x01,0x00,0x20,0x10,0x03,0x10,0x00,0x00};

static BYTE g_HostChallenge[8];
static BYTE g_EncryptedHostChallenge[8];
static BYTE g_DEK_Key[8];   // next C4 requests use last 8 bytes as key
static BYTE g_FuzzyAuthKey[8]; // for C4 2C

static const BYTE g_InitUpdateStart[] = {0x80,0x50,0x00,0x00,0x08};
static const BYTE g_ExternalAuthStart[] = {0x84,0x82,0x03,0x00,0x10};
static const BYTE g_GetMxkeyAuthKeyStart[] = {0xC4,0x2B,0x01,0x03};
static const BYTE g_GetSerialAuthKeyStart[] = {0xC4,0x2B,0x00,0x03};
static const BYTE g_GetFuzzyAuthKeyStart[] = {0xC4,0x2B,0x00,0x00};
static DWORD g_GetFuzzyAuthKeyCalls = 0;
static const BYTE g_GetCardInfoStart[] = {0xC4,0x2C,0x4F};
static const BYTE g_ValidateAuthStart1[] = {0xC4,0x30};
static const BYTE g_ValidateAuthStart2[] = {0xC4,0x31};
static const BYTE g_LicenseCheckStart[] = {0xC4,0x2E};
static const BYTE g_CardVersionStart[] = {0xC4,0x32};
static const BYTE g_UpdateBinaryStart[] = {0x80,0xD6};
static const BYTE g_NokiaAppletVersionStart[] = {0x44,0x2A};
static const BYTE g_NokiaRlpMarkerRequestStart[] = {0x44,0x2C};
static const BYTE g_NokiaCalculateRplStart[] = {0x44,0x2D};

// responses that do not change
static TransmitEntry g_TransmitDataDictResps[] =
{
    { {0x00,0xA4,0x04,0x00,0x07,0xA0,0x00,0x00,0x00,0x03,0x00,0x00}, {0x6F,0x10,0x84,0x08,0xA0,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0xA5,0x04,0x9F,0x65,0x01,0xFF,0x90,0x00}, 20 },
    { {0x00,0xA4,0x04,0x00,0x08,0xA0,0x00,0x00,0x00,0x03,0x00,0x00,0x00}, {0x6F,0x10,0x84,0x08,0xA0,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0xA5,0x04,0x9F,0x65,0x01,0xFF,0x90,0x00}, 20 },
    { {0x00,0xA4,0x04,0x00,0x06,0x7C,0x4D,0x58,0x4B,0x45,0x59}, {0x6E,0x00}, 2 },
    { {0x00,0xA4,0x04,0x00,0x08,0xA0,0x00,0x00,0x00,0xC0,0x43,0x4F,0x2E}, {0x90,0x00}, 2 },
    { {0x80,0xCA,0x9F,0x7F,0x00}, {0x9F,0x7F,0x2A,0x47,0x90,0x51,0x06,0x40,0x51,0x52,0x99,0x25,0x00,0x83,0x17,0x02,0x58,0x94,0x91,0x44,0x49,0x48,0x10,0x83,0x24,0x00,0x00,0x00,0x00,0x15,0x28,0x57,0x32,0x35,0x38,0x39,0x34,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x90,0x00}, 47 },
    { {0x00,0xA4,0x04,0x00,0x08,0xA0,0x00,0x00,0x00,0xC1,0x4E,0x4B,0x2E}, {0x90,0x00}, 2 }, // select nokia applet
    { {0x00,0xA4,0x04,0x00,0x08,0xA0,0x00,0x00,0x00,0xC1,0x43,0x49,0x2E}, {0x90,0x00}, 2 }  // select ccs tool applet
};
static const DWORD g_TransmitDataDictRespsLength = sizeof(g_TransmitDataDictResps) / sizeof(g_TransmitDataDictResps[0]);

TransmitEntry* TryFindTransmitResponse(const BYTE* sendBuf, DWORD length)
{
    for (unsigned int i = 0; i < g_TransmitDataDictRespsLength; i++)
    {
        if (memcmp(g_TransmitDataDictResps[i].sendBuf, sendBuf, length) == 0)
        {
            return &g_TransmitDataDictResps[i];
        }
    }

    return NULL;
}

BOOL HandleTransmitRequest(const BYTE* request, DWORD requestLength, BYTE* response, DWORD* responseLenth)
{
    TransmitEntry* resp = TryFindTransmitResponse(request, requestLength);
    if (resp != NULL)
    {
        *responseLenth = resp->recvSize;
        memcpy(response, resp->recvBuf, *responseLenth);
        return TRUE;
    }

    if (memcmp(request, g_InitUpdateStart, sizeof(g_InitUpdateStart)) == 0) // SCP02 init
    {
        /*memcpy(g_HostChallenge, request + 5, sizeof(g_HostChallenge));
        // calculate encrypted chal, or grab it from SCP02 handshake
        DesEcbDecrypt(g_FixedKey0, g_HostChallenge, sizeof(g_HostChallenge), g_EncryptedHostChallenge);
        DesEcbDecrypt(g_FixedKey1, g_EncryptedHostChallenge, sizeof(g_EncryptedHostChallenge), g_EncryptedHostChallenge);
        DesEcbEncrypt(g_FixedKey3, g_EncryptedHostChallenge, sizeof(g_EncryptedHostChallenge), g_EncryptedHostChallenge);
        DesEcbDecrypt(g_FixedKey2, g_EncryptedHostChallenge, sizeof(g_EncryptedHostChallenge), g_EncryptedHostChallenge);*/

        if (g_ActiveInitUpdateRawResponseIndex >= g_InitUpdateRawResponsesLenght)
        {
            g_ActiveInitUpdateRawResponseIndex = 0;
        }

        DesEcbDecrypt(g_FixedKey3, g_InitUpdateRawResponses[g_ActiveInitUpdateRawResponseIndex], RAW_INIT_UPDATE_RESPONSE_LENGTH, response);
        DesEcbEncrypt(g_FixedKey1, response, RAW_INIT_UPDATE_RESPONSE_LENGTH, g_ActiveInitUpdateResponse + INIT_UPDATE_SEQ_CHALLENGE_OFFSET);

        *responseLenth = sizeof(g_ActiveInitUpdateResponse);
        memcpy(response, g_ActiveInitUpdateResponse, *responseLenth);
        g_ActiveInitUpdateRawResponseIndex++;
        SetNokiaSessionActive(FALSE);
        goto SET_SUCCESS_RESPONSE;
    }
    if (memcmp(request, g_ExternalAuthStart, sizeof(g_ExternalAuthStart)) == 0) // SCP02 handshake
    {
        DesEcbDecrypt(g_FixedKey1, g_ActiveInitUpdateResponse + INIT_UPDATE_SEQ_CHALLENGE_OFFSET, sizeof(g_DEK_Key), g_DEK_Key);
        DesEcbEncrypt(g_FixedKey3, g_DEK_Key, sizeof(g_DEK_Key), g_DEK_Key);
        DesEcbDecrypt(g_FixedKey2, g_DEK_Key, sizeof(g_DEK_Key), g_DEK_Key);

        *responseLenth = 0;
        memcpy(g_EncryptedHostChallenge, request + 13, sizeof(g_EncryptedHostChallenge));
        goto SET_SUCCESS_RESPONSE;
    }
    if (memcmp(request, g_UpdateBinaryStart, sizeof(g_UpdateBinaryStart)) == 0)
    {
        *(WORD*)(response) = 0x6D; // respond with not supported
        *responseLenth = 2;
        return TRUE;
    }
    if (memcmp(request, g_GetMxkeyAuthKeyStart, sizeof(g_GetMxkeyAuthKeyStart)) == 0)
    {
        *responseLenth = sizeof(g_MxkeyPlain);
        DesEcbEncrypt(g_DEK_Key, g_MxkeyPlain, *responseLenth, response);
        DesEcbEncrypt(g_FixedKey0, response, *responseLenth, response);
        goto SET_SUCCESS_RESPONSE;
    }
    if (memcmp(request, g_GetSerialAuthKeyStart, sizeof(g_GetSerialAuthKeyStart)) == 0)
    {
        static BYTE buffer[8];
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, g_CardInfo + CARD_SERIAL_OFFSET, 4);

        *responseLenth = sizeof(buffer);
        DesEcbEncrypt(g_DEK_Key, buffer, *responseLenth, response);
        DesEcbEncrypt(g_FixedKey0, response, *responseLenth, response);

        memcpy(g_DEK_Key, response, sizeof(g_DEK_Key));
        goto SET_SUCCESS_RESPONSE;
    }
    if (memcmp(request, g_GetFuzzyAuthKeyStart, sizeof(g_GetFuzzyAuthKeyStart)) == 0)
    {
        static BYTE buffer[8];
        if (g_GetFuzzyAuthKeyCalls <= 1)
        {
            // some keys that are returned only on app startup, then it uses serial
            static const BYTE key1[] = {0xF1,0xED,0x1E,0x4C,0x47,0xFB,0xA2,0x8D};
            static const BYTE key2[] = {0x90,0x3E,0x39,0xEA,0x55,0x90,0xA6,0x54};
            memcpy(buffer, g_GetFuzzyAuthKeyCalls == 0 ? key1 : key2, sizeof(key2));
            g_GetFuzzyAuthKeyCalls++;
        }
        else
        {
            memset(buffer, 0, sizeof(buffer));
            memcpy(buffer, g_CardInfo + CARD_SERIAL_OFFSET, 4);
        }

        *responseLenth = sizeof(buffer);
        DesEcbEncrypt(g_DEK_Key, buffer, *responseLenth, response);
        DesEcbEncrypt(g_FixedKey0, response, *responseLenth, response);
        memcpy(g_FuzzyAuthKey, response, sizeof(g_FuzzyAuthKey));
        goto SET_SUCCESS_RESPONSE;
    }
    if (memcmp(request, g_GetCardInfoStart, sizeof(g_GetCardInfoStart)) == 0) // card info(counter, name, date...)
    {
        *responseLenth = sizeof(g_CardInfo);

        const BYTE* innerKey = (request[3] == 0x03) ? g_DEK_Key         // C4 2C 4F 03
                                                    : g_FuzzyAuthKey;   // C4 2C 4F 00

        DesEcbEncrypt(innerKey, g_CardInfo, *responseLenth, response);
        DesEcbEncrypt(g_FixedKey0, response, *responseLenth, response);

        memcpy(g_DEK_Key, response + *responseLenth - sizeof(g_DEK_Key), sizeof(g_DEK_Key));
        goto SET_SUCCESS_RESPONSE;
    }
    if (memcmp(request, g_ValidateAuthStart1, sizeof(g_ValidateAuthStart1)) == 0 ||
        memcmp(request, g_ValidateAuthStart2, sizeof(g_ValidateAuthStart2)) == 0) // re-encrypt request
    {
        *responseLenth = request[4];

        DesEcbDecrypt(g_FixedKey0, request + 5, *responseLenth, response);
        DesEcbDecrypt(g_EncryptedHostChallenge, response, *responseLenth, response);
        DesEcbDecrypt(g_C4_30_ConstKey, response, *responseLenth, response);

        DesEcbEncrypt(g_DEK_Key, response, *responseLenth, response);
        DesEcbEncrypt(g_FixedKey0, response, *responseLenth, response);

        memcpy(g_DEK_Key, response + 8, sizeof(g_DEK_Key));
        goto SET_SUCCESS_RESPONSE;
    }
    if (memcmp(request, g_LicenseCheckStart, sizeof(g_LicenseCheckStart)) == 0) // 0,1,2,3 licenses
    {
        *responseLenth = sizeof(g_ActiveLicenseRawResponse);
        DesEcbEncrypt(g_DEK_Key, g_ActiveLicenseRawResponse, *responseLenth, response);
        DesEcbEncrypt(g_FixedKey0, response, *responseLenth, response);

        memcpy(g_DEK_Key, response, sizeof(g_DEK_Key));
        goto SET_SUCCESS_RESPONSE;
    }
    if (memcmp(request, g_CardVersionStart, sizeof(g_CardVersionStart)) == 0)
    {
        *responseLenth = sizeof(g_CardVersionRawResponse);
        DesEcbEncrypt(g_DEK_Key, g_CardVersionRawResponse, *responseLenth, response);
        DesEcbEncrypt(g_FixedKey0, response, *responseLenth, response);

        memcpy(g_DEK_Key, response, sizeof(g_DEK_Key));
        goto SET_SUCCESS_RESPONSE;
    }
    if (memcmp(request, g_NokiaAppletVersionStart, sizeof(g_NokiaAppletVersionStart)) == 0)
    {
        *responseLenth = sizeof(g_NokiaAppletVersionRawResponse);
        DesEcbEncrypt(g_DEK_Key, g_NokiaAppletVersionRawResponse, *responseLenth, response);
        DesEcbEncrypt(g_FixedKey0, response, *responseLenth, response);
        goto SET_SUCCESS_RESPONSE;
    }
    if (memcmp(request, g_NokiaRlpMarkerRequestStart, sizeof(g_NokiaRlpMarkerRequestStart)) == 0)
    {
        DesEcbDecrypt(g_FixedKey0, request + 5, *responseLenth, response);
        DesEcbDecrypt(g_EncryptedHostChallenge, response, *responseLenth, response);
        SetNokiaMarker(request, response);
        *responseLenth = 0;
        goto SET_SUCCESS_RESPONSE;
    }
    if (memcmp(request, g_NokiaCalculateRplStart, sizeof(g_NokiaCalculateRplStart)) == 0)
    {
        BYTE nokiaResponse[0x50 + 0x30];
        *responseLenth = request[4];
        DesEcbDecrypt(g_FixedKey0, request + 5, *responseLenth, response);
        DesEcbDecrypt(g_EncryptedHostChallenge, response, *responseLenth, response);
        BOOL success = FALSE;

        if (*responseLenth == 0x50) // calculate DCT4 RPL DATA1
        {
            success = GenerateRplData(request, response, nokiaResponse);
        }
        else if (*responseLenth == 0x30) // parse DCT4 RPL DATA1
        {
            success = ParseRplData(request, response, nokiaResponse);
        }

        if (!success)
        {
            return FALSE;
        }

        LogBufferToFile("Raw response", nokiaResponse, *responseLenth);

        DesEcbEncrypt(g_DEK_Key, nokiaResponse, *responseLenth, response);
        DesEcbEncrypt(g_FixedKey0, response, *responseLenth, response);

        memcpy(g_DEK_Key, response + *responseLenth - sizeof(g_DEK_Key), sizeof(g_DEK_Key));
        goto SET_SUCCESS_RESPONSE;
    }

    return FALSE;

SET_SUCCESS_RESPONSE:
    *(WORD*)(response + *responseLenth) = 0x90;
    *responseLenth += 2;
    return TRUE;
}

// -------------------------------------- TRANSMIT ----------------------------------------