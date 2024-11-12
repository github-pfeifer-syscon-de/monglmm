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

enum class NetAddrState
{
    New
    , Started
    , Done
};

class NetAddress
{
public:
    NetAddress(const std::string& addr);
    explicit NetAddress(const NetAddress& orig) = delete;
    virtual ~NetAddress() = default;

    Glib::RefPtr<Gio::InetAddress> getAddress();
    std::string getName();
    std::vector<Glib::ustring> getNameSplit();
    bool isValid();
    void setTouched(gint64 touched);
    gint64 getTouched();
private:
    void lookup();
    Glib::RefPtr<Gio::InetAddress> m_address;
    std::string m_name;
    std::vector<Glib::ustring> m_splitedName;
    bool m_ip{false};
    gint64 m_touched;
};

typedef std::shared_ptr<NetAddress> pNetAddress;

class NetConnection;

typedef std::shared_ptr<NetConnection> pNetConnect;

class NetConnection
{
public:
    NetConnection(const std::vector<Glib::ustring>& parts, gint64 now);
    explicit NetConnection(const NetConnection& orig) = default;
    virtual ~NetConnection() = default;

    const pNetAddress getLocalAddr();
    uint32_t getLocalPort() const;
    const pNetAddress getRemoteAddr();
    uint32_t getRemotePort() const;
    std::string getServiceName();
    void setServiceName(const std::string& serviceName);
    bool isValid();
    uint32_t getStatus() const;
    void setStatus(uint32_t status);
    bool isIncomming() const;
    void setIncomming(bool incomming);
    uint32_t getWellKnownPort();
    std::string getGroupPrefix();
    std::string getGroupSuffix();
    void setTouched(bool touched);
    bool isTouched();
    const std::string& getKey();
    static void cleanAddressCache(gint64 now);
private:
    static pNetAddress findAddress(const Glib::ustring& addrHex, gint64 now);
    pNetAddress m_localIp;
    uint32_t m_localPort{0u};
    pNetAddress m_remoteIp;
    uint32_t m_remotePort{0u};
    std::string m_remoteServiceName;
    uint32_t m_status{0u};
    bool m_incomming{false};
    gint64 m_touched{false};
    std::string m_key;
    static std::map<std::string, pNetAddress> m_nameCache;
};
