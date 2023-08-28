#pragma once
#include <Windows.h>
#include <Mmdeviceapi.h>
#include <initguid.h>
#include <mmeapi.h>
#include <winerror.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <Audioclient.h>
#include <iostream>
#include <string>
#include <vector>

using std::string;
using std::cout;
using std::endl;

enum DeviceType
{
  Device_RENDER,
  Device_CAPTURE
};

class DeviceInfo
{
  public:
  string name;
  string id;
  DeviceType type;
  IMMDevice* device;
};

class AudioEndpointCollection
{
  HRESULT hr;
  bool initSuccess;

  public:
  AudioEndpointCollection();
  bool GetDeviceList(std::vector<DeviceInfo>& ret, DeviceType type = Device_CAPTURE);
  bool GetDefaultDevice(DeviceInfo& info, DeviceType type = Device_CAPTURE);
  ~AudioEndpointCollection();
};