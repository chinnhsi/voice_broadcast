#pragma once
#include <windows.h>
#include <Mmdeviceapi.h>
#include <initguid.h>
#include "../Log.h"
#include <mmeapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <Audioclient.h>
#include <string>
#include "AudioEndpointCollection.h"
#define EXIT_IF_ERROR(hr, str) \
  if (FAILED(hr)) {            \
    log.Error(str);            \
    goto EXIT;                 \
  }
#define SAFE_RELEASE(punk) \
  if ((punk) != NULL) {    \
    (punk)->Release();     \
    (punk) = NULL;         \
  }
#define REFTIMES_PER_SEC 1000000
#define REFTIMES_PER_MILLISEC 1000
#define MAX_BUFFER_SIZE 131072   // 缓冲池大小128k

using std::string;

class AudioEndpoint
{
  protected:
  HRESULT hr;
  IMMDevice* device                   = NULL;
  IAudioClient* pAudioClient          = NULL;
  WAVEFORMATEX* pWaveFormat           = NULL;
  REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
  REFERENCE_TIME hnsActualDuration;
  UINT32 sleepTime;
  UINT32 bufferFrameCount;
  Log& log           = *(Log::Bind("AudioEndpoint", 0));
  bool isInitialized = false;
  bool stopSignal    = false;     // 停止流
  bool Activate();                // 1. 创建音频流组件IAudioClient
  bool CreateDeviceFormat();      // 2. 设置共享音频流格式
  bool Initialize(DWORD flags);   // 3. 初始化音频流

  public:
  AudioEndpoint(DeviceInfo info) { device = info.device; };
  bool Load(DWORD flags);
  void StopStream();
};