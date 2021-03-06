#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/TargetCommand.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace std;


TargetCommand::TargetCommand(Target& target,
                             unique_ptr<Message> message)
   : mTarget(target),
     mMessage(std::move(message))
{
}

TargetCommand::TargetCommand(const TargetCommand& from)
   : mTarget(from.mTarget),
     mMessage(std::move(from.mMessage))
{
}

void TargetCommand::executeCommand()
{
   mTarget.post(std::move(mMessage));
}

Message* TargetCommand::clone() const
{
   return new TargetCommand(*this);
}

EncodeStream&
TargetCommand::encode(EncodeStream& strm) const
{
   return strm;
}

EncodeStream&
TargetCommand::encodeBrief(EncodeStream& strm) const
{
   return strm;
}

TargetCommand::Target::~Target()
{
}
