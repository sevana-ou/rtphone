/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __VARIANT_MAP_H
#define __VARIANT_MAP_H

#include <string>
#include <map>
#include "HL_Pointer.h"

enum VariantType
{
  VTYPE_BOOL,
  VTYPE_INT,
  VTYPE_INT64,
  VTYPE_FLOAT,
  VTYPE_STRING,
  VTYPE_POINTER,
  VTYPE_VMAP
};

class VariantMap;
typedef std::shared_ptr<VariantMap> PVariantMap;

class Variant
{
public:
  Variant();
  ~Variant();

  Variant(bool value);
  Variant(int value);
  Variant(int64_t value);
  Variant(float value);
  Variant(double value);
  Variant(const std::string& value);

  Variant& operator = (bool value);
  Variant& operator = (int value);
  Variant& operator = (int64_t value);
  Variant& operator = (float value);
  Variant& operator = (const std::string& value);
  Variant& operator = (const char* value);
  Variant& operator = (void* value);
  Variant& operator = (PVariantMap vmap);

  Variant operator + (const Variant& rhs);
  Variant operator - (const Variant& rhs);
  Variant operator * (const Variant& rhs);
  Variant operator / (const Variant& rhs);
  bool operator < (const Variant& rhs) const;
  bool operator > (const Variant& rhs) const;
  bool operator == (const Variant& rhs) const;
  bool operator != (const Variant& rhs) const;
  bool operator <= (const Variant& rhs) const;
  bool operator >= (const Variant& rhs) const;

  int asInt() const;
  int64_t asInt64() const;
  bool asBool() const;
  float asFloat() const;
  double asDouble() const;
  std::string asStdString() const;
  //const char* asString();
  void* asPointer() const;

  PVariantMap asVMap();

  VariantType type() const;

protected:
  VariantType         mType;
  bool                mBool;
  int                 mInt;
  int64_t             mInt64;
  float               mFloat;
  std::string         mString;
  void*               mPointer;
  PVariantMap         mVMap;
};

class VariantMap
{
public:
  VariantMap();
  ~VariantMap();
  
  bool empty() const;
  void clear();
  bool exists(int itemId) const;

  Variant& operator[](int itemId);
  Variant& at(int itemId);
protected:
  std::map<int, Variant> mData;
};

#endif
