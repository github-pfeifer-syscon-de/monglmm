/*
 * Copyright (C) 2018 rpf
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

#include <string>

#include "HistMonitor.hpp"

class NetMonitor : public HistMonitor
{
public:
    NetMonitor(guint points);
    virtual ~NetMonitor();

    gboolean update(int refreshRate, glibtop * glibtop) override;

    void load_settings(const Glib::KeyFile * setting) override;
    void save_settings(Glib::KeyFile * setting) override;

    Gtk::Box* create_config_page(MonglView *monglView) override;

    void reinit() override;
    unsigned long getTotal() override;
    std::string getPrimMax() override;
    std::string getSecMax() override;

    void updateG15(Cairo::RefPtr<Cairo::Context> cr, guint width, guint height) override;
private:
    void net_device_changed(Gtk::Entry *device_entry) ;
    
    static constexpr auto CONFIG_DISPLAY_NET = "DisplayNET";
    static constexpr auto CONFIG_NET_COLOR = "NETColor";
    static constexpr auto CONFIG_NET_SECONDARY_COLOR = "NETsecondaryColor";
    static constexpr auto CONFIG_NET_DEVICE = "NETdevice";
    static constexpr auto NET_PRIMARY_DEFAULT_COLOR = "#00FF00";
    static constexpr auto NET_SECONDARY_DEFAULT_COLOR = "#FF0000";
};




