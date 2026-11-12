//build:commandcl /TC /std:c11 webcam_default_test.c /link mf.lib mfplat.lib mfreadwrite.lib mfuuid.lib ole32.lib
#define WINVER 0x0A00
#define _WIN32_WINNT 0x0A00
#define COBJMACROS

/* printf() for diagnostic messages. */
#include <stdio.h>

/* Windows types and HRESULT handling. */
#include <windows.h>

/* COM initialization and cleanup. */
#include <objbase.h>

/* MFStartup(), MFShutdown(), MFCreateAttributes(). */
#include <mfapi.h>

/* Webcam activation and media source interfaces. */
#include <mfidl.h>

/* Media type and sample interfaces. */
#include <mfobjects.h>

/* Source Reader creation and ReadSample(). */
#include <mfreadwrite.h>

/* Media Foundation error and stream definitions. */
#include <mferror.h>

int main(void)
{
    HRESULT hr = S_OK;
    int result = 1;
    BOOL comStarted = FALSE;
    BOOL mfStarted = FALSE;

    IMFAttributes *attributes = NULL;
    IMFActivate **devices = NULL;
    IMFMediaSource *source = NULL;
    IMFSourceReader *reader = NULL;
    IMFMediaType *mediaType = NULL;
    IMFSample *sample = NULL;

    UINT32 deviceCount = 0;
    DWORD streamIndex = 0;
    DWORD flags = 0;
    LONGLONG timestamp = 0;

    printf("=== WEBCAM DEFAULT FORMAT TEST ===\n\n");

    /* Step 1: Initialize COM. */
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        printf("[ERROR] COM: 0x%08lX\n", (unsigned long)hr);
        goto cleanup;
    }
    comStarted = TRUE;
    printf("[OK] COM initialized.\n");

    /* Step 2: Initialize Media Foundation. */
    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    if (FAILED(hr)) {
        printf("[ERROR] MFStartup: 0x%08lX\n", (unsigned long)hr);
        goto cleanup;
    }
    mfStarted = TRUE;
    printf("[OK] Media Foundation initialized.\n");

    /* Step 3: Search for video capture devices. */
    hr = MFCreateAttributes(&attributes, 1);
    if (FAILED(hr)) {
        printf("[ERROR] MFCreateAttributes: 0x%08lX\n", (unsigned long)hr);
        goto cleanup;
    }

    hr = IMFAttributes_SetGUID(
        attributes,
        &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
        &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
    );
    if (FAILED(hr)) {
        printf("[ERROR] SetGUID: 0x%08lX\n", (unsigned long)hr);
        goto cleanup;
    }

    hr = MFEnumDeviceSources(attributes, &devices, &deviceCount);
    if (FAILED(hr)) {
        printf("[ERROR] MFEnumDeviceSources: 0x%08lX\n", (unsigned long)hr);
        goto cleanup;
    }

    printf("[OK] Webcams found: %u\n", deviceCount);

    if (deviceCount == 0) {
        printf("[ERROR] No webcam found.\n");
        goto cleanup;
    }

    /* Step 4: Activate the first webcam. */
    hr = IMFActivate_ActivateObject(
        devices[0],
        &IID_IMFMediaSource,
        (void **)&source
    );
    if (FAILED(hr)) {
        printf("[ERROR] ActivateObject: 0x%08lX\n", (unsigned long)hr);
        goto cleanup;
    }
    printf("[OK] Webcam activated.\n");
    /* Diagnostic: Inspect the webcam presentation descriptor. */

IMFPresentationDescriptor *pPD = NULL;
DWORD streamCount = 0;

hr = IMFMediaSource_CreatePresentationDescriptor(
    source,
    &pPD
);

printf("[DEBUG] CreatePresentationDescriptor: 0x%08lX\n",
       (unsigned long)hr);

if (SUCCEEDED(hr))
{
    hr = IMFPresentationDescriptor_GetStreamDescriptorCount(
        pPD,
        &streamCount
    );

    printf("[DEBUG] Stream count: %lu | HRESULT: 0x%08lX\n",
           (unsigned long)streamCount,
           (unsigned long)hr);

    for (DWORD i = 0; SUCCEEDED(hr) && i < streamCount; i++)
    {
        BOOL selected = FALSE;
        IMFStreamDescriptor *pSD = NULL;

        hr = IMFPresentationDescriptor_GetStreamDescriptorByIndex(
            pPD,
            i,
            &selected,
            &pSD
        );

        printf("[DEBUG] Stream %lu | Selected: %d | HRESULT: 0x%08lX\n",
               (unsigned long)i,
               selected,
               (unsigned long)hr);

        
if (pSD)
{
    IMFMediaTypeHandler *pHandler = NULL;

    hr = IMFStreamDescriptor_GetMediaTypeHandler(
        pSD,
        &pHandler
    );

    printf("[DEBUG] GetMediaTypeHandler: 0x%08lX\n",
           (unsigned long)hr);

    if (SUCCEEDED(hr))
    {
        DWORD typeCount = 0;

        hr = IMFMediaTypeHandler_GetMediaTypeCount(
            pHandler,
            &typeCount
        );

        printf("[DEBUG] Available media types: %lu\n",
               (unsigned long)typeCount);

        for (DWORD j = 0; SUCCEEDED(hr) && j < typeCount; j++)
        {
            IMFMediaType *pType = NULL;

            hr = IMFMediaTypeHandler_GetMediaTypeByIndex(
                pHandler,
                j,
                &pType
            );

            if (SUCCEEDED(hr))
            {
                GUID subtype;
                UINT64 size = 0;

                IMFMediaType_GetGUID(
                    pType,
                    &MF_MT_SUBTYPE,
                    &subtype
                );

                IMFMediaType_GetUINT64(
                    pType,
                    &MF_MT_FRAME_SIZE,
                    &size
                );

                printf("[DEBUG] Type %lu: GUID=%08lX, "
                       "Resolution=%u x %u\n",
                       (unsigned long)j,
                       (unsigned long)subtype.Data1,
                       (UINT32)(size >> 32),
                       (UINT32)size);

                IMFMediaType_Release(pType);
            }
        }

        IMFMediaTypeHandler_Release(pHandler);
    }

    IMFStreamDescriptor_Release(pSD);
}

    }

    IMFPresentationDescriptor_Release(pPD);
}
    
    /* Diagnostic: Check webcam source characteristics. */
DWORD characteristics = 0;

hr = IMFMediaSource_GetCharacteristics(
    source,
    &characteristics
);

printf("[DEBUG] GetCharacteristics HRESULT: 0x%08lX\n",
       (unsigned long)hr);

if (SUCCEEDED(hr))
{
    printf("[DEBUG] Source characteristics: 0x%08lX\n",
           (unsigned long)characteristics);
}

    /* Step 5: Create the Source Reader with default settings. */
    hr = MFCreateSourceReaderFromMediaSource(
        source,
        NULL,
        &reader
    );
    if (FAILED(hr)) {
        printf("[ERROR] CreateSourceReader: 0x%08lX\n", (unsigned long)hr);
        goto cleanup;
    }
    printf("[OK] Source Reader created.\n");

    /* Step 6: Inspect the default format without changing it. */
    hr = IMFSourceReader_GetCurrentMediaType(
        reader,
        MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        &mediaType
    );
    if (FAILED(hr)) {
        printf("[ERROR] GetCurrentMediaType: 0x%08lX\n",
               (unsigned long)hr);
        goto cleanup;
    }

    GUID subtype;
    hr = IMFMediaType_GetGUID(
        mediaType,
        &MF_MT_SUBTYPE,
        &subtype
    );
    if (SUCCEEDED(hr)) {
        printf("[INFO] Default subtype GUID: "
               "%08lX-%04X-%04X\n",
               (unsigned long)subtype.Data1,
               subtype.Data2,
               subtype.Data3);
    }

    UINT64 frameSize = 0;
    hr = IMFMediaType_GetUINT64(
        mediaType,
        &MF_MT_FRAME_SIZE,
        &frameSize
    );
    if (SUCCEEDED(hr)) {
        printf("[INFO] Default resolution: %u x %u\n",
               (UINT32)(frameSize >> 32),
               (UINT32)frameSize);
    }
    /* Request the webcam's native MJPG format. */

IMFMediaType *pMJPGType = NULL;

hr = IMFSourceReader_GetNativeMediaType(
    reader,
    MF_SOURCE_READER_FIRST_VIDEO_STREAM,
    1,  /* Native type index 1 is MJPG 1280x720 */
    &pMJPGType
);

printf("[DEBUG] GetNativeMediaType: 0x%08lX\n",
       (unsigned long)hr);

if (SUCCEEDED(hr))
{
    hr = IMFSourceReader_SetCurrentMediaType(
        reader,
        MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        NULL,
        pMJPGType
    );

    printf("[DEBUG] SetCurrentMediaType (MJPG): 0x%08lX\n",
           (unsigned long)hr);

    IMFMediaType_Release(pMJPGType);
}

if (FAILED(hr))
    goto cleanup;

    /* Step 7: Ensure the video stream is selected. */
    hr = IMFSourceReader_SetStreamSelection(
        reader,
        MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        TRUE
    );
    if (FAILED(hr)) {
        printf("[ERROR] SetStreamSelection: 0x%08lX\n",
               (unsigned long)hr);
        goto cleanup;
    }

/* Step 8: Keep reading until an actual frame arrives. */
printf("\n[TEST] Waiting for webcam frame...\n");

for (int attempt = 1; attempt <= 30; attempt++)
{
    streamIndex = 0;
    flags = 0;
    timestamp = 0;

    hr = IMFSourceReader_ReadSample(
        reader,
        MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        0,
        &streamIndex,
        &flags,
        &timestamp,
        &sample
    );

    printf("[DEBUG] Attempt %d | HRESULT: 0x%08lX | Flags: 0x%08lX\n",
           attempt,
           (unsigned long)hr,
           (unsigned long)flags);

    if (FAILED(hr))
    {
        printf("[ERROR] ReadSample failed.\n");
        goto cleanup;
    }

    if (flags & MF_SOURCE_READERF_ERROR)
    {
        printf("[ERROR] Stream error.\n");
        goto cleanup;
    }

    if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
    {
        printf("[ERROR] End of stream.\n");
        goto cleanup;
    }

    if (sample != NULL)
    {
        printf("\n[SUCCESS] Actual webcam frame received!\n");
        result = 0;
        break;
    }

    printf("[INFO] No sample yet. Waiting for next frame...\n");
}

if (sample == NULL)
{
    printf("\n[RESULT] No frame received after 30 attempts.\n");
}

cleanup:
    /* Release all interfaces and shut down Media Foundation. */
    if (sample)
        IMFSample_Release(sample);

    if (mediaType)
        IMFMediaType_Release(mediaType);

    if (reader)
        IMFSourceReader_Release(reader);

    if (source) {
        IMFMediaSource_Shutdown(source);
        IMFMediaSource_Release(source);
    }

    if (devices) {
        for (UINT32 i = 0; i < deviceCount; i++) {
            if (devices[i])
                IMFActivate_Release(devices[i]);
        }
        CoTaskMemFree(devices);
    }

    if (attributes)
        IMFAttributes_Release(attributes);

    if (mfStarted)
        MFShutdown();

    if (comStarted)
        CoUninitialize();

    printf("\n[INFO] Cleanup completed.\n");
    return result;
}
