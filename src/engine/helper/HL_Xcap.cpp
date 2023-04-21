/* Copyright(C) 2007-2023 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "HL_Xcap.h"
#include <sstream>
#include <iomanip>
#include <memory.h>
#include <algorithm>
#include <inttypes.h>

#ifdef TARGET_WIN
# include <WinSock2.h>
# include <Windows.h>
# include <cctype>
#endif

#define XML_HEADER "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>"
// --------------------- XcapHelper -----------------
std::string XcapHelper::buildBuddyList(const std::string& listName, const std::vector<std::string>& buddies)
{
    std::ostringstream result;
    result << XML_HEADER <<
              "<resource-lists xmlns=\"urn:ietf:params:xml:ns:resource-lists\">" <<
              "<list name=\"" << listName.c_str() << "\">";

    // to test CT only!
    //result << "<entry uri=\"" << "sip:dbogovych1@10.11.1.25" << "\"/>";
    //result << "<entry uri=\"" << "sip:dbogovych2@10.11.1.25" << "\"/>";

    for (unsigned i = 0; i<buddies.size(); i++)
    {
        result << "<entry uri=\"" << normalizeSipUri(buddies[i]).c_str() << "\"/>";
    }
    result << "</list></resource-lists>";
    return result.str();
}

std::string XcapHelper::buildRules(const std::vector<std::string>& buddies)
{
    std::ostringstream result;
    result << XML_HEADER <<
              "<ruleset xmlns=\"urn:ietf:params:xml:ns:common-policy\">" <<
              "<rule id=\"presence_allow\">" <<
              "<conditions>";

    for (unsigned i = 0; i<buddies.size(); i++)
    {
        result << "<identity><one id=\"" <<
                  normalizeSipUri(buddies[i]).c_str() << "\"/></identity>";
    }
    result << "</conditions>" <<
              "<actions>" <<
              "<sub-handling xmlns=\"urn:ietf:params:xml:ns:pres-rules\">" <<
              "allow" <<
              "</sub-handling>" <<
              "</actions>" <<
              "<transformations>" <<
              "<provide-devices xmlns=\"urn:ietf:params:xml:ns:pres-rules\">" <<
              "<all-devices/>" <<
              "</provide-devices>" <<
              "<provide-persons xmlns=\"urn:ietf:params:xml:ns:pres-rules\">" <<
              "<all-persons/>" <<
              "</provide-persons>" <<
              "<provide-services xmlns=\"urn:ietf:params:xml:ns:pres-rules\">" <<
              "<all-services/>" <<
              "</provide-services>" <<
              "</transformations>" <<
              "</rule>" <<
              "</ruleset>";
    return result.str();
}

std::string XcapHelper::buildServices(const std::string& serviceUri, const std::string& listRef)
{
    std::ostringstream result;

    result << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"                          << std::endl <<
              "<rls-services xmlns=\"urn:ietf:params:xml:ns:rls-services\""         << std::endl <<
              "xmlns:rl=\"urn:ietf:params:xml:ns:resource-lists\""                  << std::endl <<
              "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">"            << std::endl <<
              "<service uri=\"" << normalizeSipUri(serviceUri).c_str() << "\">"     << std::endl <<
              "<resource-list>" << listRef.c_str() << "</resource-list>"            << std::endl <<
              "<packages>"                                                          << std::endl <<
              "<package>presence</package>"                                         << std::endl <<
              "</packages>"                                                         << std::endl <<
              "</service>"                                                          << std::endl <<
              "</rls-services>";

    return result.str();
}

std::string XcapHelper::normalizeSipUri(const std::string& uri)
{
    std::string t(uri);

    if (t.length())
    {
        if (t[0] == '<')
            t.erase(0,1);
        if (t.length())
        {
            if (t[t.length()-1] == '>')
                t.erase(uri.length()-1, 1);
        }
    }
    return t;
}
