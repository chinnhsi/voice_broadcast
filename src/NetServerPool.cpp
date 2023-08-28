#include "h/Net/NetServerPool.h"
#include <winsock2.h>

bool NetServerPool::CreateServer(string port)
{
  addrinfo hint  = {};
  addrinfo* addr = NULL;
  NetServerEvent* pEv;
  int r;

  servSocket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
  EXIT_IF_ERROR(servSocket < 0, "Create socket failed:" + std::to_string(servSocket))

  hint.ai_flags    = AI_PASSIVE | AI_NUMERICHOST;
  hint.ai_family   = AF_INET6;
  hint.ai_socktype = SOCK_STREAM;
  hint.ai_protocol = IPPROTO_TCP;
  r                = getaddrinfo("::0", port.c_str(), &hint, &addr);
  EXIT_IF_ERROR(r != 0, "Function 'getaddrinfo' failed:" + std::to_string(r))

  r = bind(servSocket, addr->ai_addr, addr->ai_addrlen);
  EXIT_IF_ERROR(r == SOCKET_ERROR, "Function 'bind' failed:" + std::to_string(r))

  r = listen(servSocket, 50);
  EXIT_IF_ERROR(r == SOCKET_ERROR, "Function 'listen' failed:" + std::to_string(r))
  pEv = new NetServerEvent(recvCallback, connCallback, servSocket, order, pool, trustAddress, log);
  accpetThread = CreateThread(NULL, 0, NetServerPool::AcceptThread, (void*)pEv, 0, NULL);

  freeaddrinfo(addr);
  return true;
EXIT:
  freeaddrinfo(addr);
  servSocket > 0 && closesocket(servSocket);
  servSocket = -1;
  return false;
}
DWORD WINAPI NetServerPool::AcceptThread(LPVOID lpParameter)
{
  NetServerEvent& ev                             = *(NetServerEvent*)lpParameter;
  NetServerReceiveCallbackFunction& recvCallback = ev.recvCallback;
  NetServerConnectCallbackFunction& connCallback = ev.connCallback;
  int& servSocket                                = ev.servSocket;
  int& order                                     = ev.order;
  unordered_map<int, NetServerSocketInfo>& pool  = ev.pool;
  vector<string>& trustAddress                   = ev.trustAddress;
  Log& log                                       = ev.log;
  delete &ev;

  struct sockaddr_in6 clntAddr;
  int nSize      = sizeof(clntAddr);
  int clientSock = -1;
  NetServerSocketInfo* poolInfo;
  NetServerReceiveEvent* e;

  while (1) {
    clientSock = accept(servSocket, (struct sockaddr*)&clntAddr, &nSize);
    EXIT_IF_ERROR(clientSock < 0, "Failed to accept socket:" + std::to_string(clientSock))
    char straddr[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &clntAddr.sin6_addr, straddr, INET6_ADDRSTRLEN);


    pool.insert({order, {string(straddr), clientSock, true, NULL, CreateMutex(NULL, false, NULL)}});

    poolInfo = &pool.find(order++)->second;

    connCallback(*poolInfo);
    e = new NetServerReceiveEvent(
        {recvCallback, connCallback, servSocket, order, pool, trustAddress, log}, *poolInfo);

    WaitForSingleObject(poolInfo->mutex, 5000);
    poolInfo->recvThreadHandle =
        CreateThread(NULL, 0, NetServerPool::ReceiveThread, (void*)e, 0, NULL);
    ReleaseMutex(poolInfo->mutex);
  }
EXIT:
  return 0L;
}
DWORD WINAPI NetServerPool::ReceiveThread(LPVOID lpParameter)
{
  NetServerReceiveEvent* ev = (NetServerReceiveEvent*)lpParameter;

  NetServerEvent e          = ev->ev;
  NetServerSocketInfo& info = ev->info;
  Log& log                  = e.log;
  delete ev;

  char szBuffer[MAXBYTE] = {0};
  int r;
  while (info.isConnected) {
    r = recv(info.socket, szBuffer, MAXBYTE, NULL);
    EXIT_IF_ERROR(
        r < 0,
        "[ServerRecvThread:" + info.address + "] Function 'recv' failed: " + std::to_string(r))
    e.recvCallback(info, szBuffer, r);
  }
EXIT:
  WaitForSingleObject(info.mutex, 5000);
  closesocket(info.socket);
  info.isConnected = false;
  ReleaseMutex(info.mutex);
  ExitThread(1);
  return 0L;
}


void NetServerPool::SetConnectCallbackFunction(NetServerReceiveCallbackFunction recvCallback)
{
  this->recvCallback = recvCallback;
}
void NetServerPool::SetReceiveCallbackFuntion(NetServerConnectCallbackFunction connCallback)
{
  this->connCallback = connCallback;
}
bool NetServerPool::AppendTrustUser(string address)
{
  vector<string>::iterator it = std::find(trustAddress.begin(), trustAddress.end(), address);
  if (it != trustAddress.end()) {
    return false;
  }
  trustAddress.push_back(address);
  return true;
}
bool NetServerPool::RemoveTrustUser(string address)
{
  vector<string>::iterator it = std::find(trustAddress.begin(), trustAddress.end(), address);
  if (it != trustAddress.end()) {
    it->erase(it->begin(), it->begin() + 1);
    return true;
  }
  return false;
}
bool NetServerPool::PushMessage(NetServerSocketInfo& info, const char* message, int length)
{
  int r = send(info.socket, message, length, NULL);
  EXIT_IF_ERROR(r == SOCKET_ERROR, "Failed to send message" + std::to_string(r))
  return true;
EXIT:
  return false;
}
bool NetServerPool::PushMessage(int id, const char* message, int length)
{
  NetServerSocketInfo& info = pool.find(id)->second;
  return PushMessage(info, message, length);
}
bool NetServerPool::PushMessage(const char* message, int length)
{
  bool isSuccess = true;
  for (auto i = pool.begin(); i != pool.end(); ++i) {
    if (!i->second.isConnected) continue;
    if (!PushMessage(i->second, message, length)) {
      isSuccess = false;
    }
  }
  return isSuccess;
}
bool NetServerPool::CloseConnection(NetServerSocketInfo& info)
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
bool NetServerPool::CloseConnection(int id)
{
  NetServerSocketInfo& info = pool.find(id)->second;
  return CloseConnection(info);
}
bool NetServerPool::CloseConnection()
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
bool NetServerPool::DestoryServer()
{
  servSocket > 0 && closesocket(servSocket);
  servSocket = -1;
  return true;
}
NetServerPool::~NetServerPool()
{
  DestoryServer();
  CloseConnection();
}