/* -*- Mode: c++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2023 RPf <gpl3@pfeifer-syscon.de>
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

#include "NetConnection.hpp"

NetAddress::NetAddress(const std::string& addr)
{
    if (addr.length() == 8) {   // ipv4
        auto uIp = static_cast<uint32_t>(std::stol(addr, nullptr, 16));  // parse as long as toi fails for values out of signed integer range
        auto bIp = reinterpret_cast<guint8*>(&uIp);
        m_address = Gio::InetAddress::create(bIp, Gio::SocketFamily::SOCKET_FAMILY_IPV4);
        //std::cout << "ip4 " <<  m_address->to_string() << " in " << addr << std::endl;
    }
    else {                      // ipv6
        std::array<uint32_t, 4> uIp;
        for (uint32_t i = 0; i < uIp.size(); ++i) {
            uIp[i] = static_cast<uint32_t>(std::stol(addr.substr(i*8, 8), nullptr, 16));
        }
        auto bIp = reinterpret_cast<guint8*>(&uIp);
        m_address = Gio::InetAddress::create(bIp, Gio::SocketFamily::SOCKET_FAMILY_IPV6);
        //std::cout << "ip6 " << m_address->to_string() << " in " << addr << std::endl;
    }
}

std::string
NetAddress::getName()
{
    if (m_name.empty()) {
        auto resolver = Gio::Resolver::get_default();
        try {
            m_name = resolver->lookup_by_address(m_address);
        }
        catch (const Glib::Error& e) {
            m_name = m_address->to_string();
            m_ip = true;
        }
    }
    return m_name;
}


std::vector<Glib::ustring>
NetAddress::getNameSplit()
{
    if (m_splitedName.empty()) {
        if (m_ip) {
            if (m_address->get_family() == Gio::SocketFamily::SOCKET_FAMILY_IPV6) {
                auto addr = getName();
                int dotcount = 0;
                for (auto i = addr.begin(); i != addr.end(); ++i) {
                    auto c = *i;
                    if (c == ':') {
                        ++dotcount;
                    }
                }
                auto pos = addr.find("::");
                if (pos != addr.npos) {
                    ++pos;
                    addr.insert(pos, "0");    // there might be multiple blocks... but keep it simple for now as our searching will break with empty parts...
                }
                StringUtils::split(addr, ':', m_splitedName);
            }
            else {
                std::vector<Glib::ustring> parts;   // ipv4 are noted from most to least significant part
                StringUtils::split(getName(), '.', parts);
                std::reverse(parts.begin(), parts.end());
                m_splitedName = parts;
            }
        }
        else {
            StringUtils::split(getName(), '.', m_splitedName);
        }
    }
    return m_splitedName;
}


NetConnection::NetConnection()
{
}

NetConnection::NetConnection(
              const std::string& localIp
            , const std::string& remoteIp
            , const std::string& status)
{
    try {
        m_status = std::stoi(status, nullptr, 16);
        auto pos = localIp.find(':');
        if (pos != localIp.npos) {
            m_localIp = std::make_shared<NetAddress>(localIp.substr(0, pos));
            ++pos;
            m_localPort = std::stoi(localIp.substr(pos), nullptr, 16);
        }
        pos = remoteIp.find(':');
        if (pos != remoteIp.npos) {
            m_remoteIp = std::make_shared<NetAddress>(remoteIp.substr(0, pos));
            ++pos;
            m_remotePort = std::stoi(remoteIp.substr(pos), nullptr, 16);
        }
    }
    catch (const std::exception& ex) {
        std::cout << __FILE__ << "::NetConnection"
                  << " error " << ex.what()
                  << " parsing " << localIp
                  << " " << remoteIp
                  << " " << status << std::endl;
    }
}

bool
NetConnection::isValid()
{
    return m_localIp
       && m_remoteIp
       && m_localPort > 0u
       && m_remotePort > 0u     // for listening this might be valid ...
       && m_status > 0u;
}

std::shared_ptr<NetAddress>
NetConnection::getLocalAddr()
{
    return m_localIp;
}

uint32_t
NetConnection::getLocalPort() const
{
    return m_localPort;
}

std::shared_ptr<NetAddress>
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

std::string
NetConnection::ip(uint32_t iip)
{
    auto h2 = iip & 0x000000ffu;        // requires network byte order
    auto h1 = (iip & 0x0000ff00u) >> 8u;
    auto l2 = (iip & 0x00ff0000u) >> 16u;
    auto l1 = (iip & 0xff000000u) >> 24u;
    auto sip = Glib::ustring::sprintf("%d.%d.%d.%d", h2, h1, l2, l1);
    //std::cout << "ip " << std::hex << iip << std::dec
    //          << " sip " << sip << std::endl;
    return sip;
}

/**
 * this is used for the well known service name
 *   for outgoing the remote port
 *   for incomming the local port
 **/
std::string
NetConnection::getServiceName() {
    return m_remoteSericeName;
}

void
NetConnection::setServiceName(const std::string& serviceName)
{
    m_remoteSericeName = serviceName;
}

