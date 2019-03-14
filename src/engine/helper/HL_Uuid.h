#ifndef __HL_UUID_H
#define __HL_UUID_H

#include <string>

#if defined(TARGET_LINUX) || defined(TARGET_OSX)
# include <uuid/uuid.h>
#endif
#if defined(TARGET_WIN)
# include <rpc.h>
#endif

class Uuid
{
public:
  Uuid();
  static Uuid generateOne();
  static Uuid parse(const std::string& s);
  std::string toString() const;
  bool operator < (const Uuid& right) const;

protected:
#if defined(USE_NULL_UUID)
  unsigned char mUuid[16];
#else

#if defined(TARGET_LINUX) || defined(TARGET_OSX)
  uuid_t mUuid;
#endif
#if defined(TARGET_WIN)
  UUID mUuid;
#endif
#if defined(TARGET_ANDROID)
  // Stub only
#endif
#endif
};


#endif
