#pragma once
#include <Windows.h>
#include <cstring>
#include <cwchar>
#include <iostream>
#include "h/Audio/AudioEndpointCollection.h"
#include "h/Audio/AudioRender.h"
#include "h/Audio/AudioCapture.h"
#include "h/Net/NetStreammer.h"

using std::cout;
using std::cin;
using std::endl;
NetStreammer* _streammer  = nullptr;
AudioRender* _audioDevice = nullptr;
void RenderStreamUDP(NetStreammerInfo& info, char* buffer, short length)
{
  if (_audioDevice == nullptr) {
    delete[] buffer;
    return;
  }
  int filledIndex, d;
  WaitForSingleObject((*_audioDevice).mutex, 5000);
  filledIndex = (*_audioDevice).buffer.filledIndex;
  if (filledIndex + length > MAX_BUFFER_SIZE) {
    d = MAX_BUFFER_SIZE - filledIndex;
    memcpy((*_audioDevice).buffer.buffer + filledIndex, buffer, d);
    filledIndex = length - d;
    memcpy((*_audioDevice).buffer.buffer, buffer + d, d);
  }
  else {
    memcpy((*_audioDevice).buffer.buffer + (*_audioDevice).buffer.filledIndex, buffer, length);
    filledIndex += length;
  }
  (*_audioDevice).buffer.bufferLen += length;
  (*_audioDevice).buffer.filledIndex = filledIndex % MAX_BUFFER_SIZE;
  ReleaseMutex((*_audioDevice).mutex);
  delete[] buffer;
}
void CaptureStreamUDP(AudioBufferPack& pack)
{
  _streammer->PushMessage(reinterpret_cast<const char*>(pack.pBuffer), pack.size);
}
int main()
{
  // 初始化
  WSADATA wsaData;
  int r = WSAStartup(MAKEWORD(2, 1), &wsaData);
  if (r != 0) {
    cout << "Initialize winsock failed: " << r << endl;
    return 1;
  }
  //--------
  cout << "Initialize AudioEndpointCollection..." << endl;
  AudioEndpointCollection endpointCollection;
  std::vector<DeviceInfo> deviceListRender;
  std::vector<DeviceInfo> deviceListCapture;
  cout << "Get render device..." << endl;
  bool ret = endpointCollection.GetDeviceList(deviceListRender, Device_RENDER);
  if (!ret) {
    cout << "Get render device failed!" << endl;
    return 1;
  }
  cout << "Get capture device..." << endl;
  ret = endpointCollection.GetDeviceList(deviceListCapture);
  if (!ret) {
    cout << "Get capture device failed!" << endl;
    return 1;
  }
  int renderSize = deviceListRender.size(), captureSize = deviceListCapture.size();
  //-------
  cout << "Audio Device List:" << endl;
  int i = 0, selectedDevice;
  for (; i < renderSize; i++) {
    cout << i << ". [Render]" << deviceListRender[i].name << endl;
  }

  for (i = 0; i < captureSize; i++) {
    cout << (i + renderSize) << ". [Capture]" << deviceListCapture[i].name << endl;
  }
  cout << "Switch device:";
  cin >> selectedDevice;
  if (selectedDevice >= renderSize + captureSize || selectedDevice < 0) {
    cout << "Unexpected index:" << selectedDevice << endl;
    return 0;
  }
  DeviceInfo& deviceInfo = selectedDevice < renderSize
                               ? deviceListRender[selectedDevice]
                               : deviceListCapture[selectedDevice - renderSize];
  int role;
  cout << "1. Server-Render(Default) \n2. Client-Capture" << endl;
  cout << "Switch mode:";
  cin >> role;
  NetStreammer streammer;
  _streammer = &streammer;
  streammer.AppendPool("2409:8a62:f28:7e40:3577:d024:d308:6555");
  //-------
  if (role != 2) {   // 1 render
    AudioRender render(deviceInfo);
    streammer.Load(RenderStreamUDP, "20334");
    render.Load(0);
    _audioDevice = &render;
    render.GetRenderService();
    render.StartRenderStream();
  }
  else {   // 2 capture
    AudioCapture capture(deviceInfo);
    streammer.Load(RenderStreamUDP, "20333");
    capture.Load(selectedDevice >= renderSize ? 0 : AUDCLNT_STREAMFLAGS_LOOPBACK);
    capture.GetCaptureService();
    capture.SetCaptureStreamCallback(CaptureStreamUDP);
    capture.StartCaptureStream();
  }

  Sleep(10000000);
  return 0;
}