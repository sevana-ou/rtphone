#ifndef __AGENT_JSON_H
#define __AGENT_JSON_H

#include <string>

class Agent
{
protected:
  void* mContext;

public:
  Agent();
  ~Agent();
  void write(const std::string& command);
  bool waitForData(int milliseconds);
  std::string read();
};

#endif
