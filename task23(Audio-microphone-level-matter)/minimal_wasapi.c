#define COBJMACROS

#include <windows.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <stdio.h>

#define REFTIMES_PER_SEC 10000000

int main(void)
{
    HRESULT hr;

    IMMDeviceEnumerator *enumerator = NULL;
    IMMDevice *device = NULL;
    IAudioClient *audioClient = NULL;
    WAVEFORMATEX *waveFormat = NULL;
    WAVEFORMATEX *closestMatch = NULL;
    WAVEFORMATEX testFormat = {0};
    BOOL comInitialized = FALSE;

    // Step 1: Initialize COM.
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    printf("CoInitializeEx: 0x%08lX\n", (unsigned long)hr);

    if (FAILED(hr))
    {
        printf("COM initialization failed.\n");
        return 1;
    }

    comInitialized = TRUE;

    // Step 2: Create the audio device enumerator.
    hr = CoCreateInstance(
        &CLSID_MMDeviceEnumerator,
        NULL,
        CLSCTX_ALL,
        &IID_IMMDeviceEnumerator,
        (void **)&enumerator
    );

    printf("CoCreateInstance: 0x%08lX\n", (unsigned long)hr);

    if (FAILED(hr))
        goto cleanup;

    // Step 3: Get the default microphone.
    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(
        enumerator,
        eCapture,
        eCommunications,
        &device
    );

    printf("GetDefaultAudioEndpoint: 0x%08lX\n",
           (unsigned long)hr);

    if (FAILED(hr))
        goto cleanup;

    // Step 4: Activate the audio client.
    hr = IMMDevice_Activate(
        device,
        &IID_IAudioClient,
        CLSCTX_ALL,
        NULL,
        (void **)&audioClient
    );

    printf("Activate: 0x%08lX\n", (unsigned long)hr);

    if (FAILED(hr))
        goto cleanup;

    // Step 5: Retrieve the microphone's mix format.
    hr = IAudioClient_GetMixFormat(
        audioClient,
        &waveFormat
    );

    printf("GetMixFormat: 0x%08lX\n", (unsigned long)hr);

    if (FAILED(hr))
        goto cleanup;

    printf("\n--- Audio Format ---\n");
    printf("Format tag  : 0x%04X\n", waveFormat->wFormatTag);
    printf("Channels    : %u\n", waveFormat->nChannels);
    printf("Sample rate : %lu\n",
           (unsigned long)waveFormat->nSamplesPerSec);
    printf("Bits/sample : %u\n", waveFormat->wBitsPerSample);
    printf("Block align : %u\n", waveFormat->nBlockAlign);

    // Configure a 16-bit PCM test format.
    testFormat.wFormatTag = WAVE_FORMAT_PCM;
    testFormat.nChannels = 2;
    testFormat.nSamplesPerSec = 48000;
    testFormat.wBitsPerSample = 16;
    testFormat.nBlockAlign = 4;
    testFormat.nAvgBytesPerSec = 192000;
    testFormat.cbSize = 0;

    // Check whether the microphone supports the 16-bit PCM format.
    hr = IAudioClient_IsFormatSupported(
        audioClient,
        AUDCLNT_SHAREMODE_SHARED,
        &testFormat,
        &closestMatch
    );

    printf("\nIsFormatSupported HRESULT: 0x%08lX\n",
           (unsigned long)hr);

    if (hr == S_OK)
    {
        printf("The microphone supports the requested PCM format.\n");
    }
    else if (hr == S_FALSE)
    {
        printf("The exact PCM format is not supported.\n");

        if (closestMatch != NULL)
        {
            printf("Closest format: %u bits, %lu Hz\n",
                   closestMatch->wBitsPerSample,
                   (unsigned long)closestMatch->nSamplesPerSec);
        }
    }
    else
    {
        printf("Format support check failed.\n");
    }

    if (closestMatch != NULL)
    {
        CoTaskMemFree(closestMatch);
        closestMatch = NULL;
    }

    // Step 6: Initialize the audio stream.
    printf("\nInitializing WASAPI...\n");

    hr = IAudioClient_Initialize(
        audioClient,
        AUDCLNT_SHAREMODE_SHARED,
        0,
        0,
        0,
        &testFormat,
        NULL
    );

    printf("Initialize: 0x%08lX\n", (unsigned long)hr);

    if (SUCCEEDED(hr))
    {
        printf("\nSUCCESS: WASAPI initialized successfully!\n");
    }
    else
    {
        printf("\nFAILED: WASAPI initialization failed.\n");
    }

cleanup:

    if (waveFormat != NULL)
        CoTaskMemFree(waveFormat);

    if (audioClient != NULL)
        IAudioClient_Release(audioClient);

    if (device != NULL)
        IMMDevice_Release(device);

    if (enumerator != NULL)
        IMMDeviceEnumerator_Release(enumerator);

    if (comInitialized)
        CoUninitialize();

    return FAILED(hr) ? 1 : 0;
}