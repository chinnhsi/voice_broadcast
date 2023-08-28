#pragma once
#include <iostream>
#include <minwindef.h>
#include <string>
#include <windows.h>
using std::string;
using std::wstring;
class Helper
{

  static string wstring2string(wstring wstr)
  {
    string result;
    // 获取缓冲区大小，并申请空间，缓冲区大小事按字节计算的
    int len      = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), NULL, 0, NULL, NULL);
    char* buffer = new char[len + 1];
    // 宽字节编码转换成多字节编码
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), buffer, len, NULL, NULL);
    buffer[len] = '\0';
    // 删除缓冲区并返回值
    result.append(buffer);
    delete[] buffer;
    return result;
  }

  public:
  static string ConvertUnicodeToString(wchar_t* v)
  {
    wstring wstr(v);
    string s = Helper::wstring2string(wstr);
    // std::cout << s << std::endl;
    return s;
  }
};