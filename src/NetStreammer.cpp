#include "h/Net/NetStreammer.h"
#include <cstring>
#include <string>

NetStreammer::~NetStreammer()
{
  delete &log;
}
bool NetStreammer::Load(NetStreammerReceiveCallbackFunction cb, string port)
{
  NetStreammerEvent* pEv;
  addrinfo hint  = {};
  addrinfo* addr = NULL;
  int r;
  hint.ai_flags    = AI_PASSIVE | AI_NUMERICHOST;
  hint.ai_family   = AF_INET6;
  hint.ai_socktype = SOCK_DGRAM;
  hint.ai_protocol = IPPROTO_UDP;
  r                = getaddrinfo("::0", port.c_str(), &hint, &addr);
  if (r != 0) {
    EXIT_IF_ERROR("Function 'getaddrinfo' failed: " + std::to_string(r))
  }
  servSocket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
  if (servSocket < 0) {
    EXIT_IF_ERROR("Create socket failed: " + std::to_string(servSocket))
  }
  r = bind(servSocket, addr->ai_addr, addr->ai_addrlen);
  if (r == SOCKET_ERROR) {
    EXIT_IF_ERROR("Function 'bind' failed: " + std::to_string(r))
  }
  recvCallback = cb;
  pEv          = new NetStreammerEvent(recvCallback, servSocket, pool, log);
  accpetThread = CreateThread(NULL, 0, NetStreammer::ReceiveThread, (void*)pEv, 0, NULL);
  freeaddrinfo(addr);
  return true;

EXIT:
  freeaddrinfo(addr);
  servSocket >= 0 && closesocket(servSocket);
  servSocket = -1;
  isLoad     = false;
  if (pEv != NULL) delete &pEv;
  return false;
}
DWORD WINAPI NetStreammer::ReceiveThread(LPVOID lpParameter)
{
  NetStreammerEvent& ev                             = *(NetStreammerEvent*)lpParameter;
  NetStreammerReceiveCallbackFunction& recvCallback = ev.recvCallback;
  int& servSocket                                   = ev.servSocket;
  unordered_map<string, NetStreammerInfo>& pool     = ev.pool;
  Log& log                                          = ev.log;
  delete &ev;

  struct sockaddr_in6 clntAddr;
  int nSize = sizeof(clntAddr);
  char buffer[65536];
  char straddr[INET6_ADDRSTRLEN];
  short r;
  while (1) {
    r = recvfrom(servSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&clntAddr, &nSize);

    if (r < 0) continue;
    inet_ntop(AF_INET6, &clntAddr.sin6_addr, straddr, INET6_ADDRSTRLEN);

    // Check address
    unordered_map<string, NetStreammerInfo>::iterator it = pool.find(string(straddr));
    if (it == pool.end())
      continue;
    else {
      if (r == 0) continue;
      r = CheckUDPMessage(buffer, r);
      if (r == -1) continue;
      // log.Info("Receive stream message from " + it->first + ", length: " + std::to_string(r));
      char* buf = (char*)malloc(r * sizeof(char));
      memcpy(buf, buffer + 12, r);
      recvCallback(it->second, buf, r);
    }
  }
}
short NetStreammer::CheckUDPMessage(const char* buffer, unsigned int length)
{
  /*
    Stream Protocol: UDP
    |←                       4byte                         →|
    ┌───────────────────────────────────────────────────────┐
    │     'V'     │     'O'     │     'I'     │     'C'     │
    ├───────────────────────────────────────────────────────┤
    │           Length          |          unused           │
    ├───────────────────────────────────────────────────────┤
    │                     index(unused)                     │
    ├───────────────────────────────────────────────────────┤
    │                        Buffer                         |
    │                        ......                         |
    └───────────────────────────────────────────────────────┘
  */

  if (length < 6) {
    return -1;
  }
  if (buffer[0] != 'V' || buffer[1] != 'O' || buffer[2] != 'I' || buffer[3] != 'C') {
    return -1;
  }
  short l;
  memcpy(&l, buffer + 4, 2);
  return l;
}
void NetStreammer::PushMessage(const char* buffer, short length)
{
  char* pack = (char*)malloc(sizeof(char) * (length + 12));
  pack[0] = 'V', pack[1] = 'O', pack[2] = 'I', pack[3] = 'C';
  memcpy(pack + 4, &length, 2);
  memset(pack + 6, 0, 6);
  memcpy(pack + 12, buffer, length);

  for (auto i = pool.begin(); i != pool.end(); ++i) {
    struct sockaddr_in6 sockAddr;
    memset(&sockAddr, 0, sizeof(sockAddr));
    sockAddr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, (i->first).c_str(), &sockAddr.sin6_addr);
    sockAddr.sin6_port = htons(atoi("20334"));
    int r              = sendto(servSocket,
                   pack,
                   sizeof(char) * length + 12,
                   0,
                   (struct sockaddr*)&sockAddr,
                   sizeof(sockAddr));
    if (r < 0) {
      log.Error("Send message to " + i->first + " failed! result:" + std::to_string(r));
    }
  }
  delete pack;
}
void NetStreammer::AppendPool(string address)
{
  NetStreammerInfo info;
  info.address = address;
  pool.insert({address, info});
}