#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2ipdef.h>
#include <algorithm>
#include "../Log.h"
#pragma comment(lib, "ws2_32.lib");
#define EXIT_IF_ERROR(b, str) \
  if (b) {                    \
    /*log.Error(str); */      \
    goto EXIT;                \
  }


using std::string;
using std::unordered_map;
using std::vector;

class NetServerSocketInfo
{
  friend class NetServerPool;
  NetServerSocketInfo(string address, int socket, bool isConnected, HANDLE recvThreadHandle,
                      HANDLE mutex)
      : address(address)
      , socket(socket)
      , isConnected(isConnected)
      , recvThreadHandle(recvThreadHandle)
      , mutex(mutex)
  {}
  string address;
  int socket;
  bool isConnected;
  HANDLE recvThreadHandle;
  HANDLE mutex;
};
typedef void (*NetServerReceiveCallbackFunction)(NetServerSocketInfo& info, const char* data,
                                                 int length);
typedef void (*NetServerConnectCallbackFunction)(NetServerSocketInfo& info);

class NetServerEvent
{
  friend class NetServerPool;
  NetServerEvent(NetServerReceiveCallbackFunction& recvCallback,
                 NetServerConnectCallbackFunction& connCallback, int& servSocket, int& order,
                 unordered_map<int, NetServerSocketInfo>& pool, vector<string>& trustAddress,
                 Log& log)
      : recvCallback(recvCallback)
      , connCallback(connCallback)
      , servSocket(servSocket)
      , order(order)
      , pool(pool)
      , trustAddress(trustAddress)
      , log(log)
  {}
  NetServerReceiveCallbackFunction& recvCallback;
  NetServerConnectCallbackFunction& connCallback;
  int& servSocket;
  int& order;
  unordered_map<int, NetServerSocketInfo>& pool;
  vector<string>& trustAddress;
  Log& log;
};
class NetServerReceiveEvent
{
  friend class NetServerPool;
  NetServerReceiveEvent(NetServerEvent ev, NetServerSocketInfo& info)
      : ev(ev)
      , info(info)
  {}
  NetServerEvent ev;
  NetServerSocketInfo& info;
};

class NetServerPool
{
  int servSocket = -1;
  int order      = 0;
  HANDLE accpetThread;
  unordered_map<int, NetServerSocketInfo> pool;
  NetServerReceiveCallbackFunction recvCallback = NULL;
  NetServerConnectCallbackFunction connCallback = NULL;
  vector<string> trustAddress;
  Log& log = *(Log::Bind("NetServerPool", 0));
  static DWORD WINAPI AcceptThread(LPVOID lpParameter);    // 接收client请求
  static DWORD WINAPI ReceiveThread(LPVOID lpParameter);   // 接收client receive

  public:
  ~NetServerPool();
  void SetConnectCallbackFunction(NetServerReceiveCallbackFunction recvCallback);
  void SetReceiveCallbackFuntion(NetServerConnectCallbackFunction connCallback);
  bool CreateServer(string port);
  bool AppendTrustUser(string address);
  bool RemoveTrustUser(string address);
  bool PushMessage(NetServerSocketInfo& info, const char* message, int length);
  bool PushMessage(int id, const char* message, int length);
  bool PushMessage(const char* message, int length);   // Push message to all connection
  bool CloseConnection(NetServerSocketInfo& info);
  bool CloseConnection(int id);
  bool CloseConnection();   // Close all connection
  bool DestoryServer();
};
