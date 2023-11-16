/* -*- Mode: c++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
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

#pragma once

#include <glibmm.h>
#include <giomm.h>
#include <string>
#include <cstdint>

class NetAddress
{
public:
    NetAddress(const std::string& addr);
    explicit NetAddress(const NetAddress& orig) = delete;
    virtual ~NetAddress() = default;

    Glib::RefPtr<Gio::InetAddress> getAddress();
    std::string getName();
    std::vector<Glib::ustring> getNameSplit();
private:
    Glib::RefPtr<Gio::InetAddress> m_address;
    std::string m_name;
    std::vector<Glib::ustring> m_splitedName;
    bool m_ip{false};
};

class NetConnection
{
public:
    NetConnection(
              const std::string& localIp
            , const std::string& remoteIp
            , const std::string& status);
    explicit NetConnection(const NetConnection& orig) = delete;
    virtual ~NetConnection() = default;

    std::shared_ptr<NetAddress> getLocalAddr();
    std::string getLocalName();
    uint32_t getLocalPort() const;
    std::shared_ptr<NetAddress> getRemoteAddr();
    uint32_t getRemotePort() const;
    std::string getServiceName();
    void setServiceName(const std::string& serviceName);
    bool isValid();
    uint32_t getStatus() const;
    bool isIncomming() const;
    void setIncomming(bool incomming);
    static std::string ip(uint32_t iip);
    uint32_t getWellKnownPort();
    std::string getGroupSuffix();

private:
    std::shared_ptr<NetAddress> m_localIp;
    uint32_t m_localPort{0u};
    std::shared_ptr<NetAddress> m_remoteIp;
    uint32_t m_remotePort{0u};
    std::string m_remoteSericeName;
    uint32_t m_status{0u};
    bool m_incomming{false};
};

