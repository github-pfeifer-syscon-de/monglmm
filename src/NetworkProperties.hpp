/* -*- Mode: c++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * Copyright (C) 2024 RPf 
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

#include <gtkmm.h>
#include <KeyfileTableManager.hpp>

#include "BaseNetInfo.hpp"


class ProcessNetInfo
: public BaseNetInfo
{
public:
    ProcessNetInfo(const std::string& basePath)
    : m_basePath{basePath}
    {
    }
    virtual ~ProcessNetInfo() = default;
protected:
    std::string getBasePath() override {
        return m_basePath;
    };
private:
    std::string m_basePath;
};

class PortConverter
: public psc::ui::CustomConverter<uint32_t>
{
public:
    PortConverter(Gtk::TreeModelColumn<uint32_t>& col, const std::shared_ptr<ProcessNetInfo>& processNetInfo)
    : psc::ui::CustomConverter<uint32_t>(col)
    , m_processNetInfo{processNetInfo}
    {
    }
    virtual ~PortConverter() = default;

    void convert(Gtk::CellRenderer* rend, const Gtk::TreeModel::iterator& iter) override {
        uint32_t value = 0;
        iter->get_value(m_col.index(), value);
        auto service  = m_processNetInfo->getServiceName(value);
        auto textRend = static_cast<Gtk::CellRendererText*>(rend);
        textRend->property_text() = service;
     }
private:
    std::shared_ptr<ProcessNetInfo> m_processNetInfo;
};

class NetworkColumns
: public psc::ui::ColumnRecord
{
public:
    Gtk::TreeModelColumn<Glib::ustring> m_direction;
    Gtk::TreeModelColumn<Glib::ustring> m_addr;
    Gtk::TreeModelColumn<uint32_t> m_service;
    //Gtk::TreeModelColumn<glong> m_rxPackets;
    //Gtk::TreeModelColumn<glong> m_txPackets;
    Gtk::TreeModelColumn<pNetConnect> m_netConnect;

    NetworkColumns(const std::shared_ptr<ProcessNetInfo>& processNetInfo)
    {
        add<Glib::ustring>("Direction", m_direction);
        add<Glib::ustring>("Address", m_addr);
        auto portConverter = std::make_shared<PortConverter>(m_service, processNetInfo);
        add<uint32_t>("Service", portConverter);
        // failed to get some more infos, if you know how this works let me know
        //add<glong>("RxPackets", m_rxPackets, 1.0f);
        //add<glong>("TxPackets", m_txPackets, 1.0f);
        // keep this as last, for not confuse indexing of additional infos
        Gtk::TreeModel::ColumnRecord::add(m_netConnect);
    }
    virtual ~NetworkColumns() = default;
};

class NetworkProperties
: public Gtk::Dialog
{
public:
    NetworkProperties(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder, const long processId, Glib::KeyFile* keyFile, int32_t update_interval);
    explicit NetworkProperties(const NetworkProperties& orig) = delete;
    virtual ~NetworkProperties() = default;

    static NetworkProperties* show(const long processId, Glib::KeyFile* keyFile, int32_t update_interval);
protected:

    bool refresh();
    void on_response(int response_id);
    void addNetConnect(const Gtk::TreeModel::iterator& i, pNetConnect& netConnect);

    static constexpr auto CONFIG_GRP = "NetworkProperties";
private:
    std::shared_ptr<ProcessNetInfo> m_processNetInfo;
    long m_processId;
    Glib::RefPtr<Gtk::TreeStore> m_properties;
    sigc::connection m_timer;               /* Timer for regular updates */
    Glib::RefPtr<Gtk::TreeView> m_netTree;
    std::shared_ptr<NetworkColumns> m_propertyColumns;
    std::shared_ptr<psc::ui::KeyfileTableManager> m_kfTableManager;
    std::vector<pNetConnect> m_netConn;
};

