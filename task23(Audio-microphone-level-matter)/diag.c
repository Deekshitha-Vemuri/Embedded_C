#define COBJMACROS
#include <windows.h>
#include <initguid.h>
#include <functiondiscoverykeys_devpkey.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <mmreg.h>
#include <stdio.h>
#ifndef AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM
#define AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM 0x80000000
#endif
#ifndef AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY
#define AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY 0x08000000
#endif

#define RAW_OPTION 0x1   /* AUDCLNT_STREAMOPTIONS_RAW */

typedef struct { UINT32 cbSize; BOOL bIsOffload; int eCategory; int Options; } MyClientProps;

static void Try(IMMDevice *dev, const char *label, int raw, int usePeriod, DWORD flags, int pcm16)
{
    IAudioClient *c = NULL;
    WAVEFORMATEX *mix = NULL, fmt;
    REFERENCE_TIME def = 0, min = 0, dur = 0;
    HRESULT hr;

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_ALL, NULL, (void **)&c);
    if (FAILED(hr)) { printf("  %-34s Activate 0x%08lX\n", label, (unsigned long)hr); return; }

    if (raw)
    {
        IAudioClient2 *c2 = NULL;
        if (SUCCEEDED(IAudioClient_QueryInterface(c, &IID_IAudioClient2, (void **)&c2)))
        {
            MyClientProps p = { sizeof(p), FALSE, 0, RAW_OPTION };
            hr = IAudioClient2_SetClientProperties(c2, (AudioClientProperties *)&p);
            if (FAILED(hr)) printf("  (SetClientProperties 0x%08lX)\n", (unsigned long)hr);
            IAudioClient2_Release(c2);
        }
        else printf("  (IAudioClient2 not available)\n");
    }

    IAudioClient_GetMixFormat(c, &mix);
    IAudioClient_GetDevicePeriod(c, &def, &min);
    dur = usePeriod ? def : 0;

    if (pcm16)
    {
        memset(&fmt, 0, sizeof(fmt));
        fmt.wFormatTag = WAVE_FORMAT_PCM;
        fmt.nChannels = 2;
        fmt.nSamplesPerSec = 48000;
        fmt.wBitsPerSample = 16;
        fmt.nBlockAlign = 4;
        fmt.nAvgBytesPerSec = 48000 * 4;
        hr = IAudioClient_Initialize(c, AUDCLNT_SHAREMODE_SHARED, flags, dur, 0, &fmt, NULL);
    }
    else
        hr = IAudioClient_Initialize(c, AUDCLNT_SHAREMODE_SHARED, flags, dur, 0, mix, NULL);

    printf("  %-34s -> 0x%08lX\n", label, (unsigned long)hr);

    CoTaskMemFree(mix);
    IAudioClient_Release(c);
}

int main(void)
{
    IMMDeviceEnumerator *en = NULL;
    IMMDeviceCollection *col = NULL;
    UINT n = 0;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void **)&en);
    IMMDeviceEnumerator_EnumAudioEndpoints(en, eCapture, DEVICE_STATE_ACTIVE, &col);
    IMMDeviceCollection_GetCount(col, &n);
    printf("Active capture devices: %u\n", n);

    for (UINT i = 0; i < n; i++)
    {
        IMMDevice *d = NULL;
        IPropertyStore *ps = NULL;
        PROPVARIANT v;

        IMMDeviceCollection_Item(col, i, &d);
        PropVariantInit(&v);
        if (SUCCEEDED(IMMDevice_OpenPropertyStore(d, STGM_READ, &ps)))
        {
            if (SUCCEEDED(IPropertyStore_GetValue(ps, &PKEY_Device_FriendlyName, &v)))
                wprintf(L"\nDevice %u: %ls\n", i, v.pwszVal);
            PropVariantClear(&v);
            IPropertyStore_Release(ps);
        }

        Try(d, "mix, dur=0",                 0, 0, 0, 0);
        Try(d, "mix, dur=device period",     0, 1, 0, 0);
        Try(d, "mix, dur=period, event mode",0, 1, 0x00040000, 0);
        Try(d, "RAW, mix, dur=0",            1, 0, 0, 0);
        Try(d, "RAW, mix, dur=period",       1, 1, 0, 0);
        Try(d, "RAW, pcm16 autoconvert",     1, 0, AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY, 1);
        IMMDevice_Release(d);
    }

    IMMDeviceCollection_Release(col);
    IMMDeviceEnumerator_Release(en);
    CoUninitialize();
    return 0;
}