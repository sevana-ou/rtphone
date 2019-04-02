/* Copyright(C) 2007-2017 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "HL_VariantMap.h"
#include "HL_Exception.h"
#include <assert.h>

#if defined(TARGET_ANDROID)
# define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>
#include <stdint.h>

Variant::Variant()
{
  mType = VTYPE_INT;
  mInt = 0;
  mInt64 = 0;
  mBool = 0;
  mFloat = 0;
  
  mPointer = NULL;
}

Variant::~Variant()
{
}

Variant::Variant(bool value)
  :mType(VTYPE_BOOL), mBool(value)
{}

Variant::Variant(int value)
  :mType(VTYPE_INT), mInt(value)
{}

Variant::Variant(int64_t value)
  :mType(VTYPE_INT64), mInt64(value)
{}

Variant::Variant(float value)
  :mType(VTYPE_FLOAT), mFloat(value)
{}

Variant::Variant(double value)
  :mType(VTYPE_FLOAT), mFloat((float)value)
{}

Variant::Variant(const std::string& value)
  :mType(VTYPE_STRING), mString(value)
{}

Variant& Variant::operator = (bool value)
{
  mType = VTYPE_BOOL;
  mBool = value;

  return *this;
}

Variant& Variant::operator = (int value)
{
  mType = VTYPE_INT;
  mInt = value;

  return *this;
}

Variant& Variant::operator = (int64_t value)
{
  mType = VTYPE_INT64;
  mInt64 = value;

  return *this;
}

Variant& Variant::operator = (float value)
{
  mType = VTYPE_FLOAT;
  mFloat = value;

  return *this;
}

Variant& Variant::operator = (const std::string& value)
{
  mType = VTYPE_STRING;
  mString = value;

  return *this;
}

Variant& Variant::operator = (const char* value)
{
  mType = VTYPE_STRING;
  mString = value;

  return *this;
}

Variant& Variant::operator = (void* value)
{
  mType = VTYPE_POINTER;
  mPointer = value;

  return *this;
}

Variant& Variant::operator = (PVariantMap map)
{
  mType = VTYPE_VMAP;
  mVMap = map;

  return *this;
}

Variant Variant::operator + (const Variant& rhs)
{
  switch (type())
  {
  case VTYPE_BOOL:
  case VTYPE_INT:     return asInt() + rhs.asInt();
  case VTYPE_INT64:   return asInt64() + rhs.asInt64();
  case VTYPE_FLOAT:   return asFloat() + rhs.asFloat();
  case VTYPE_STRING:  return asStdString() + rhs.asStdString();
  default:
    return false;
  }
}

Variant Variant::operator - (const Variant& rhs)
{
  switch (type())
  {
  case VTYPE_BOOL:
  case VTYPE_STRING:
  case VTYPE_INT:     return asInt() - rhs.asInt();
  case VTYPE_INT64:   return asInt64() - rhs.asInt64();
  case VTYPE_FLOAT:   return asFloat() - rhs.asFloat();

  default:
    return false;
  }
}

Variant Variant::operator * (const Variant& rhs)
{
  switch (type())
  {
  case VTYPE_BOOL:
  case VTYPE_STRING:
  case VTYPE_INT:     return asInt() * rhs.asInt();
  case VTYPE_INT64:   return asInt64() * rhs.asInt64();
  case VTYPE_FLOAT:   return asFloat() * rhs.asFloat();
  default:
    return false;
  }
}

Variant Variant::operator / (const Variant& rhs)
{
  switch (type())
  {
  case VTYPE_BOOL:
  case VTYPE_STRING:
  case VTYPE_INT:     return asInt() / rhs.asInt();
  case VTYPE_INT64:   return asInt64() / rhs.asInt64();
  case VTYPE_FLOAT:   return asFloat() / rhs.asFloat();

  default:
    return false;
  }
}

bool Variant::operator < (const Variant& rhs) const
{
  switch (type())
  {
  case VTYPE_STRING:  return asStdString() < rhs.asStdString();
  case VTYPE_BOOL:
  case VTYPE_INT:     return asInt() < rhs.asInt();
  case VTYPE_INT64:   return asInt64() < rhs.asInt64();
  case VTYPE_FLOAT:   return asFloat() < rhs.asFloat();
  default:
    return false;
  }
}

bool Variant::operator > (const Variant& rhs) const
{
  return !(*this == rhs) && !(*this < rhs);
}

bool Variant::operator == (const Variant& rhs) const
{
  switch (type())
  {
  case VTYPE_STRING:  return asStdString() == rhs.asStdString();
  case VTYPE_BOOL:    return asBool() == rhs.asBool();
  case VTYPE_INT:     return asInt() == rhs.asInt();
  case VTYPE_INT64:   return asInt64() == rhs.asInt64();
  case VTYPE_FLOAT:   return asFloat() == rhs.asFloat();
  case VTYPE_POINTER: return asPointer() == rhs.asPointer();
  case VTYPE_VMAP:    assert(0); break;
  default:
    return false;
  }

  return false;
}

bool Variant::operator != (const Variant& rhs) const
{
  return !(*this == rhs);
}

bool Variant::operator <= (const Variant& rhs) const
{
  return (*this < rhs) || (*this == rhs);
}

bool Variant::operator >= (const Variant& rhs) const
{
  return (*this > rhs) || (*this == rhs);
}

int Variant::asInt() const
{
  if (mType != VTYPE_INT)
    throw Exception(ERR_BAD_VARIANT_TYPE);
  
  return mInt;
}

int64_t Variant::asInt64() const
{
  if (mType != VTYPE_INT64)
    throw Exception(ERR_BAD_VARIANT_TYPE);

  return mInt;
}

bool Variant::asBool() const
{
  switch (mType)
  {
  case VTYPE_INT:
    return mInt != 0;

  case VTYPE_INT64:
    return mInt64 != 0;

  case VTYPE_BOOL:
    return mBool;

  case VTYPE_FLOAT:
    return mFloat != 0;

  case VTYPE_STRING:
    return mString.length() != 0;

  default:
    throw Exception(ERR_BAD_VARIANT_TYPE);
  }
}

float Variant::asFloat() const
{
  switch (mType)
  {
  case VTYPE_INT:
    return (float)mInt;

  case VTYPE_INT64:
    return (float)mInt64;

  case VTYPE_FLOAT:
    return mFloat;

  default:
    throw Exception(ERR_BAD_VARIANT_TYPE);
  }
}


std::string Variant::asStdString() const
{
  char buffer[32];
  switch (mType)
  {
  case VTYPE_STRING:
    return mString;
  
  case VTYPE_INT:
    sprintf(buffer, "%d", mInt);
    return buffer;

  case VTYPE_INT64:
    sprintf(buffer, "%lli", mInt64);
    return buffer;

  case VTYPE_BOOL:
    return mBool ? "true" : "false";

  case VTYPE_FLOAT:
    sprintf(buffer, "%f", mFloat);
    return buffer;

  default:
    throw Exception(ERR_BAD_VARIANT_TYPE);
  }
}

void* Variant::asPointer() const
{
  switch (mType)
  {
  case VTYPE_POINTER:
    return mPointer;
  default:
    throw Exception(ERR_BAD_VARIANT_TYPE);
  }
}

PVariantMap Variant::asVMap()
{
  switch (mType)
  {
  case VTYPE_VMAP:
    return mVMap;
  default:
    throw Exception(ERR_BAD_VARIANT_TYPE);
  }
}

VariantType Variant::type() const
{
  return mType;
}

VariantMap::VariantMap()
{
}

VariantMap::~VariantMap()
{
}

bool VariantMap::empty() const
{
  return mData.empty();
}

void VariantMap::clear()
{
  mData.clear();
}

bool VariantMap::exists(int itemId) const
{
  return mData.find(itemId) != mData.end();
}

Variant& VariantMap::operator [](int itemId)
{
  return mData[itemId];
}

Variant& VariantMap::at(int itemId)
{
  return mData[itemId];
}
