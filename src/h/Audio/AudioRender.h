#pragma once
#include "./AudioEndpoint.h"
class AudioRenderBuffer
{
  public:
  BYTE buffer[MAX_BUFFER_SIZE];
  /*
  ┌───────────────────────────────────────────────┐
  │  Empty Area  |  Filled Area  |   Empty Area   │
  ├───────────────────────────────────────────────┤
  │              ↑←  bufferLen  →↑                │
  │          usedIndex      filledIndex           │
  │←              MAX_BUFFER_SIZE                →│
  └───────────────────────────────────────────────┘
  */
  UINT32 bufferLen   = 0;   // 总缓存长度
  UINT32 filledIndex = 0;   // 已缓冲的最末位置
  UINT32 usedIndex   = 0;   // 已缓冲的起始位置
  AudioRenderBuffer() { memset(buffer, 0, MAX_BUFFER_SIZE); }
  static void Reset(AudioRenderBuffer& buffer)
  {
    buffer.bufferLen   = 0;
    buffer.filledIndex = 0;
    buffer.usedIndex   = 0;
  }
};
class AudioRenderEvent
{
  friend class AudioRender;
  AudioRenderEvent(IAudioClient*& pAudioClient, IAudioRenderClient*& pAudioRenderClient,
                   UINT32& sleepTime, UINT32& bufferFrameCount, WAVEFORMATEX*& pWaveFormat,
                   HANDLE& mutex, AudioRenderBuffer& buffer, Log& log, bool& stopSignal)
      : pAudioClient(pAudioClient)
      , pAudioRenderClient(pAudioRenderClient)
      , sleepTime(sleepTime)
      , bufferFrameCount(bufferFrameCount)
      , pWaveFormat(pWaveFormat)
      , mutex(mutex)
      , buffer(buffer)
      , log(log)
      , stopSignal(stopSignal)
  {}
  IAudioClient*& pAudioClient;
  IAudioRenderClient*& pAudioRenderClient;
  UINT32& sleepTime;
  UINT32& bufferFrameCount;
  HANDLE& mutex;
  WAVEFORMATEX*& pWaveFormat;
  AudioRenderBuffer& buffer;
  Log& log;
  bool& stopSignal;
};
class AudioRender : public AudioEndpoint
{
  private:
  HANDLE threadHandle;
  IAudioRenderClient* pAudioRenderClient = NULL;
  static DWORD WINAPI StartRenderStreamThread(LPVOID lpParameter);
  static bool inline CopyBuffer(BYTE* pData, AudioRenderBuffer& buffer, UINT32& len);

  public:
  HANDLE mutex = CreateMutex(NULL, false, NULL);
  AudioRenderBuffer buffer;
  AudioRender(DeviceInfo info)
      : AudioEndpoint(info)
  {
    log.ResetModuleName("AudioRender");
  };
  bool GetRenderService();    // 4 获取呈现Render服务
  void StartRenderStream();   // 5 开始呈现音频流
};