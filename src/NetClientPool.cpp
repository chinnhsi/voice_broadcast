#include "h/Net/NetClientPool.h"

int NetClientPool::CreateConnection(string address, string port)
{
  addrinfo hint    = {};
  hint.ai_flags    = AI_NUMERICHOST;
  hint.ai_family   = AF_INET6;
  hint.ai_socktype = SOCK_STREAM;
  hint.ai_protocol = IPPROTO_TCP;
  addrinfo* addr;
  NetClientSocketInfo* poolInfo;
  NetClientEvent* pEv;
  int sock;
  int r;

  r = getaddrinfo(address.c_str(), port.c_str(), &hint, &addr);
  EXIT_IF_ERROR(r != 0, "Function 'getaddrinfo' failed:" + std::to_string(r))

  sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
  EXIT_IF_ERROR(sock < 0, "Create socket failed:" + std::to_string(sock))

  r = connect(sock, addr->ai_addr, addr->ai_addrlen);

  EXIT_IF_ERROR(r == SOCKET_ERROR, "Function 'connect' failed: " + std::to_string(r))


  pool.insert({order, {address, port, sock, true, order, 0, CreateMutex(NULL, false, NULL)}});

  poolInfo = &pool.find(order)->second;
  pEv      = new NetClientEvent(*poolInfo, recvCallback, log);
  WaitForSingleObject(poolInfo->mutex, 5000);
  poolInfo->recvThreadHandle =
      CreateThread(NULL, 0, NetClientPool::ReceiveMessageThread, (void*)pEv, 0, NULL);
  ReleaseMutex(poolInfo->mutex);

  freeaddrinfo(addr);
  return order++;

EXIT:
  freeaddrinfo(addr);
  sock > 0 && closesocket(sock);
  return -1;
}
DWORD WINAPI NetClientPool::ReceiveMessageThread(LPVOID lpParameter)
{
  NetClientEvent& ev                   = *(NetClientEvent*)lpParameter;
  NetClientSocketInfo& info            = ev.info;
  NetClientReceiveCallbackFunction& cb = ev.cb;
  Log& log                             = ev.log;
  char szBuffer[MAXBYTE]               = {0};
  int r;

  while (info.isConnected) {
    r = recv(info.socket, szBuffer, MAXBYTE, NULL);
    EXIT_IF_ERROR(r < 0,
                  "[RecvThread:" + info.address + "] Function 'recv' failed: " + std::to_string(r))
    cb(info, szBuffer, r);
  }

EXIT:
  WaitForSingleObject(info.mutex, 5000);
  info.isConnected = false;
  closesocket(info.socket);
  ReleaseMutex(info.mutex);
  CloseHandle(info.mutex);
  ExitThread(1);
  return 0L;
}

bool NetClientPool::PushMessage(NetClientSocketInfo& info, const char* message, int length)
{
  int r = send(info.socket, message, length, NULL);
  EXIT_IF_ERROR(r == SOCKET_ERROR, "Failed to send message" + std::to_string(r))
  return true;
EXIT:
  return false;
}
bool NetClientPool::PushMessage(int id, const char* message, int length)
{
  NetClientSocketInfo& info = pool.find(id)->second;
  return PushMessage(info, message, length);
}
bool NetClientPool::PushMessage(const char* message, int length)
{
  bool isSuccess = true;
  for (auto i = pool.begin(); i != pool.end(); ++i) {
    if (!PushMessage(i->second, message, length)) {
      isSuccess = false;
    }
  }
  return isSuccess;
}

bool NetClientPool::CloseConnection(NetClientSocketInfo& info)
{
  if (!info.isConnected) return true;
  WaitForSingleObject(info.mutex, 50000);
  TerminateThread(info.recvThreadHandle, 0);
  info.isConnected = false;
  closesocket(info.socket);
  ReleaseMutex(info.mutex);
  CloseHandle(info.mutex);
  return true;
}
bool NetClientPool::CloseConnection(int id)
{
  NetClientSocketInfo& info = pool.find(id)->second;
  return CloseConnection(info);
}
bool NetClientPool::CloseConnection()
{
  bool isSuccess = true;
  for (auto i = pool.begin(); i != pool.end(); ++i) {
    if (!i->second.isConnected) continue;
    if (!CloseConnection(i->second)) {
      isSuccess = false;
    }
  }
  return isSuccess;
}
void NetClientPool::SetReceiveCallbackFunction(NetClientReceiveCallbackFunction cb)
{
  recvCallback = cb;
}
NetClientPool::~NetClientPool()
{
  CloseConnection();
}