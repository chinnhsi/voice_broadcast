#pragma once
#include "./AudioEndpoint.h"
class AudioBufferPack
{
  public:
  AudioBufferPack(const BYTE* pBuffer, UINT32 size)
      : pBuffer(pBuffer)
      , size(size)
  {}
  const BYTE* pBuffer;
  UINT32 size;
};
typedef void (*AudioCaptureCallbackFunction)(AudioBufferPack& pack);
class AudioCaptureEvent
{
  friend class AudioCapture;
  AudioCaptureEvent(IAudioClient*& pAudioClient, IAudioCaptureClient*& pAudioCaptureClient,
                    UINT32& sleepTime, UINT32& bufferFrameCount,
                    AudioCaptureCallbackFunction& callback, WAVEFORMATEX*& pWaveFormat, Log& log,
                    bool& stopSignal)
      : pAudioClient(pAudioClient)
      , pAudioCaptureClient(pAudioCaptureClient)
      , sleepTime(sleepTime)
      , bufferFrameCount(bufferFrameCount)
      , callback(callback)
      , pWaveFormat(pWaveFormat)
      , log(log)
      , stopSignal(stopSignal)
  {}
  IAudioClient*& pAudioClient;
  IAudioCaptureClient*& pAudioCaptureClient;
  UINT32& sleepTime;
  UINT32& bufferFrameCount;
  AudioCaptureCallbackFunction& callback;
  WAVEFORMATEX*& pWaveFormat;
  Log& log;
  bool& stopSignal;
};
class AudioCapture : public AudioEndpoint
{
  private:
  AudioCaptureCallbackFunction captureCallback;
  IAudioCaptureClient* pAudioCaptureClient = NULL;
  HANDLE threadHandle;
  static DWORD WINAPI StartCaptureStreamThread(LPVOID lpParameter);

  public:
  AudioCapture(DeviceInfo info)
      : AudioEndpoint(info)
  {
    log.ResetModuleName("AudioCapture");
  };
  bool GetCaptureService();   // 4 获取捕获Capture服务
  void SetCaptureStreamCallback(
      AudioCaptureCallbackFunction captureCallback);   // 5.1 设置捕获的音频流Callback
  void StartCaptureStream();                           // 5.2 开始捕获音频流
};