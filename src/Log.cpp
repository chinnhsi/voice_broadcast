#include "h/Log.h"
Log* Log::CreateLog(string moduleName, unsigned short logLevel)
{
  Log* l        = (new Log());
  l->moduleName = moduleName;
  l->logLevel   = logLevel;
  return l;
}
Log* Log::Bind(string moduleName, unsigned short logLevel)
{
  return CreateLog(moduleName, (logLevel > 2 || logLevel < 0) ? 0 : logLevel);
}
Log* Log::Bind(string moduleName)
{
  return CreateLog(moduleName, 0);
}
void Log::Loginfo(const string& moduleName, const string& type, const string& value)
{
  cout << "[" << type << "][" << moduleName << "]" << value << endl;
}

void Log::Info(const string& value)
{
  if (logLevel <= 0) Log::Loginfo(moduleName, "Info", value);
}
void Log::Warn(const string& value)
{
  if (logLevel <= 1) Log::Loginfo(moduleName, "Warn", value);
}
void Log::Error(const string& value)
{
  if (logLevel <= 2) Log::Loginfo(moduleName, "Error", value);
}
void Log::ResetModuleName(string moduleName)
{
  this->moduleName = moduleName;
}