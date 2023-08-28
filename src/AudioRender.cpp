#include "./h/Audio/AudioRender.h"

bool AudioRender::GetRenderService()
{
  if (!isInitialized) return false;
  hr = pAudioClient->GetService(IID_IAudioRenderClient, (void**)&pAudioRenderClient);
  EXIT_IF_ERROR(hr, "Failed to get render service.")
  return true;
EXIT:
  return false;
}


void AudioRender::StartRenderStream()
{
  if (!isInitialized) return;
  if (pAudioRenderClient == NULL) return;
  AudioRenderBuffer::Reset(buffer);
  AudioRenderEvent& ev = *new AudioRenderEvent(pAudioClient,
                                               pAudioRenderClient,
                                               sleepTime,
                                               bufferFrameCount,
                                               pWaveFormat,
                                               mutex,
                                               buffer,
                                               log,
                                               stopSignal);

  threadHandle = CreateThread(NULL, 0, AudioRender::StartRenderStreamThread, (void*)&ev, 0, NULL);
}
DWORD WINAPI AudioRender::StartRenderStreamThread(LPVOID lpParameter)
{
  AudioRenderEvent& ev = *(AudioRenderEvent*)lpParameter;

  IAudioClient*& pAudioClient             = ev.pAudioClient;
  IAudioRenderClient*& pAudioRenderClient = ev.pAudioRenderClient;
  UINT32& sleepTime                       = ev.sleepTime;
  UINT32& bufferFrameCount                = ev.bufferFrameCount;
  HANDLE& mutex                           = ev.mutex;
  WAVEFORMATEX*& pWaveFormat              = ev.pWaveFormat;
  AudioRenderBuffer& buffer               = ev.buffer;
  Log& log                                = ev.log;
  bool& stopSignal                        = ev.stopSignal;
  // bool& packUsed        = *ev.packUsed;
  // AudioBufferPack& pack = *ev.pack;

  delete &ev;

  HRESULT hr;
  BYTE* pData;                 // 音频流buffer
  UINT32 numFramesPadding;     // 已填充的音频流frame大小
  UINT32 numFramesAvailable;   // 实际可用音频流frame大小
  UINT32 len;                  // 缓冲区buffer byte长度

  hr = pAudioRenderClient->GetBuffer(bufferFrameCount, &pData);
  EXIT_IF_ERROR(hr, "Failed to get first buffer.")

  WaitForSingleObject(mutex, 5000);
  len = bufferFrameCount * pWaveFormat->nBlockAlign;
  CopyBuffer(pData, buffer, len);
  ReleaseMutex(mutex);

  hr = pAudioRenderClient->ReleaseBuffer(bufferFrameCount, 0);

  EXIT_IF_ERROR(hr, "Failed to release first buffer.")

  hr = pAudioClient->Start();
  EXIT_IF_ERROR(hr, "Failed to start render stream.")

  while (!stopSignal) {
    Sleep(sleepTime);

    hr = pAudioClient->GetCurrentPadding(&numFramesPadding);
    EXIT_IF_ERROR(hr, "Failed to get current padding frame count.")

    numFramesAvailable = bufferFrameCount - numFramesPadding;

    hr = pAudioRenderClient->GetBuffer(numFramesAvailable, &pData);
    EXIT_IF_ERROR(hr, "Failed to get buffer.")

    WaitForSingleObject(mutex, 5000);
    len = numFramesAvailable * pWaveFormat->nBlockAlign;
    CopyBuffer(pData, buffer, len);
    ReleaseMutex(mutex);

    hr = pAudioRenderClient->ReleaseBuffer(numFramesAvailable, 0);
    EXIT_IF_ERROR(hr, "Failed to release buffer.")
  }

  Sleep(sleepTime);   // 等待最后一个buffer
  hr = pAudioClient->Stop();
  EXIT_IF_ERROR(hr, "Failed to stop stream.")
EXIT:
  stopSignal = false;
  return 0L;
}

bool inline AudioRender::CopyBuffer(BYTE* pData, AudioRenderBuffer& buffer, UINT32& len)
{
  if (buffer.bufferLen < len) {   // 缓冲大小不够本次，静音
    memset(pData, 0, len);
    return false;
  }
  else {
    if (buffer.usedIndex + len >= MAX_BUFFER_SIZE) {   // 超出buffer边界，分次填充
      UINT32 d = MAX_BUFFER_SIZE - buffer.usedIndex;
      memcpy(pData, buffer.buffer + buffer.usedIndex, d);
      memcpy(pData + d, buffer.buffer, len - d);
      buffer.usedIndex = len - d;
    }
    else {   // 未超出，整段填充
      memcpy(pData, buffer.buffer + buffer.usedIndex, len);
      buffer.usedIndex += len;
    }
    buffer.bufferLen -= len;
    return true;
  }
}