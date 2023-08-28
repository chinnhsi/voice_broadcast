#include "h/Audio/AudioEndpointCollection.h"
#include "h/Helper.h"
#include <cstddef>
#include <winerror.h>

AudioEndpointCollection::AudioEndpointCollection()
{
  // 初始化组件
  hr = CoInitialize(NULL);
  if (FAILED(hr)) {
    cout << "Failed to initialize COM library" << endl;
    initSuccess = false;
  }
  else {
    initSuccess = true;
  }
}
AudioEndpointCollection::~AudioEndpointCollection()
{
  CoUninitialize();
}
bool AudioEndpointCollection::GetDeviceList(std::vector<DeviceInfo>& ret, DeviceType type)
{
  if (!initSuccess) return false;
  IMMDeviceEnumerator* pEnumerator       = NULL;
  IMMDeviceCollection* pDeviceCollection = NULL;

  // 获取设备枚举器pEnumerator
  hr = CoCreateInstance(__uuidof(MMDeviceEnumerator),
                        NULL,
                        CLSCTX_ALL,
                        __uuidof(IMMDeviceEnumerator),
                        (void**)&pEnumerator);
  if (FAILED(hr)) {
    std::cout << "Failed to create device enumerator" << std::endl;
    return false;
  }

  // 获取音频设备集合pDeviceCollection
  hr = pEnumerator->EnumAudioEndpoints(
      type == Device_CAPTURE ? eCapture : eRender, DEVICE_STATE_ACTIVE, &pDeviceCollection);
  if (FAILED(hr)) {
    std::cout << "Failed to enumerate audio endpoints" << std::endl;
    pEnumerator->Release();
    return false;
  }
  // 获取集合大小deviceCount
  unsigned int deviceCount;
  hr = pDeviceCollection->GetCount(&deviceCount);
  if (FAILED(hr)) {
    std::cout << "Failed to get device count" << std::endl;
    pDeviceCollection->Release();
    pEnumerator->Release();
    return false;
  }
  ret = std::vector<DeviceInfo>(deviceCount);

  // foreach device
  for (unsigned int i = 0; i < deviceCount; i++) {
    IMMDevice* pDevice;
    hr = pDeviceCollection->Item(i, &pDevice);
    if (FAILED(hr)) continue;
    // 获取设备ID
    LPWSTR deviceID = NULL;
    hr              = pDevice->GetId(&deviceID);
    if (FAILED(hr)) continue;
    // 获取设备属性Store
    IPropertyStore* pPropertyStore;
    hr = pDevice->OpenPropertyStore(STGM_READ, &pPropertyStore);
    if (FAILED(hr)) {
      pPropertyStore->Release();
      continue;
    }
    // 获取名称
    PROPVARIANT name;
    PropVariantInit(&name);
    hr = pPropertyStore->GetValue(PKEY_Device_FriendlyName, &name);
    if (FAILED(hr)) {
      pPropertyStore->Release();
      PropVariantClear(&name);
      continue;
    }

    ret[i].device = pDevice;
    ret[i].name   = Helper::ConvertUnicodeToString(name.pwszVal);
    ret[i].id     = Helper::ConvertUnicodeToString(deviceID);
    ret[i].type   = type;
    pPropertyStore->Release();
    PropVariantClear(&name);
  }
  pDeviceCollection->Release();
  pEnumerator->Release();
  return true;
}

bool AudioEndpointCollection::GetDefaultDevice(DeviceInfo& info, DeviceType type)
{
  if (!initSuccess) return NULL;
  IMMDeviceEnumerator* pEnumerator = NULL;
  IMMDevice* pDevice               = NULL;
  IPropertyStore* pPropertyStore;
  PROPVARIANT name;
  PropVariantInit(&name);
  LPWSTR deviceID = NULL;

  // 获取设备枚举器pEnumerator
  hr = CoCreateInstance(__uuidof(MMDeviceEnumerator),
                        NULL,
                        CLSCTX_ALL,
                        __uuidof(IMMDeviceEnumerator),
                        (void**)&pEnumerator);
  if (FAILED(hr)) goto error;
  hr = pEnumerator->GetDefaultAudioEndpoint(
      type == Device_CAPTURE ? eCapture : eRender, eConsole, &pDevice);
  // 获取设备ID
  hr = pDevice->GetId(&deviceID);
  if (FAILED(hr)) goto error;
  // 获取设备属性Store
  hr = pDevice->OpenPropertyStore(STGM_READ, &pPropertyStore);
  if (FAILED(hr)) goto error;
  // 获取名称
  hr = pPropertyStore->GetValue(PKEY_Device_FriendlyName, &name);
  if (FAILED(hr)) goto error;
  info.device = pDevice;
  info.name   = Helper::ConvertUnicodeToString(name.pwszVal);
  info.id     = Helper::ConvertUnicodeToString(deviceID);
  info.type   = type;
  PropVariantClear(&name);
  pPropertyStore->Release();
  pEnumerator->Release();
  return true;
error:
  PropVariantClear(&name);
  pPropertyStore->Release();
  pEnumerator->Release();
  return false;
}
