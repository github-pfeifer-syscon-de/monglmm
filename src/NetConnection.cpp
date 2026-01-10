/* -*- Mode: c++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2023 RPf 
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <StringUtils.hpp>
#include <netdb.h>
#include <psc_format.hpp>
#include <Log.hpp>

#include "NetConnection.hpp"

// use cache as there seems not much caching is involved in default lookup
std::map<std::string, pNetAddress> NetConnection::m_nameCache;



NetAddress::NetAddress(const std::string& addr)
{
    if (addr.length() == 8) {   // ipv4
        auto uIp = static_cast<uint32_t>(std::stoul(addr, nullptr, 16));  // parse as long as toi fails for values out of signed integer range
        auto bIp = reinterpret_cast<guint8*>(&uIp);
        m_address = Gio::InetAddress::create(bIp, Gio::SocketFamily::SOCKET_FAMILY_IPV4);
        //std::cout << "ip4 " <<  m_address->to_string() << " in " << addr << std::endl;
    }
    else {                      // ipv6
        std::array<uint32_t, 4> uIp;
        for (uint32_t i = 0; i < uIp.size(); ++i) {
            uIp[i] = static_cast<uint32_t>(std::stoul(addr.substr(i*8, 8), nullptr, 16));
        }
        auto bIp = reinterpret_cast<guint8*>(&uIp);
        m_address = Gio::InetAddress::create(bIp, Gio::SocketFamily::SOCKET_FAMILY_IPV6);
        //std::cout << "ip6 " << m_address->to_string() << " in " << addr << std::endl;
    }
}

void
NetAddress::address_lookup_ready(const Glib::RefPtr<Gio::AsyncResult>& result)
{
    bool set{false};
    try {
        auto resolver = Gio::Resolver::get_default();
        auto name = resolver->lookup_by_address_finish(result);
        if (!name.empty()) {
            m_name = name;
            set = true;
            m_splitedName.clear();  // rebuild with corrected name
        }
    }
    catch (const Glib::Error& e) {
        // we get an error if address was not "resolvable" -> happens frequently
    }
    if (!set) {
        m_name = getIpAsString();
        m_ip = true;
    }
}

void
NetAddress::lookup()
{
    if (!m_querying) {  // only try once
        try {
            m_querying = true;
            auto resolver = Gio::Resolver::get_default();
            resolver->lookup_by_address_async(m_address,
                    sigc::mem_fun(*this, &NetAddress::address_lookup_ready));
        }
        catch (const Glib::Error& e) {
            std::cout << "NetAddress::lookup error " << e.what() << std::endl;
        }
    }
}

Glib::ustring
NetAddress::getName()
{
    return m_name;
}

Glib::ustring
NetAddress::getIpAsString()
{
    return m_address->to_string();
}

void
NetAddress::setTouched(gint64 touched)
{
    m_touched = touched;
}

gint64
NetAddress::getTouched()
{
    return m_touched;
}

std::vector<Glib::ustring>
NetAddress::getNameSplit()
{
    if (m_splitedName.empty()) {
        //std::cout << __FILE__ << "::getNameSplit"
        //          << " addr " << m_address->to_string()
        //          << " name " << getName()
        //          << " isIp " << (m_ip ? "yes" : "no")
        //          << " fam " << (m_address->get_family() == Gio::SocketFamily::SOCKET_FAMILY_IPV4 ? "ipv4" : m_address->get_family() == Gio::SocketFamily::SOCKET_FAMILY_IPV6 ? "ipv6" :Glib::ustring::sprintf("%d", m_address->get_family()))
        //          << std::endl;
        auto name = getName();
        bool isIp = m_ip;
        if (name.empty()) {
            name = getIpAsString();
            isIp = true;
        }
        if (isIp) {
            // keep it simple, especially ipv6 will take up a relatively large room if splitted (and not showing much)
            m_splitedName.push_back(name);
//            if (m_address->get_family() == Gio::SocketFamily::SOCKET_FAMILY_IPV6) {
//                auto addr = getName();
//                int dotcount = 0;
//                for (auto i = addr.begin(); i != addr.end(); ++i) {
//                    auto c = *i;
//                    if (c == ':') {
//                        ++dotcount;
//                    }
//                }
//                auto pos = addr.find("::");
//                if (pos != addr.npos) {
//                    ++pos;
//                    addr.insert(pos, "0");    // there might be multiple blocks... but keep it simple for now as our searching will break with empty parts...
//                }
//                StringUtils::split(addr, ':', m_splitedName);
//                std::reverse(m_splitedName.begin(), m_splitedName.end());
//            }
//            else {
//                std::vector<Glib::ustring> parts;   // ipv4 are noted from most to least significant part
//                StringUtils::split(getName(), '.', parts);
//                std::reverse(parts.begin(), parts.end());
//                m_splitedName = parts;
//            }
        }
        else {
            StringUtils::split(name, '.', m_splitedName);
        }
        //for (auto part : m_splitedName) {
        //    std::cout << part << " , ";
        //}
        //std::cout << "--------" << std::endl;
    }
    return m_splitedName;
}

Glib::RefPtr<Gio::InetAddress>
NetAddress::getAddress()
{
    return m_address;
}

bool
NetAddress::isValid()
{
    return m_address.operator bool();
}


NetConnection::NetConnection(const std::vector<Glib::ustring>& parts, gint64 now)
{
    auto localIpPort = parts[1];
    auto remoteIpPort = parts[2];
    auto status = parts[3];
    //auto inodeAddr = parts[11]; // the inode & addr changes if state changes to WAIT (6)
    m_key = localIpPort + "_" + remoteIpPort;   // do not include state -> it will change
    try {
        m_status = std::stoi(status, nullptr, 16);
        auto pos = localIpPort.find(':');
        if (pos != localIpPort.npos) {
            m_localIp = findAddress(localIpPort.substr(0, pos), now);
            ++pos;
            m_localPort = std::stoi(localIpPort.substr(pos), nullptr, 16);
        }
        pos = remoteIpPort.find(':');
        if (pos != remoteIpPort.npos) {
            m_remoteIp = findAddress(remoteIpPort.substr(0, pos), now);
            ++pos;
            m_remotePort = std::stoi(remoteIpPort.substr(pos), nullptr, 16);
        }
    }
    catch (const std::exception& ex) {
        psc::log::Log::logAdd(psc::log::Level::Notice, [&] {
            return psc::fmt::format("parsing conn local {} remote {} stat {}"
                    ,localIpPort, remoteIpPort, status, ex);
        });
    }
}

bool
NetConnection::isValid()
{
    return m_localIp && m_localIp->isValid()
       && m_remoteIp && m_remoteIp->isValid()
       && m_localPort > 0u
       && m_remotePort > 0u     // for listening this is a valid value...
       && m_status > 0u;
}

pNetAddress
NetConnection::findAddress(const Glib::ustring& addrHex, gint64 now)
{
    pNetAddress addr;
    auto cachedName = m_nameCache.find(addrHex);
    if (cachedName == m_nameCache.end()) {
        addr = std::make_shared<NetAddress>(addrHex);
        m_nameCache.insert(std::make_pair(addrHex, addr));
    }
    else {
        addr = (*cachedName).second;
    }
    addr->setTouched(now);
    return addr;
}

void
NetConnection::cleanAddressCache(gint64 now)
{
    constexpr auto removeDelayUs = 30l * G_TIME_SPAN_MINUTE;    // 30min
    for (auto iter = m_nameCache.begin(); iter != m_nameCache.end(); ) {
        auto& addr = (*iter).second;
        if (now - addr->getTouched() > removeDelayUs) {
            iter = m_nameCache.erase(iter);
        }
        else {
            ++iter;
        }
    }
}

// use well known port names, not a randomly chosen
uint32_t
NetConnection::getWellKnownPort()
{
    return isIncomming()
            ? getLocalPort()
            : getRemotePort();
}

std::string
NetConnection::getGroupPrefix()
{
    return Glib::ustring::sprintf("%04x", getWellKnownPort());
}

// add some suffix for grouping
std::string
NetConnection::getGroupSuffix()
{
    return Glib::ustring(isIncomming()
                ? "<"
                : ">");
}

const pNetAddress
NetConnection::getLocalAddr()
{
    return m_localIp;
}

uint32_t
NetConnection::getLocalPort() const
{
    return m_localPort;
}

const pNetAddress
NetConnection::getRemoteAddr()
{
    return m_remoteIp;
}

uint32_t
NetConnection::getRemotePort() const
{
    return m_remotePort;
}

uint32_t
NetConnection::getStatus() const
{
    return m_status;
}

void
NetConnection::setStatus(uint32_t status)
{
    m_status = status;
}

bool
NetConnection::isIncomming() const
{
    return m_incomming;
}

void
NetConnection::setIncomming(bool incomming)
{
    m_incomming = incomming;
}

/**
 * this is used for the well known service name
 *   for outgoing the remote port
 *   for incomming the local port
 **/
std::string
NetConnection::getServiceName()
{
    return m_remoteServiceName;
}

void
NetConnection::setServiceName(const std::string& serviceName)
{
    m_remoteServiceName = serviceName;
}

void
NetConnection::setTouched(bool touched)
{
    m_touched = touched;
}

bool
NetConnection::isTouched()
{
    return m_touched;
}

const std::string&
NetConnection::getKey()
{
    return m_key;
}
