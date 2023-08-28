#pragma once
#include <string>
#include <unordered_map>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <minwindef.h>
#include "../Log.h"
#define EXIT_IF_ERROR(str) \
  log.Error(str);      \
  goto EXIT;
using std::string;
using std::unordered_map;


class NetStreammerInfo
{
  public:
  string address;
};
typedef void (*NetStreammerReceiveCallbackFunction)(NetStreammerInfo&, char*, short);
class NetStreammerEvent
{
  friend class NetStreammer;
  NetStreammerEvent(NetStreammerReceiveCallbackFunction& recvCallback, int& servSocket,
                    unordered_map<string, NetStreammerInfo>& pool, Log& log)
      : recvCallback(recvCallback)
      , servSocket(servSocket)
      , pool(pool)
      , log(log)
  {}
  NetStreammerReceiveCallbackFunction& recvCallback;
  int& servSocket;
  unordered_map<string, NetStreammerInfo>& pool;
  Log& log;
};
class NetStreammer
{
  unordered_map<string, NetStreammerInfo> pool;
  string port;
  int servSocket;
  bool isLoad = false;
  Log& log    = *(Log::Bind("NetStreammer", 0));
  HANDLE accpetThread;
  NetStreammerReceiveCallbackFunction recvCallback;
  static short CheckUDPMessage(const char* buffer, unsigned int length);

  public:
  bool Load(NetStreammerReceiveCallbackFunction cb, string port);
  void PushMessage(const char* buffer, short length);      // 推送
  static DWORD WINAPI ReceiveThread(LPVOID lpParameter);   // 接收cb
  void AppendPool(string address);
  bool RemovePool(string address);
  ~NetStreammer();
};