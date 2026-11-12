//build command: cl /TC /std:c11 webcam_brightness.c /link mf.lib mfplat.lib mfreadwrite.lib mfuuid.lib ole32.lib
#define WINVER 0x0A00
#define _WIN32_WINNT 0x0A00
#define COBJMACROS
 
/* Standard input/output functions such as printf(). */
#include <stdio.h>
 
/* Windows data types, HRESULT, BYTE, DWORD and Windows definitions. */
#include <windows.h>
 
/* COM initialization and cleanup functions. */
#include <objbase.h>
 
/* Media Foundation startup, shutdown and media type configuration functions. */
#include <mfapi.h>
 
/* Media Foundation interfaces for webcam activation, media types and media sources. */
#include <mfidl.h>
 
/* Media buffer and 2D buffer interfaces used to access frame memory. */
#include <mfobjects.h>
 
/* Source Reader creation and frame-reading functions. */
#include <mfreadwrite.h>
 
/* Media Foundation error codes and stream-related definitions. */
#include <mferror.h>
 
/* Console keyboard functions used to stop the program by pressing Q. */
#include <conio.h>
 
 
/* ---------------------------------------------------------
   GLOBAL CONFIGURATION
 
   We are using the native NV12 format supported by the
   laptop webcam.
   --------------------------------------------------------- */
 
#define FRAME_WIDTH  640
#define FRAME_HEIGHT 480
#define FRAME_RATE   30
 
 
/* ---------------------------------------------------------
   FUNCTION: NormalizeY
 
   Converts limited-range Y values (16-235) to 0-255.
   Full-range Y values are already in the 0-255 range.
   --------------------------------------------------------- */
 
static BYTE NormalizeY(BYTE y, UINT32 nominalRange)
{
    if (nominalRange == MFNominalRange_16_235)
    {
        if (y <= 16)
            return 0;
 
        if (y >= 235)
            return 255;
 
        return (BYTE)(((UINT32)(y - 16) * 255 + 109) / 219);
    }
 
    return y;
}
 
 
/* ---------------------------------------------------------
   FUNCTION: CalculateBrightness
 
   Reads only the Y plane of an NV12 frame and calculates
   its average brightness.
 
   First, it tries to access the frame using IMF2DBuffer.
   If that interface is unavailable, it uses IMFMediaBuffer
   and the default stride stored in the media type.
   --------------------------------------------------------- */
 
static HRESULT CalculateBrightness(
    IMFMediaBuffer *pBuffer,
    IMFMediaType *pMediaType,
    UINT32 width,
    UINT32 height,
    UINT32 nominalRange,
    double *brightness)
{
    HRESULT hr;
 
    BYTE *scanline0 = NULL;
    BYTE *pData = NULL;
 
    LONG pitch = 0;
    LONG stride = 0;
 
    DWORD maxLength = 0;
    DWORD currentLength = 0;
 
    unsigned long long sum = 0;
 
    IMF2DBuffer *p2DBuffer = NULL;
 
    /* Try to obtain the 2D buffer interface. */
    hr = IUnknown_QueryInterface(
        (IUnknown *)pBuffer,
        &IID_IMF2DBuffer,
        (void **)&p2DBuffer
    );
 
    if (SUCCEEDED(hr))
    {
        /* Lock the image memory and obtain its row pitch. */
        hr = IMF2DBuffer_Lock2D(
            p2DBuffer,
            &scanline0,
            &pitch
        );
 
        if (FAILED(hr))
        {
            IMF2DBuffer_Release(p2DBuffer);
            return hr;
        }
 
        LONG absPitch = (pitch < 0) ? -pitch : pitch;
 
        if (absPitch < (LONG)width)
        {
            IMF2DBuffer_Unlock2D(p2DBuffer);
            IMF2DBuffer_Release(p2DBuffer);
            return E_FAIL;
        }
 
        /* Read one Y value for every pixel. */
        for (UINT32 row = 0; row < height; row++)
        {
            BYTE *rowData = scanline0 + ((LONG)row * pitch);
 
            for (UINT32 col = 0; col < width; col++)
            {
                BYTE y = NormalizeY(rowData[col], nominalRange);
                sum += y;
            }
        }
 
        IMF2DBuffer_Unlock2D(p2DBuffer);
        IMF2DBuffer_Release(p2DBuffer);
 
        *brightness = (double)sum / ((double)width * height);
 
        return S_OK;
    }
 
 
    /* If the buffer does not support 2D locking, use normal Lock(). */
 
    hr = IMFMediaBuffer_Lock(
        pBuffer,
        &pData,
        &maxLength,
        &currentLength
    );
 
    if (FAILED(hr))
        return hr;
 
    UINT32 strideValue = 0;
 
    /* Try to retrieve the actual row stride from the media type. */
    hr = IMFMediaType_GetUINT32(
        pMediaType,
        &MF_MT_DEFAULT_STRIDE,
        &strideValue
    );
 
    if (SUCCEEDED(hr))
    {
        stride = (LONG)strideValue;
    }
    else
    {
        /* Calculate the default stride if it was not specified. */
        hr = MFGetStrideForBitmapInfoHeader(
            MFVideoFormat_NV12.Data1,
            width,
            &stride
        );
    }
 
    if (FAILED(hr) || stride == 0)
    {
        IMFMediaBuffer_Unlock(pBuffer);
        return E_FAIL;
    }
 
    LONG absStride = (stride < 0) ? -stride : stride;
 
    if (absStride < (LONG)width ||
        (unsigned long long)absStride * height > currentLength)
    {
        IMFMediaBuffer_Unlock(pBuffer);
        return E_FAIL;
    }
 
    /* Find the first logical row when the stride is negative. */
    BYTE *firstRow = pData;
 
    if (stride < 0)
    {
        firstRow = pData + ((height - 1) * absStride);
    }
 
    /* Read only the Y plane. The UV plane is not required. */
    for (UINT32 row = 0; row < height; row++)
    {
        BYTE *rowData = firstRow + ((LONG)row * stride);
 
        for (UINT32 col = 0; col < width; col++)
        {
            BYTE y = NormalizeY(rowData[col], nominalRange);
            sum += y;
        }
    }
 
    IMFMediaBuffer_Unlock(pBuffer);
 
    *brightness = (double)sum / ((double)width * height);
 
    return S_OK;
}
 
 
/* ---------------------------------------------------------
   MAIN PROGRAM
   --------------------------------------------------------- */
 
int main(void)
{
    HRESULT hr;
 
    BOOL comInitialized = FALSE;
    BOOL mfInitialized = FALSE;
 
    IMFAttributes *pAttributes = NULL;
    IMFActivate **ppDevices = NULL;
 
    IMFMediaSource *pSource = NULL;
    IMFSourceReader *pReader = NULL;
 
    IMFMediaType *pRequestedType = NULL;
    IMFMediaType *pCurrentType = NULL;
 
    IMFSample *pSample = NULL;
    IMFMediaBuffer *pBuffer = NULL;
 
    UINT32 deviceCount = 0;
 
    UINT32 actualWidth = 0;
    UINT32 actualHeight = 0;
 
    UINT32 nominalRange = MFNominalRange_Unknown;
 
    GUID subtype;
 
    unsigned long long frameCount = 0;
 
 
    printf("============================================\n");
    printf("       WEBCAM BRIGHTNESS METER\n");
    printf("============================================\n\n");
 
 
    /* -----------------------------------------------------
       STEP 1: INITIALIZE COM
       ----------------------------------------------------- */
 
    printf("[1] Initializing COM...\n");
 
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
 
    if (FAILED(hr))
    {
        printf("[ERROR] COM initialization failed: 0x%08lX\n",
               (unsigned long)hr);
        goto cleanup;
    }
 
    comInitialized = TRUE;
 
    printf("[SUCCESS] COM initialized.\n\n");
 
 
    /* -----------------------------------------------------
       STEP 2: INITIALIZE MEDIA FOUNDATION
       ----------------------------------------------------- */
 
    printf("[2] Initializing Media Foundation...\n");
 
    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
 
    if (FAILED(hr))
    {
        printf("[ERROR] Media Foundation initialization failed.\n");
        goto cleanup;
    }
 
    mfInitialized = TRUE;
 
    printf("[SUCCESS] Media Foundation initialized.\n\n");
 
 
    /* -----------------------------------------------------
       STEP 3: PREPARE WEBCAM DISCOVERY
       ----------------------------------------------------- */
 
    printf("[3] Preparing webcam discovery...\n");
 
    hr = MFCreateAttributes(&pAttributes, 1);
 
    if (FAILED(hr))
    {
        printf("[ERROR] Failed to create device attributes.\n");
        goto cleanup;
    }
 
    hr = IMFAttributes_SetGUID(
        pAttributes,
        &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
        &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
    );
 
    if (FAILED(hr))
    {
        printf("[ERROR] Failed to configure video capture attributes.\n");
        goto cleanup;
    }
 
    printf("[SUCCESS] Webcam discovery attributes configured.\n\n");
 
 
    /* -----------------------------------------------------
       STEP 4: ENUMERATE AVAILABLE WEBCAMS
       ----------------------------------------------------- */
 
    printf("[4] Searching for webcams...\n");
 
    hr = MFEnumDeviceSources(
        pAttributes,
        &ppDevices,
        &deviceCount
    );
 
    if (FAILED(hr))
    {
        printf("[ERROR] Webcam enumeration failed.\n");
        goto cleanup;
    }
 
    printf("[SUCCESS] Found %u webcam(s).\n", deviceCount);
 
    if (deviceCount == 0)
    {
        printf("[ERROR] No webcam detected.\n");
        goto cleanup;
    }
 
    printf("\n");
 
 
    /* -----------------------------------------------------
       STEP 5: ACTIVATE THE FIRST WEBCAM
       ----------------------------------------------------- */
 
    printf("[5] Activating webcam...\n");
 
    hr = IMFActivate_ActivateObject(
        ppDevices[0],
        &IID_IMFMediaSource,
        (void **)&pSource
    );
 
    if (FAILED(hr))
    {
        printf("[ERROR] Webcam activation failed.\n");
        goto cleanup;
    }
 
    printf("[SUCCESS] Webcam activated.\n\n");
 
 
    /* -----------------------------------------------------
       STEP 6: CREATE SOURCE READER
       ----------------------------------------------------- */
 
    printf("[6] Creating Source Reader...\n");
 
    hr = MFCreateSourceReaderFromMediaSource(
        pSource,
        NULL,
        &pReader
    );
 
    if (FAILED(hr))
    {
        printf("[ERROR] Source Reader creation failed.\n");
        goto cleanup;
    }
 
    printf("[SUCCESS] Source Reader created.\n\n");
 
 
    /* -----------------------------------------------------
       STEP 7: CREATE AND CONFIGURE NV12 MEDIA TYPE
       ----------------------------------------------------- */
 
    printf("[7] Configuring video format...\n");
 
    hr = MFCreateMediaType(&pRequestedType);
 
    if (FAILED(hr))
    {
        printf("[ERROR] Failed to create requested media type.\n");
        goto cleanup;
    }
 
    /* Tell Media Foundation that this is a video stream. */
    hr = IMFMediaType_SetGUID(
        pRequestedType,
        &MF_MT_MAJOR_TYPE,
        &MFMediaType_Video
    );
 
    if (FAILED(hr))
    {
        printf("[ERROR] Failed to set video major type.\n");
        goto cleanup;
    }
 
    /* Request NV12 pixel format. */
    hr = IMFMediaType_SetGUID(
        pRequestedType,
        &MF_MT_SUBTYPE,
        &MFVideoFormat_NV12
    );
 
    if (FAILED(hr))
    {
        printf("[ERROR] Failed to set NV12 format.\n");
        goto cleanup;
    }
 
    /* Request 640 x 480 resolution. */
   UINT64 frameSize =
    ((UINT64)FRAME_WIDTH << 32) | FRAME_HEIGHT;
 
hr = IMFMediaType_SetUINT64(
    pRequestedType,
    &MF_MT_FRAME_SIZE,
    frameSize
);
 
    if (FAILED(hr))
    {
        printf("[ERROR] Failed to set frame resolution.\n");
        goto cleanup;
    }
 
    /* Request 30 FPS. */
    UINT64 frameRate =
    ((UINT64)FRAME_RATE << 32) | 1;
 
hr = IMFMediaType_SetUINT64(
    pRequestedType,
    &MF_MT_FRAME_RATE,
    frameRate
);
 
    if (FAILED(hr))
    {
        printf("[ERROR] Failed to set frame rate.\n");
        goto cleanup;
    }
 
    printf("[SUCCESS] Requested NV12, 640 x 480, 30 FPS.\n");
 
 
    /* Apply the requested format to the Source Reader. */
    hr = IMFSourceReader_SetCurrentMediaType(
        pReader,
        MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        NULL,
        pRequestedType
    );
 
    if (FAILED(hr))
    {
        printf("[ERROR] Source Reader rejected NV12 configuration.\n");
        goto cleanup;
    }
 
    printf("[SUCCESS] Source Reader accepted the requested format.\n\n");
 
 
    /* -----------------------------------------------------
       STEP 8: VERIFY THE CURRENT OUTPUT FORMAT
       ----------------------------------------------------- */
 
    printf("[8] Verifying current video format...\n");
 
    hr = IMFSourceReader_GetCurrentMediaType(
        pReader,
        MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        &pCurrentType
    );
 
    if (FAILED(hr))
    {
        printf("[ERROR] Failed to retrieve current media type.\n");
        goto cleanup;
    }
 
    hr = IMFMediaType_GetGUID(
        pCurrentType,
        &MF_MT_SUBTYPE,
        &subtype
    );
 
    if (FAILED(hr) || !IsEqualGUID(&subtype, &MFVideoFormat_NV12))
    {
        printf("[ERROR] Source Reader is not providing NV12.\n");
        goto cleanup;
    }
 
 
hr = IMFMediaType_GetUINT64(
    pCurrentType,
    &MF_MT_FRAME_SIZE,
    &frameSize
);
 
if (SUCCEEDED(hr))
{
    actualWidth = (UINT32)(frameSize >> 32);
    actualHeight = (UINT32)frameSize;
}
 
    if (FAILED(hr))
    {
        printf("[ERROR] Failed to retrieve current resolution.\n");
        goto cleanup;
    }
 
    if (actualWidth != FRAME_WIDTH || actualHeight != FRAME_HEIGHT)
    {
        printf("[ERROR] Unexpected frame resolution: %u x %u\n",
               actualWidth, actualHeight);
        goto cleanup;
    }
 
    /* Check whether the Y plane uses full or limited video range. */
    hr = IMFMediaType_GetUINT32(
        pCurrentType,
        &MF_MT_VIDEO_NOMINAL_RANGE,
        &nominalRange
    );
 
    if (FAILED(hr))
    {
        nominalRange = MFNominalRange_Unknown;
        printf("[INFO] Nominal range not specified; using raw Y values.\n");
    }
    else if (nominalRange == MFNominalRange_16_235)
    {
        printf("[INFO] Limited-range Y detected. Normalizing to 0-255.\n");
    }
    else if (nominalRange == MFNominalRange_0_255)
    {
        printf("[INFO] Full-range Y detected.\n");
    }
    else
    {
        printf("[INFO] Other or unspecified Y range; using raw Y values.\n");
    }
 
    printf("[SUCCESS] Confirmed NV12 at %u x %u.\n\n",
           actualWidth, actualHeight);
 
 
    /* -----------------------------------------------------
       STEP 9: CONTINUOUS FRAME CAPTURE
       ----------------------------------------------------- */
 
    printf("[9] Starting webcam frame capture...\n");
    printf("[INFO] Cover and uncover the camera to observe changes.\n");
    printf("[INFO] Press Q to stop the program.\n\n");
 
 
    while (1)
    {
        /* Stop when the user presses Q. */
        if (_kbhit())
        {
            int key = _getch();
 
            if (key == 'q' || key == 'Q')
            {
                printf("\n[INFO] Stop requested by user.\n");
                break;
            }
        }
 
        DWORD actualStreamIndex = 0;
        DWORD streamFlags = 0;
 
        LONGLONG timestamp = 0;
 
        pSample = NULL;
 
        /* Request one video sample from the webcam. */
        hr = IMFSourceReader_ReadSample(
            pReader,
            MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            0,
            &actualStreamIndex,
            &streamFlags,
            &timestamp,
            &pSample
        );
 
        if (FAILED(hr))
        {
            printf("[ERROR] Failed to read webcam frame: 0x%08lX\n",
                   (unsigned long)hr);
            goto cleanup;
        }
 
        if (streamFlags & MF_SOURCE_READERF_ERROR)
        {
            printf("[ERROR] Source Reader reported a stream error.\n");
            goto cleanup;
        }
 
        if (streamFlags & MF_SOURCE_READERF_ENDOFSTREAM)
        {
            printf("[INFO] End of video stream.\n");
            break;
        }
 
        if (streamFlags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED)
        {
            printf("[ERROR] Video output format changed during capture.\n");
            goto cleanup;
        }
 
        /* A stream tick or temporary absence of a sample is possible. */
        if (pSample == NULL)
            continue;
 
 
        /* -------------------------------------------------
           STEP 10: GET THE FRAME BUFFER
           ------------------------------------------------- */
 
        hr = IMFSample_ConvertToContiguousBuffer(
            pSample,
            &pBuffer
        );
 
        if (FAILED(hr))
        {
            printf("[ERROR] Failed to obtain frame buffer.\n");
            IMFSample_Release(pSample);
            pSample = NULL;
            goto cleanup;
        }
 
 
        /* -------------------------------------------------
           STEP 11: CALCULATE FRAME BRIGHTNESS
           ------------------------------------------------- */
 
        double brightness = 0.0;
 
        hr = CalculateBrightness(
            pBuffer,
            pCurrentType,
            actualWidth,
            actualHeight,
            nominalRange,
            &brightness
        );
 
        /* Release the frame resources after processing. */
        IMFMediaBuffer_Release(pBuffer);
        pBuffer = NULL;
 
        IMFSample_Release(pSample);
        pSample = NULL;
 
        if (FAILED(hr))
        {
            printf("[ERROR] Failed to calculate frame brightness.\n");
            goto cleanup;
        }
 
        frameCount++;
 
        printf("Frame: %llu | Brightness: %.2f / 255\n",
               frameCount, brightness);
    }
 
 
    printf("\n[INFO] Total frames processed: %llu\n", frameCount);
 
 
    /* -----------------------------------------------------
       STEP 12: CLEANUP
       ----------------------------------------------------- */
 
cleanup:
 
    printf("\n[Cleanup] Releasing resources...\n");
 
    if (pBuffer)
        IMFMediaBuffer_Release(pBuffer);
 
    if (pSample)
        IMFSample_Release(pSample);
 
    if (pCurrentType)
        IMFMediaType_Release(pCurrentType);
 
    if (pRequestedType)
        IMFMediaType_Release(pRequestedType);
 
    if (pReader)
        IMFSourceReader_Release(pReader);
 
    if (pSource)
    {
        IMFMediaSource_Shutdown(pSource);
        IMFMediaSource_Release(pSource);
    }
 
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