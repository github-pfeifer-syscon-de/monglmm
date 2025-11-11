/* -*- Mode: c++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2024 RPf <gpl3@pfeifer-syscon.de>
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
#include <Log.hpp>


#include "NetworkProperties.hpp"

// using the processId was a idea i had when i discovered that
//   these infos where available in a subdirectory from process,
//   but as it seems these are just the global infos...
//   but maybe there is an option, or these will be available in the future
NetworkProperties::NetworkProperties(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder, const long processId, Glib::KeyFile* keyFile, int32_t update_interval)
: Gtk::Dialog{cobject}
, m_processNetInfo{std::make_shared<ProcessNetInfo>(Glib::ustring::sprintf("/proc/%ld/net", processId))}
, m_processId{processId}
, m_propertyColumns{std::make_shared<NetworkColumns>(m_processNetInfo)}
{
    auto object = builder->get_object("properties");
    m_netTree = Glib::RefPtr<Gtk::TreeView>::cast_dynamic(object);
    if (m_netTree) {
        m_properties = Gtk::TreeStore::create(*m_propertyColumns);
        m_netTree->set_model(m_properties);
        m_kfTableManager = std::make_shared<psc::ui::KeyfileTableManager>(m_propertyColumns, keyFile, CONFIG_GRP);
        m_kfTableManager->setup(this);
        m_kfTableManager->setup(m_netTree);
        refresh();
        if (update_interval > 0) {
            m_timer = Glib::signal_timeout().connect_seconds(
                    sigc::mem_fun(*this, &NetworkProperties::refresh), update_interval);
        }
    }
    else {
        psc::log::Log::logAdd(psc::log::Level::Warn, "Not found Gtk::TreeView named properties");
    }
}

void
NetworkProperties::on_response(int response_id)
{
    // signal-hide does not work for this as the dialog will be hidden and we wont get a usable size
    if (m_timer.connected()) {
        m_timer.disconnect(); // No more updating
    }
    m_kfTableManager->saveConfig(this);
    Gtk::Dialog::on_response(response_id);
}

void
NetworkProperties::addNetConnect(const Gtk::TreeModel::iterator& i, pNetConnect& netConnect)
{
	auto row = *i;
	row.set_value(m_propertyColumns->m_direction, Glib::ustring(netConnect->isIncomming() ? "tx" : "rx"));
    auto name = netConnect->getRemoteAddr()->getName();
    if (name.empty()) {
        name = netConnect->getRemoteAddr()->getIpAsString();
    }
	row.set_value(m_propertyColumns->m_addr, name);
	row.set_value(m_propertyColumns->m_service, netConnect->getWellKnownPort());
	row.set_value(m_propertyColumns->m_netConnect, netConnect);
}

bool
NetworkProperties::refresh()
{
    auto sel = m_netTree->get_selection();
    auto iterSel = sel->get_selected();
    pNetConnect selectedConnect;
    if (iterSel) {      // save selection if valid
        auto row = *iterSel;
        selectedConnect = row.get_value(m_propertyColumns->m_netConnect);
    }
    m_processNetInfo->updateConnections(m_netConn);  // use our own model as we get a garbled display if we are messing with the live time of main processes
    m_properties->clear();
    for (auto& conn : m_netConn) {
        if (conn->isValid()) {
            auto iterChld = m_properties->append();
            addNetConnect(iterChld, conn);
        }
    }
    m_netTree->expand_all();   // required for selection to work
    if (selectedConnect) {
        Gtk::TreeNodeChildren chlds = m_properties->children();
        for (auto iter = chlds.begin(); iter != chlds.end(); ++iter) {
            auto row = *iter;
            auto con = row.get_value(m_propertyColumns->m_netConnect);
            if (con == selectedConnect) {
                m_netTree->get_selection()->select(row);
                break;
            }
        }
    }
    return true;
}

NetworkProperties*
NetworkProperties::show(const long processId, Glib::KeyFile* keyFile, int32_t update_interval)
{
    NetworkProperties* netProp = nullptr;
    auto refBuilder = Gtk::Builder::create();
    try {
        auto gappl = Gtk::Application::get_default();
        auto appl = Glib::RefPtr<Gtk::Application>::cast_dynamic(gappl);
        refBuilder->add_from_resource(
            appl->get_resource_base_path() + "/network_properties.ui");
        refBuilder->get_widget_derived("net-prop-dlg", netProp, processId, keyFile, update_interval);
        if (netProp) {
            netProp->set_transient_for(*appl->get_active_window());
        }
        else {
            std::cerr << "NetworkProperties::show((): No \"proc-prop-dlg\" object in process_properties.ui"
                << std::endl;
        }
    }
    catch (const Glib::Error& ex) {
        std::cerr << "NetworkProperties::show((): " << ex.what() << std::endl;
    }
    return netProp;
}
