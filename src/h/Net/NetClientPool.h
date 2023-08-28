#pragma once
#include <string>
#include <unordered_map>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "../Log.h"
#pragma comment(lib, "ws2_32.lib");
#define EXIT_IF_ERROR(b, str) \
  if (b) {                    \
    /* log.Error(str);*/      \
    goto EXIT;                \
  }

using std::string;
using std::unordered_map;

class NetClientSocketInfo
{
  friend class NetClientPool;
  NetClientSocketInfo(string address, string port, int socket, bool isConnected, int order,
                      HANDLE recvThreadHandle, HANDLE mutex)
      : address(address)
      , port(port)
      , socket(socket)
      , isConnected(isConnected)
      , order(order)
      , recvThreadHandle(recvThreadHandle)
      , mutex(mutex)
  {}
  string address;
  string port;
  int socket;
  bool isConnected;
  int order;
  HANDLE recvThreadHandle;
  HANDLE mutex;
};

typedef void (*NetClientReceiveCallbackFunction)(NetClientSocketInfo& info, const char* data,
                                                 int length);
class NetClientEvent
{
  friend class NetClientPool;
  NetClientEvent(NetClientSocketInfo& info, NetClientReceiveCallbackFunction& cb, Log& log)
      : info(info)
      , cb(cb)
      , log(log)
  {}
  NetClientSocketInfo& info;
  NetClientReceiveCallbackFunction& cb;
  Log& log;
};

class NetClientPool
{
  unordered_map<int, NetClientSocketInfo> pool;
  int order = 0;
  NetClientReceiveCallbackFunction recvCallback;
  static DWORD WINAPI ReceiveMessageThread(LPVOID lpParameter);
  Log& log = *(Log::Bind("NetClientPool", 0));

  public:
  ~NetClientPool();
  int CreateConnection(string address, string port);
  void SetReceiveCallbackFunction(NetClientReceiveCallbackFunction cb);
  bool PushMessage(NetClientSocketInfo& info, const char* message, int length);
  bool PushMessage(int id, const char* message, int length);
  bool PushMessage(const char* message, int length);
  bool CloseConnection(NetClientSocketInfo& info);
  bool CloseConnection(int id);
  bool CloseConnection();   // Close all connection
};