#include "h/Audio/AudioEndpoint.h"

bool AudioEndpoint::Activate()
{
  hr = device->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);
  EXIT_IF_ERROR(hr, "Failed to activate endpoint device")
  return true;
EXIT:
  return false;
}


bool AudioEndpoint::CreateDeviceFormat()
{
  if (pAudioClient == NULL) return false;
  pWaveFormat                  = new WAVEFORMATEX();
  pWaveFormat->wFormatTag      = WAVE_FORMAT_PCM;
  pWaveFormat->nChannels       = 1;
  pWaveFormat->wBitsPerSample  = 8;
  pWaveFormat->nSamplesPerSec  = 32000;
  pWaveFormat->nBlockAlign     = pWaveFormat->nChannels * pWaveFormat->wBitsPerSample / 8;
  pWaveFormat->nAvgBytesPerSec = pWaveFormat->nBlockAlign * pWaveFormat->nSamplesPerSec;
  pWaveFormat->cbSize          = 0;
  cout << "WaveFormat nBlockAlign:" << pWaveFormat->nBlockAlign << endl;
  cout << "WaveFormat wBitsPerSample:" << pWaveFormat->wBitsPerSample << endl;
  cout << "WaveFormat nChannels:" << pWaveFormat->nChannels << endl;
  cout << "WaveFormat nAvgBytesPerSec:" << pWaveFormat->nAvgBytesPerSec << endl;
  cout << "WaveFormat wFormatTag:" << pWaveFormat->wFormatTag << endl;
  cout << "WaveFormat nSamplesPerSec:" << pWaveFormat->nSamplesPerSec << endl;
  cout << "WaveFormat cbSize:" << pWaveFormat->cbSize << endl;
  return true;
}
bool AudioEndpoint::Initialize(DWORD flags)
{
  if (isInitialized) return true;
  if (pAudioClient == NULL) return false;
  if (pWaveFormat == NULL) return false;
  hr = pAudioClient->Initialize(
      AUDCLNT_SHAREMODE_SHARED,
      flags | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
      hnsRequestedDuration,   // 缓冲区大小，100ns=0.1ms
      0,
      pWaveFormat,
      NULL);
  EXIT_IF_ERROR(hr, "Failed to Initialize endpoint stream")

  hr = pAudioClient->GetBufferSize(&bufferFrameCount);
  EXIT_IF_ERROR(hr, "Failed to get endpoint buffer frame size")

  hnsActualDuration = (double)hnsRequestedDuration * bufferFrameCount / pWaveFormat->nSamplesPerSec;
  isInitialized     = true;
  sleepTime         = hnsActualDuration / REFTIMES_PER_MILLISEC / 2;   // 1/2 duration
  cout << "hnsActualDuration:" << hnsActualDuration << endl;
  cout << "bufferFrameCount:" << bufferFrameCount << endl;
  cout << "nSamplesPerSec:" << pWaveFormat->nSamplesPerSec << endl;
  cout << "sleep time:" << sleepTime << endl;
  return true;
EXIT:
  return false;
}

bool AudioEndpoint::Load(DWORD flags)
{
  bool r = Activate();
  if (!r) return r;
  r = CreateDeviceFormat();
  if (!r) return r;
  r = Initialize(flags);
  return r;
}
void AudioEndpoint::StopStream()
{
  stopSignal = true;
}