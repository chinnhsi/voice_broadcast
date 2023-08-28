#pragma once
#include <string>
#include "NetServerPool.h"
#include "NetClientPool.h"
#include <vector>

using std::string;
using std::vector;
/*
  heartbeat
  start
  end
*/
class NetController
{
  bool HeartBeat();
  vector<string> addressList;
  NetClientPool clientPool;
  NetServerPool serverPool;

  public:
  NetController();
};