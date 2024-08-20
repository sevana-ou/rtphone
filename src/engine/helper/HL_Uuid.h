#ifndef __HL_UUID_H
#define __HL_UUID_H

#include <string>
#include <stdint.h>

class Uuid
{
public:
    Uuid();
    static Uuid generateOne();
    static Uuid parse(const std::string& s);
    std::string toString() const;
    bool operator < (const Uuid& right) const;

protected:
    uint8_t mUuid[16];
};


#endif
