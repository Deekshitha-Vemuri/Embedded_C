// Build: cl /TC audio.c /link ole32.lib
#define COBJMACROS
#include <windows.h>      // CoInitializeEx(), CoUninitialize(), CoCreateInstance(), CoTaskMemFree(), Sleep(), SetConsoleCtrlHandler()
#include <mmdeviceapi.h>  // IMMDeviceEnumerator_GetDefaultAudioEndpoint(), IMMDevice_Activate(), Release()
#include <audioclient.h>  // IAudioClient_Initialize(), GetMixFormat(), GetService(), Start(), Stop(), IAudioCaptureClient_GetNextPacketSize(), GetBuffer(), ReleaseBuffer()
#include <mmreg.h>        // WAVEFORMATEX, WAVEFORMATEXTENSIBLE
#include <ksmedia.h>      // SDK declarations: KSDATAFORMAT_SUBTYPE_PCM and KSDATAFORMAT_SUBTYPE_IEEE_FLOAT
#include <stdio.h>        // printf(), fflush()
#include <stdint.h>       // int16_t
#include <stddef.h>       // size_t
#include <string.h>       // memcmp()
#include <math.h>         // sqrt()

// Local Windows GUID definitions.
// These definitions avoid unresolved external GUID symbols when building with MSVC.
const CLSID CLSID_MMDeviceEnumerator =
    { 0xBCDE0395, 0xE52F, 0x467C, { 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E } };

const IID IID_IMMDeviceEnumerator =
    { 0xA95664D2, 0x9614, 0x4F35, { 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6 } };

const IID IID_IAudioClient =
    { 0x1CB9AD4C, 0xDBFA, 0x4C32, { 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2 } };

const IID IID_IAudioCaptureClient =
    { 0xC8ADBD64, 0xE71E, 0x48A0, { 0xA4, 0xDE, 0x18, 0x5C, 0x39, 0x5C, 0xD3, 0x17 } };

const GUID KSDATAFORMAT_SUBTYPE_PCM =
    { 0x00000001, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 } };

const GUID KSDATAFORMAT_SUBTYPE_IEEE_FLOAT =
    { 0x00000003, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 } };

#define SAMPLE_FORMAT_INT16  1

#define SAMPLE_FORMAT_FLOAT  2

#define REFTIMES_PER_SEC 10000000

#ifndef AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM

#define AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM 0x80000000

#endif

#ifndef AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY

#define AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY 0x08000000

#endif

static void TestLoopback(IMMDeviceEnumerator *enumerator)

{
    IMMDevice *spk = NULL;
    IAudioClient *c = NULL;
    WAVEFORMATEX *mix = NULL;
    HRESULT h;
    h = IMMDeviceEnumerator_GetDefaultAudioEndpoint(enumerator, eRender, eConsole, &spk);
    if (FAILED(h)) { printf("No render device 0x%08lX\n", (unsigned long)h); return; }
    IMMDevice_Activate(spk, &IID_IAudioClient, CLSCTX_ALL, NULL, (void **)&c);
    IAudioClient_GetMixFormat(c, &mix);
    h = IAudioClient_Initialize(c, AUDCLNT_SHAREMODE_SHARED,

                                0x00020000 /* AUDCLNT_STREAMFLAGS_LOOPBACK */,

                                10000000, 0, mix, NULL);

    printf("Loopback on speakers -> 0x%08lX\n", (unsigned long)h);
    CoTaskMemFree(mix);
    IAudioClient_Release(c);
    IMMDevice_Release(spk);
}
// This controls the capture loop when the user presses Ctrl+C.
static volatile LONG keepRunning = TRUE;
// This handles Ctrl+C from the console.
static BOOL WINAPI ConsoleHandler(DWORD signal)
{
    if (signal == CTRL_C_EVENT || signal == CTRL_BREAK_EVENT)
    {
        InterlockedExchange(&keepRunning, FALSE);
        return TRUE;
    }
    return FALSE;
}
// This prints the error returned by a Windows API function.
static void PrintError(const char *operation, HRESULT hr)

{

    printf("\n%s failed.\n", operation);
    printf("HRESULT = 0x%08lX\n", (unsigned long)hr);
    if (hr == E_ACCESSDENIED)
    {
        printf("Microphone access was denied.\n");
        printf("Check Windows microphone privacy settings.\n");
    }
    else if (hr == AUDCLNT_E_DEVICE_INVALIDATED)
    {
        printf("The microphone was disconnected or invalidated.\n");
    }
    else if (hr == AUDCLNT_E_SERVICE_NOT_RUNNING)
    {
        printf("The Windows audio service is not running.\n");
    }
    else if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT)
    {
        printf("The requested audio format is not supported.\n");
    }
    else
    {
        printf("An unexpected Windows API error occurred.\n");
    }
}

// This checks the audio format returned by GetMixFormat().

static int GetSampleFormat(WAVEFORMATEX *waveFormat)
{
    if (waveFormat->wFormatTag == WAVE_FORMAT_PCM &&
        waveFormat->wBitsPerSample == 16)
    {
        return SAMPLE_FORMAT_INT16;
    }
    if (waveFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT &&
        waveFormat->wBitsPerSample == 32)
    {
        return SAMPLE_FORMAT_FLOAT;
    }
    // Some Windows audio devices return WAVEFORMATEXTENSIBLE.
    if (waveFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
        waveFormat->cbSize >=
        sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))
    {
        WAVEFORMATEXTENSIBLE *extendedFormat =
            (WAVEFORMATEXTENSIBLE *)waveFormat;
        if (memcmp(&extendedFormat->SubFormat,
                   &KSDATAFORMAT_SUBTYPE_PCM, sizeof(GUID)) == 0 &&
            waveFormat->wBitsPerSample == 16 &&
            extendedFormat->Samples.wValidBitsPerSample == 16)
        {
            return SAMPLE_FORMAT_INT16;
        }
        if (memcmp(&extendedFormat->SubFormat,
                   &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, sizeof(GUID)) == 0 &&
            waveFormat->wBitsPerSample == 32 &&
            extendedFormat->Samples.wValidBitsPerSample == 32)
        {
            return SAMPLE_FORMAT_FLOAT;
        }
    }
    return 0;
}

// This calculates the RMS value for one audio packet.

static double CalculateRMS(

    BYTE *audioData,

    UINT32 frames,

    UINT16 channels,

    DWORD flags,

    int sampleFormat)

{
    size_t totalSamples = (size_t)frames * channels;
    double sumSquares = 0.0;
    // A silent packet does not contain useful sample data.
    if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
    {
        return 0.0;
    }
    if (audioData == NULL || totalSamples == 0)
    {
        return 0.0;
    }
    if (sampleFormat == SAMPLE_FORMAT_INT16)
    {
        int16_t *samples = (int16_t *)audioData;
        for (size_t i = 0; i < totalSamples; i++)
        {
            double value = samples[i] / 32768.0;
            sumSquares += value * value;
        }
    }
    else if (sampleFormat == SAMPLE_FORMAT_FLOAT)
    {
        float *samples = (float *)audioData;
        for (size_t i = 0; i < totalSamples; i++)
        {
            double value = samples[i];
            sumSquares += value * value;
        }
    }
    return sqrt(sumSquares / totalSamples);
}
int main(void)
{
    HRESULT hr;
    REFERENCE_TIME defaultPeriod = 0, minPeriod = 0;
    int exitCode = 0;
    BOOL comInitialized = FALSE;
    BOOL audioStarted = FALSE;
    BOOL handlerRegistered = FALSE;
    IMMDeviceEnumerator *deviceEnumerator = NULL;
    IMMDevice *microphone = NULL;
    IAudioClient *audioClient = NULL;
    IAudioCaptureClient *captureClient = NULL;
    WAVEFORMATEX *waveFormat = NULL;
    WAVEFORMATEX *closestMatch = NULL;
    UINT16 channels = 0;
    int sampleFormat = 0;
    // Initialize COM before using Windows audio interfaces.
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        PrintError("COM initialization", hr);
        return 1;
    }
    comInitialized = TRUE;
    // Get the Windows audio device enumerator.
    hr = CoCreateInstance(
        &CLSID_MMDeviceEnumerator,
        NULL,
        CLSCTX_ALL,
        &IID_IMMDeviceEnumerator,
        (void **)&deviceEnumerator
    );
    if (FAILED(hr))
    {
        PrintError("Creating audio device enumerator", hr);
        exitCode = 1;
        goto cleanup;
    }
    // Select the default microphone.
    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(
        deviceEnumerator,
        eCapture,
        eConsole,
        &microphone
    );
    if (FAILED(hr))
    {
        PrintError("Finding the default microphone", hr);
        printf("No suitable default recording device was found.\n");
        printf("Check whether a microphone is connected and enabled.\n");
        exitCode = 1;
        goto cleanup;
    }
    // Get the audio client for the selected microphone.
    hr = IMMDevice_Activate(
        microphone,
        &IID_IAudioClient,
        CLSCTX_ALL,
        NULL,
        (void **)&audioClient
    );
    if (FAILED(hr))
    {
        PrintError("Activating audio client", hr);
        exitCode = 1;
        goto cleanup;
    }
    // Find out which audio format Windows provides.
    hr = IAudioClient_GetMixFormat(
        audioClient,
        &waveFormat
    );
    if (FAILED(hr))
    {
        PrintError("Getting audio format", hr);
        exitCode = 1;
        goto cleanup;
    }
    printf("Tag=0x%04X cbSize=%u blockAlign=%u avgBytes=%lu\n",
           waveFormat->wFormatTag, waveFormat->cbSize,
           waveFormat->nBlockAlign, (unsigned long)waveFormat->nAvgBytesPerSec);
    if (waveFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        WAVEFORMATEXTENSIBLE *e = (WAVEFORMATEXTENSIBLE *)waveFormat;
        printf("ValidBits=%u ChannelMask=0x%08lX SubFormat.Data1=%lu\n",
               e->Samples.wValidBitsPerSample,
               (unsigned long)e->dwChannelMask,
               (unsigned long)e->SubFormat.Data1);
    }
    // Save the channel count before freeing the format structure.
    channels = waveFormat->nChannels;
    if (channels == 0)
    {
        printf("Invalid audio channel count.\n");
        exitCode = 1;
        goto cleanup;
    }
    printf("Sample rate : %lu Hz\n",
           (unsigned long)waveFormat->nSamplesPerSec);
    printf("Channels    : %u\n", channels);
    printf("Bits/sample : %u\n", waveFormat->wBitsPerSample);
    // Check whether this program supports the returned format.
    sampleFormat = GetSampleFormat(waveFormat);
    if (sampleFormat == 0)
    {
        printf("\nUnsupported audio format.\n");
        printf("This program supports 16-bit PCM and 32-bit IEEE Float.\n");
        exitCode = 1;
        goto cleanup;
    }
    // Check whether Windows accepts the audio format.
    hr = IAudioClient_IsFormatSupported(
        audioClient,
        AUDCLNT_SHAREMODE_SHARED,
        waveFormat,
        &closestMatch
    );
    TestLoopback(deviceEnumerator);
    if (hr == S_FALSE)
    {
        printf("\nThe exact format is not supported.\n");
        exitCode = 1;
        goto cleanup;
    }
    else if (FAILED(hr))
    {
        PrintError("Checking audio format support", hr);
        exitCode = 1;
        goto cleanup;
    }
    if (closestMatch != NULL)
    {
        CoTaskMemFree(closestMatch);
        closestMatch = NULL;
    }
    // Configure the microphone capture stream.
    hr = IAudioClient_GetDevicePeriod(
        audioClient,
        &defaultPeriod,
        &minPeriod
    );
    if (FAILED(hr))
    {
        PrintError("Getting audio device period", hr);
        exitCode = 1;
        goto cleanup;
    }
        // Configure the microphone capture stream.
    hr = IAudioClient_Initialize(
        audioClient,
        AUDCLNT_SHAREMODE_SHARED,
        0,              // stream flags
        0,              // use the default shared-mode buffer duration
        0,              // periodicity: 0 in shared mode
        waveFormat,
        NULL
    );
    if (FAILED(hr))
    {
        PrintError("Initializing audio stream", hr);
        exitCode = 1;
        goto cleanup;
    }
    // We no longer need the format structure.
    CoTaskMemFree(waveFormat);
    waveFormat = NULL;
    // Get the interface that provides the actual audio samples.
    hr = IAudioClient_GetService(
        audioClient,
        &IID_IAudioCaptureClient,
        (void **)&captureClient
    );
    if (FAILED(hr))
    {
        PrintError("Getting audio capture client", hr);
        exitCode = 1;
        goto cleanup;
    }
    // Start receiving microphone audio.
    hr = IAudioClient_Start(audioClient);
    if (FAILED(hr))
    {
        PrintError("Starting microphone capture", hr);
        exitCode = 1;
        goto cleanup;
    }
    audioStarted = TRUE;
    // Register the handler so Ctrl+C can stop the capture loop.
    if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE))
    {
        printf("Could not register the console handler.\n");
        exitCode = 1;
        goto cleanup;
    }
    handlerRegistered = TRUE;
    printf("\nMicrophone capture started!\n");
    printf("Speak near your microphone to see the RMS level.\n");
    printf("Press Ctrl+C to stop.\n\n");
    // Keep checking for available audio packets.
    while (InterlockedCompareExchange(&keepRunning, TRUE, TRUE))
    {
        UINT32 packetFrames = 0;
        hr = IAudioCaptureClient_GetNextPacketSize(
            captureClient,
            &packetFrames
        );
        if (FAILED(hr))
        {
            PrintError("Checking next audio packet", hr);
            exitCode = 1;
            break;
        }
        // Process all packets that are currently available.
        while (packetFrames > 0 &&
               InterlockedCompareExchange(&keepRunning, TRUE, TRUE))
        {
            BYTE *audioData = NULL;
            UINT32 numFrames = 0;
            DWORD flags = 0;
            hr = IAudioCaptureClient_GetBuffer(
                captureClient,
                &audioData,
                &numFrames,
                &flags,
                NULL,
                NULL
            );
            if (FAILED(hr))
            {
                PrintError("Getting audio buffer", hr);
                exitCode = 1;
                InterlockedExchange(&keepRunning, FALSE);
                break;
            }
            // Calculate the RMS before releasing the buffer.
            double rms = CalculateRMS(
                audioData,
                numFrames,
                channels,
                flags,
                sampleFormat
            );
            // Tell WASAPI that we have finished with this packet.
            hr = IAudioCaptureClient_ReleaseBuffer(
                captureClient,
                numFrames
            );
            if (FAILED(hr))
            {
                PrintError("Releasing audio buffer", hr);
                exitCode = 1;
                InterlockedExchange(&keepRunning, FALSE);
                break;
            }
            // Display the current microphone level.
            printf("\rRMS = %.4f       ", rms);
            fflush(stdout);
            // Check whether another packet is available.
            hr = IAudioCaptureClient_GetNextPacketSize(
                captureClient,
                &packetFrames
            );
            if (FAILED(hr))
            {
                PrintError("Checking next audio packet", hr);
                exitCode = 1;
                InterlockedExchange(&keepRunning, FALSE);
                break;
            }
        }
        // Wait briefly instead of continuously checking the device.
        Sleep(5);
    }
    printf("\n\nMicrophone capture stopped.\n");
cleanup:
    // Stop the audio stream before releasing its interfaces.
    if (audioStarted && audioClient != NULL)
    {
        hr = IAudioClient_Stop(audioClient);
        if (FAILED(hr))
        {
            PrintError("Stopping audio stream", hr);
            exitCode = 1;
        }
    }
    if (captureClient != NULL)
    {
        IAudioCaptureClient_Release(captureClient);
    }
    if (audioClient != NULL)
    {
        IAudioClient_Release(audioClient);
    }
    if (microphone != NULL)
    {
        IMMDevice_Release(microphone);
    }
    if (deviceEnumerator != NULL)
    {
        IMMDeviceEnumerator_Release(deviceEnumerator);
    }
    if (waveFormat != NULL)
    {
        CoTaskMemFree(waveFormat);
    }
    if (closestMatch != NULL)
    {
        CoTaskMemFree(closestMatch);
    }
    if (comInitialized)
    {
        CoUninitialize();
    }
    if (handlerRegistered)
    {

        SetConsoleCtrlHandler(ConsoleHandler, FALSE);
    }
    return exitCode;
}
