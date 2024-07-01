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

class DiskInfos;    // forward declaration

class DiskMonitor : public HistMonitor
{
public:
    DiskMonitor(guint points);
    virtual ~DiskMonitor();

    gboolean update(int refreshRate, glibtop * glibtop) override;
    void updateG15(Cairo::RefPtr<Cairo::Context> cr, guint width, guint height) override;

    void load_settings(const Glib::KeyFile * setting) override;

    void save_settings(Glib::KeyFile * setting) override;

    Gtk::Box* create_config_page(MonglView *monglView) override;

    void reinit() override;
    unsigned long getTotal() override;
    std::string getPrimMax() override;
    std::string getSecMax() override;

    void setDiskInfos(const std::shared_ptr<DiskInfos>& diskInfos);
private:
    void disk_device_changed(Gtk::ComboBox *device_combo);
    std::shared_ptr<DiskInfos> m_diskInfos;
    static constexpr auto DISK_PRIMARY_DEFAULT_COLOR = "#00FF00";
    static constexpr auto DISK_SECONDARY_DEFAULT_COLOR = "#FF0000";
    static constexpr auto CONFIG_DISPLAY_DISK = "DisplayDISK";
    static constexpr auto CONFIG_DISK_COLOR = "DISKColor";
    static constexpr auto CONFIG_DISK_SECONDARY_COLOR = "DISKsecondaryColor";
    static constexpr auto CONFIG_DISK_DEVICE = "DISKdevice";
};
