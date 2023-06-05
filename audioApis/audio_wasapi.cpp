#include "audio_wasapi.h"

#include <fstream>
#include <windows.h>
#include <wrl/client.h>
#include <objbase.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

// These constants are currently missing from the MinGW headers.
#ifndef AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM
# define AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM  0x80000000
#endif
#ifndef AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY
# define AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY  0x08000000
#endif

using namespace Microsoft::WRL;

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);

static ComPtr<IMMDeviceEnumerator> immdev_enumerator;

static struct WasapiState {
    ComPtr<IMMDevice> device;
    ComPtr<IAudioClient> client;
    ComPtr<IAudioRenderClient> rclient;
    UINT32 buffer_frame_count;
    bool initialized;
    bool started;
} wasapi;

static class NotificationClient : public IMMNotificationClient {
    LONG refcount;
public:
    NotificationClient() : refcount(1) {
    }

    virtual HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState) {
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId) {
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId) {
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId) {
        if (flow == eRender && role == eConsole) {
            // This callback runs on a separate thread,
            // but it's not important how fast this write takes effect.
            wasapi.initialized = false;
        }
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key) {
        return S_OK;
    }

    virtual ULONG STDMETHODCALLTYPE AddRef() {
        return InterlockedIncrement(&refcount);
    }

    virtual ULONG STDMETHODCALLTYPE Release() {
        ULONG rc = InterlockedDecrement(&refcount);
        if (rc == 0) {
            delete this;
        }
        return rc;
    }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface) {
        if (riid == __uuidof(IUnknown)) {
            AddRef();
            *ppvInterface = (IUnknown *)this;
        } else if (riid == __uuidof(IMMNotificationClient)) {
            AddRef();
            *ppvInterface = (IMMNotificationClient *)this;
        } else {
            *ppvInterface = nullptr;
            return E_NOINTERFACE;
        }
        return S_OK;
    }
} notification_client;

static void ThrowIfFailed(HRESULT res) {
    if (FAILED(res)) {
        throw res;
    }
}


bool AudioAPI_WASAPI::init()
{
    CoInitialize(NULL);
    try {
        ThrowIfFailed(CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&immdev_enumerator)));
    } catch (HRESULT res) {
        //DEBUG_PRINT("audio init fail!!! %d", res);
        return false;
    }

    ThrowIfFailed(immdev_enumerator->RegisterEndpointNotificationCallback(new NotificationClient()));
    //DEBUG_PRINT("audio init OK!!!");
    return true;
}

bool AudioAPI_WASAPI::destroy()
{
    return true;
}

void AudioAPI_WASAPI::play(int16_t* audioBuffer, uint32_t numSamples)
{
    if (!wasapi.initialized) {
        if (!setupStream()) {
            return;
        }
    }
    try {
        UINT32 frames = numSamples / 4;

        UINT32 padding;
        ThrowIfFailed(wasapi.client->GetCurrentPadding(&padding));
        if(padding >= 5400)
            return;
        //printf("%u %u\n", frames, padding);

        UINT32 available = wasapi.buffer_frame_count - padding;
        if (available < frames) {
            frames = available;

        }

        BYTE *data;
        ThrowIfFailed(wasapi.rclient->GetBuffer(frames, &data));
        memcpy(data, audioBuffer, frames * 4);
        ThrowIfFailed(wasapi.rclient->ReleaseBuffer(frames, 0));

        if (!wasapi.started && padding + frames > 1500) {
            wasapi.started = true;
            ThrowIfFailed(wasapi.client->Start());
        }
    } catch (HRESULT res) {
        wasapi = WasapiState();
    }
}

int AudioAPI_WASAPI::getBuffered()
{
    //DEBUG_PRINT("audio audio_wasapi_buffered!!!");
    if (!wasapi.initialized) {
        if (!setupStream()) {
            return 0;
        }
    }
    try {
        UINT32 padding;
        ThrowIfFailed(wasapi.client->GetCurrentPadding(&padding));
        return padding;
    } catch (HRESULT res) {
        wasapi = WasapiState();
        return 0;
    }
}

bool AudioAPI_WASAPI::setupStream()
{
    wasapi = WasapiState();

    try {
        ThrowIfFailed(immdev_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &wasapi.device));
        ThrowIfFailed(wasapi.device->Activate(IID_IAudioClient, CLSCTX_ALL, nullptr, IID_PPV_ARGS_Helper(&wasapi.client)));

        WAVEFORMATEX desired;
        desired.wFormatTag = WAVE_FORMAT_PCM;
        desired.nChannels = 2;
        desired.nSamplesPerSec = 32000;
        desired.nAvgBytesPerSec = 32000 * 2 * 2;
        desired.nBlockAlign = 4;
        desired.wBitsPerSample = 16;
        desired.cbSize = 0;

        ThrowIfFailed(wasapi.client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY, 2000000, 0, &desired, nullptr));

        ThrowIfFailed(wasapi.client->GetBufferSize(&wasapi.buffer_frame_count));
        ThrowIfFailed(wasapi.client->GetService(IID_PPV_ARGS(&wasapi.rclient)));

        wasapi.started = false;
        wasapi.initialized = true;
    } catch (HRESULT res) {
        wasapi = WasapiState();
        return false;
    }

    return true;
}
