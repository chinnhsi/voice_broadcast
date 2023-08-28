#pragma once
#include <iostream>
#include <string>
using std::cout;
using std::endl;
using std::string;
class Log
{
  string moduleName;
  unsigned short logLevel;
  static void Loginfo(const string& moduleName, const string& type, const string& value);
  static Log* CreateLog(string moduleName, unsigned short logLevel);
  Log(){};

  public:
  void Info(const string& value);    // 0
  void Warn(const string& value);    // 0 1
  void Error(const string& value);   // 0 1 2
  void ResetModuleName(string moduleName);
  static Log* Bind(string moduleName);
  static Log* Bind(string moduleName, unsigned short logLevel);
};