/* Copyright(C) 2007-2024 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __HELPER_STRING_H
#define __HELPER_STRING_H

#include <vector>
#include <string>
#include <sstream>

class XcapHelper
{
public:
    static std::string buildBuddyList(const std::string& listName, const std::vector<std::string>& buddies);
    static std::string buildRules(const std::vector<std::string>& buddies);
    static std::string buildServices(const std::string& serviceUri, const std::string& listRef);
    static std::string normalizeSipUri(const std::string& uri);
};


#endif
