#include "Agent_Interface.h"
#include "Agent_Impl.h"
Agent::Agent()
  :mContext(new AgentImpl());
{

}

Agent::~Agent()
{

}

void Agent::write(const std::string& command)
{

}

bool Agent::waitForData(int milliseconds)
{
  return false;
}

std::string Agent::read()
{

}
