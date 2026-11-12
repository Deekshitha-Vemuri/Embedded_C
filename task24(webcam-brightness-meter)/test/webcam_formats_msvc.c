#define WINVER 0x0A00
#define _WIN32_WINNT 0x0A00
#define COBJMACROS

#include <stdio.h>
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>

/*
 * MSVC x64 build:
 * cl /TC webcam_formats_msvc.c /link mf.lib mfreadwrite.lib mfplat.lib mfuuid.lib ole32.lib uuid.lib
 */

static void PrintGUID(const GUID *guid)
{
    printf(
        "%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
        (unsigned long)guid->Data1,
        guid->Data2,
        guid->Data3,
        guid->Data4[0],
        guid->Data4[1],
        guid->Data4[2],
        guid->Data4[3],
        guid->Data4[4],
        guid->Data4[5],
        guid->Data4[6],
        guid->Data4[7]
    );
}

int main(void)
{
    HRESULT hr;
    BOOL comInitialized = FALSE;
    BOOL mfInitialized = FALSE;

    IMFAttributes *pAttributes = NULL;
    IMFActivate **ppDevices = NULL;
    IMFMediaSource *pSource = NULL;
    IUnknown *pUnknown = NULL;
    IMFSourceReader *pReader = NULL;
    IMFMediaType *pType = NULL;

    UINT32 deviceCount = 0;
    UINT32 width, height;
    UINT32 numerator, denominator;
    GUID subtype;

    printf("=== Webcam Native Format Inspector ===\n\n");

    /* 1. Initialize COM */
    printf("[1] Initializing COM...\n");

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (FAILED(hr))
    {
        printf("ERROR: CoInitializeEx failed: 0x%08lX\n",
               (unsigned long)hr);
        goto cleanup;
    }

    comInitialized = TRUE;
    printf("SUCCESS: COM initialized.\n\n");

    /* 2. Initialize Media Foundation */
    printf("[2] Initializing Media Foundation...\n");

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);

    if (FAILED(hr))
    {
        printf("ERROR: MFStartup failed: 0x%08lX\n",
               (unsigned long)hr);
        goto cleanup;
    }

    mfInitialized = TRUE;
    printf("SUCCESS: Media Foundation initialized.\n\n");

    /* 3. Prepare webcam discovery */
    printf("[3] Preparing webcam discovery...\n");

    hr = MFCreateAttributes(&pAttributes, 1);

    if (FAILED(hr))
    {
        printf("ERROR: MFCreateAttributes failed: 0x%08lX\n",
               (unsigned long)hr);
        goto cleanup;
    }

    hr = IMFAttributes_SetGUID(
        pAttributes,
        &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
        &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
    );

    if (FAILED(hr))
    {
        printf("ERROR: SetGUID failed: 0x%08lX\n",
               (unsigned long)hr);
        goto cleanup;
    }

    /* 4. Enumerate webcams */
    printf("[4] Enumerating webcams...\n");

    hr = MFEnumDeviceSources(
        pAttributes,
        &ppDevices,
        &deviceCount
    );

    if (FAILED(hr))
    {
        printf("ERROR: MFEnumDeviceSources failed: 0x%08lX\n",
               (unsigned long)hr);
        goto cleanup;
    }

    printf("SUCCESS: Found %u webcam(s).\n\n", deviceCount);

    if (deviceCount == 0)
    {
        printf("No webcam found.\n");
        goto cleanup;
    }

    /* 5. Activate the first webcam */
    printf("[5] Activating the first webcam...\n");

    hr = IMFActivate_ActivateObject(
        ppDevices[0],
        &IID_IUnknown,
        (void **)&pUnknown
    );

    if (FAILED(hr))
    {
        printf("ERROR: Webcam activation failed: 0x%08lX\n",
               (unsigned long)hr);
        goto cleanup;
    }

    hr = IUnknown_QueryInterface(
        pUnknown,
        &IID_IMFMediaSource,
        (void **)&pSource
    );

    if (FAILED(hr))
    {
        printf("ERROR: IMFMediaSource QueryInterface failed: 0x%08lX\n",
               (unsigned long)hr);
        goto cleanup;
    }

    IUnknown_Release(pUnknown);
    pUnknown = NULL;

    printf("SUCCESS: Webcam activated.\n\n");

    /* 6. Create Source Reader */
    printf("[6] Creating Source Reader...\n");

    hr = MFCreateSourceReaderFromMediaSource(
        pSource,
        NULL,
        &pReader
    );

    if (FAILED(hr))
    {
        printf("ERROR: Source Reader creation failed: 0x%08lX\n",
               (unsigned long)hr);
        goto cleanup;
    }

    printf("SUCCESS: Source Reader created.\n\n");

    /* 7. Enumerate native video media types */
    printf("[7] Enumerating native video formats...\n\n");

    for (DWORD index = 0; ; index++)
    {
        hr = IMFSourceReader_GetNativeMediaType(
            pReader,
            MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            index,
            &pType
        );

        if (hr == MF_E_NO_MORE_TYPES)
        {
            printf("No more native media types.\n");
            break;
        }

        if (FAILED(hr))
        {
            printf("ERROR: GetNativeMediaType failed at index %lu: 0x%08lX\n",
                   (unsigned long)index,
                   (unsigned long)hr);
            break;
        }

        printf("Format #%lu\n", (unsigned long)(index + 1));

        /* Pixel format / subtype */
        hr = IMFMediaType_GetGUID(
            pType,
            &MF_MT_SUBTYPE,
            &subtype
        );

        if (SUCCEEDED(hr))
        {
            printf("  Subtype GUID: ");
            PrintGUID(&subtype);
            printf("\n");
        }
        else
        {
            printf("  Subtype: unavailable (0x%08lX)\n",
                   (unsigned long)hr);
        }

        /* Resolution */
        UINT64 frameSize = 0;

        hr = IMFMediaType_GetUINT64(
            pType,
            &MF_MT_FRAME_SIZE,
            &frameSize
        );

        if (SUCCEEDED(hr))
        {
            width = (UINT32)(frameSize >> 32);
            height = (UINT32)frameSize;

            printf("  Resolution: %u x %u\n", width, height);
        }
        else
        {
            printf("  Resolution: unavailable\n");
        }

        /* Frame rate */
        UINT64 frameRate = 0;

        hr = IMFMediaType_GetUINT64(
            pType,
            &MF_MT_FRAME_RATE,
            &frameRate
        );

        if (SUCCEEDED(hr))
        {
            numerator = (UINT32)(frameRate >> 32);
            denominator = (UINT32)frameRate;

            if (denominator != 0)
            {
                printf("  Frame rate: %u/%u = %.2f FPS\n",
                       numerator,
                       denominator,
                       (double)numerator / denominator);
            }
            else
            {
                printf("  Frame rate: denominator is zero\n");
            }
        }
        else
        {
            printf("  Frame rate: unavailable\n");
        }

        printf("\n");

        IMFMediaType_Release(pType);
        pType = NULL;
    }

cleanup:

    printf("\n[Cleanup] Releasing resources...\n");

    if (pType)
        IMFMediaType_Release(pType);

    if (pReader)
        IMFSourceReader_Release(pReader);

    if (pSource)
    {
        IMFMediaSource_Shutdown(pSource);
        IMFMediaSource_Release(pSource);
    }

    if (pUnknown)
        IUnknown_Release(pUnknown);

    if (ppDevices)
    {
        for (UINT32 i = 0; i < deviceCount; i++)
        {
            if (ppDevices[i])
                IMFActivate_Release(ppDevices[i]);
        }

        CoTaskMemFree(ppDevices);
    }

    if (pAttributes)
        IMFAttributes_Release(pAttributes);

    if (mfInitialized)
        MFShutdown();

    if (comInitialized)
        CoUninitialize();

    printf("[Cleanup] Completed.\n");

    return 0;
}
