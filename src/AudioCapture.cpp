#include "./h/Audio/AudioCapture.h"
#include <string>
bool AudioCapture::GetCaptureService()
{
  if (!isInitialized) return false;
  hr = pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&pAudioCaptureClient);
  EXIT_IF_ERROR(hr, "Failed to get capture service.")
  return true;
EXIT:
  return false;
}

void AudioCapture::StartCaptureStream()
{
  log.Info("start to capture");
  if (!isInitialized) return;
  if (pAudioCaptureClient == NULL) return;
  AudioCaptureEvent& ev = *new AudioCaptureEvent(pAudioClient,
                                                 pAudioCaptureClient,
                                                 sleepTime,
                                                 bufferFrameCount,
                                                 captureCallback,
                                                 pWaveFormat,
                                                 log,
                                                 stopSignal);
  threadHandle = CreateThread(NULL, 0, AudioCapture::StartCaptureStreamThread, (void*)&ev, 0, NULL);
}

DWORD WINAPI AudioCapture::StartCaptureStreamThread(LPVOID lpParameter)
{
  AudioCaptureEvent& ev                     = *(AudioCaptureEvent*)lpParameter;
  IAudioClient*& pAudioClient               = ev.pAudioClient;
  IAudioCaptureClient*& pAudioCaptureClient = ev.pAudioCaptureClient;
  UINT32& sleepTime                         = ev.sleepTime;
  UINT32& bufferFrameCount                  = ev.bufferFrameCount;
  AudioCaptureCallbackFunction& callback    = ev.callback;
  WAVEFORMATEX*& pWaveFormat                = ev.pWaveFormat;
  Log& log                                  = ev.log;
  bool& stopSignal                          = ev.stopSignal;

  delete &ev;

  UINT32 packetLength = 0;
  BYTE* pData;                 // 音频流buffer
  BYTE pBuffer[65532];         // 临时存储buffer
  UINT32 numFramesAvailable;   // 实际avaliable frame
  UINT32 len;                  // buffer size
  UINT32 i;                    // buffer size counter
  DWORD flags;

  HRESULT hr = pAudioClient->Start();
  EXIT_IF_ERROR(hr, "Failed to start capture stream.")

  while (!stopSignal) {
    Sleep(sleepTime);
    hr = pAudioCaptureClient->GetNextPacketSize(&packetLength);
    EXIT_IF_ERROR(hr, "Failed to get next packet size(1).")
    i = 0;
    while (packetLength != 0) {
      hr = pAudioCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, NULL, NULL);
      EXIT_IF_ERROR(hr, "Failed to get buffer.")
      len = numFramesAvailable * pWaveFormat->nBlockAlign;
      if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {   // 静音流,置0
        memset(pBuffer + i, 0, len);
        pData = NULL;
      }
      else {
        memcpy(pBuffer + i, pData, len);
      }
      i += len;
      hr = pAudioCaptureClient->ReleaseBuffer(numFramesAvailable);
      EXIT_IF_ERROR(hr, "Failed to release buffer.")

      hr = pAudioCaptureClient->GetNextPacketSize(&packetLength);
      EXIT_IF_ERROR(hr, "Failed to get next packet size(2).")
    }
    AudioBufferPack pack(pBuffer, i);
    callback(pack);
  }
  hr = pAudioClient->Stop();
  EXIT_IF_ERROR(hr, "Failed to stop capture stream.")
EXIT:
  stopSignal = false;
  return 0L;
}

void AudioCapture::SetCaptureStreamCallback(AudioCaptureCallbackFunction captureCallback)
{
  this->captureCallback = captureCallback;
}