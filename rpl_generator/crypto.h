#include <windows.h>
#include <wincrypt.h>

#pragma comment(lib, "advapi32.lib")

typedef struct _DESKEYBLOB
{
    BLOBHEADER hdr;
    DWORD      dwKeySize;
    BYTE       rgbKeyData[8];
} DESKEYBLOB;

BOOL DesEcbEncrypt(
    const BYTE key[8],
    const BYTE* plaintext,
    DWORD plaintextLen,
    BYTE* ciphertext)
{
    HCRYPTPROV hProv = 0;
    HCRYPTKEY hKey = 0;
    BOOL result = FALSE;

    if ((plaintextLen % 8) != 0)
        return FALSE;

    memcpy(ciphertext, plaintext, plaintextLen);

    if (!CryptAcquireContext(
            &hProv,
            NULL,
            MS_ENHANCED_PROV,
            PROV_RSA_FULL,
            CRYPT_VERIFYCONTEXT))
    {
        goto cleanup;
    }

    DESKEYBLOB blob;

    blob.hdr.bType = PLAINTEXTKEYBLOB;
    blob.hdr.bVersion = CUR_BLOB_VERSION;
    blob.hdr.reserved = 0;
    blob.hdr.aiKeyAlg = CALG_DES;

    blob.dwKeySize = 8;
    memcpy(blob.rgbKeyData, key, 8);

    if (!CryptImportKey(
            hProv,
            (BYTE*)&blob,
            sizeof(blob),
            0,
            0,
            &hKey))
    {
        goto cleanup;
    }

    {
        DWORD mode = CRYPT_MODE_ECB;

        if (!CryptSetKeyParam(
                hKey,
                KP_MODE,
                (BYTE*)&mode,
                0))
        {
            goto cleanup;
        }
    }

    {
        DWORD len = plaintextLen;

        if (!CryptEncrypt(
                hKey,
                0,
                FALSE,
                0,
                ciphertext,
                &len,
                plaintextLen))
        {
            goto cleanup;
        }
    }

    result = TRUE;

cleanup:

    if (hKey)
        CryptDestroyKey(hKey);

    if (hProv)
        CryptReleaseContext(hProv, 0);

    return result;
}

BOOL DesEcbDecrypt(
    const BYTE key[8],
    const BYTE* ciphertext,
    DWORD ciphertextLen,
    BYTE* plaintext)
{
    HCRYPTPROV hProv = 0;
    HCRYPTKEY hKey = 0;
    BOOL result = FALSE;

    if ((ciphertextLen % 8) != 0)
        return FALSE;

    memcpy(plaintext, ciphertext, ciphertextLen);

    if (!CryptAcquireContext(
            &hProv,
            NULL,
            MS_ENHANCED_PROV,
            PROV_RSA_FULL,
            CRYPT_VERIFYCONTEXT))
    {
        goto cleanup;
    }

    DESKEYBLOB blob;

    blob.hdr.bType = PLAINTEXTKEYBLOB;
    blob.hdr.bVersion = CUR_BLOB_VERSION;
    blob.hdr.reserved = 0;
    blob.hdr.aiKeyAlg = CALG_DES;

    blob.dwKeySize = 8;
    memcpy(blob.rgbKeyData, key, 8);

    if (!CryptImportKey(
            hProv,
            (BYTE*)&blob,
            sizeof(blob),
            0,
            0,
            &hKey))
    {
        goto cleanup;
    }

    {
        DWORD mode = CRYPT_MODE_ECB;

        if (!CryptSetKeyParam(
                hKey,
                KP_MODE,
                (BYTE*)&mode,
                0))
        {
            goto cleanup;
        }
    }

    {
        DWORD len = ciphertextLen;

        if (!CryptDecrypt(
                hKey,
                0,
                FALSE,
                0,
                plaintext,
                &len))
        {
            goto cleanup;
        }
    }

    result = TRUE;

cleanup:

    if (hKey)
        CryptDestroyKey(hKey);

    if (hProv)
        CryptReleaseContext(hProv, 0);

    return result;
}