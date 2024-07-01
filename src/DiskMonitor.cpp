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
#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdlib.h>
#include <gtkmm.h>
#include <errno.h>
#include <math.h>
#include <glib/gi18n.h>

#include "HistMonitor.hpp"
#include "G15Worker.hpp"
#include "DiskMonitor.hpp"
#include "DiskInfos.hpp"

DiskMonitor::DiskMonitor(guint points)
: HistMonitor{points, "DISK"}
{
    m_enabled = FALSE;
}


DiskMonitor::~DiskMonitor() {
}


void
DiskMonitor::reinit()
{

}


void
DiskMonitor::disk_device_changed(Gtk::ComboBox *device_combo)
{
    m_device = device_combo->get_active_id();

    reinit();
}


Gtk::Box *
DiskMonitor::create_config_page(MonglView *monglView)
{
    auto disk_box = create_default_config_page(
            _("Display DISK usage"),
            _("DISK read color"),
            _("DISK write color"));

    auto combo = Gtk::manage(new Gtk::ComboBoxText());
    combo->append("", "");
    for (auto dev : m_diskInfos->getDevices()) {
        auto device = dev.first;
        auto devInfo = dev.second;
        std::string name(device);
        if (!devInfo->getMount().empty()) {
            name += " (" + devInfo->getMount() + ")";
        }
        combo->append(device, name);
    }
    combo->set_active_id(m_device);
    combo->signal_changed().connect(
                    sigc::bind<Gtk::ComboBox *>(
                    sigc::mem_fun(*this, &DiskMonitor::disk_device_changed),
                    combo));
    add_widget2box(disk_box, _("Device (.e.g sda)"), combo, 0.0f);

    //auto device_entry = Gtk::manage(new Gtk::Entry());
    //device_entry->set_text(m_device);
    //add_widget2box(disk_box, _("Device (.e.g sda)"), device_entry, 1.0f);
    //device_entry->signal_changed().connect(
    //	    sigc::bind<Gtk::Entry *>(
    //		sigc::mem_fun(*this, &DiskMonitor::disk_device_changed),
    //	    device_entry));

    return disk_box;
}

gboolean
DiskMonitor::update(int refreshRate, glibtop * glibtop)
{
    if (m_diskInfos) {
        auto diskInfo = m_diskInfos->getPrefered(m_device);
        if (diskInfo) {
            m_used_device = diskInfo->getDevice();
            addPrimarySecondary(diskInfo->getBytesReadPerS(), diskInfo->getBytesWrittenPerS());
            return true;
        }
    }
    return false;
}

unsigned long
DiskMonitor::getTotal()
{
    float fmax = std::max(m_primaryHist.getMax(), m_secondaryHist.getMax());
    return (unsigned long)fmax;
}

void
DiskMonitor::updateG15(Cairo::RefPtr<Cairo::Context> cr, guint width, guint height)
{
    float fmax = MAX(m_primaryHist.getMax(), m_secondaryHist.getMax());

    cr->move_to(1.0, 10.0);
    cr->show_text("Dsk");
    cr->move_to(1.0, 20.0);
    cr->show_text(m_used_device);
    cr->move_to(1.0, 30.0);
    std::string total = formatScale(fmax, "B/s");
    cr->show_text(total);

    cr->rectangle(60.5, 0.5, width-61, height-1);
    cr->stroke();
    double x = width - 1.5;
    for (int i = m_size-1; i >= 0; --i)
    {
        cr->move_to(x, height-1);
        float y = (float)(height-1) - (MAX(m_primaryHist.get(i), m_secondaryHist.get(i))/fmax)*(gfloat)(height-2);
        cr->line_to(x, y);
        x -= 1.0;
        if (x <= 60.0) {
            break;
        }
    }
    cr->stroke();
}

void
DiskMonitor::load_settings(const Glib::KeyFile * settings)
{
    config_setting_lookup_int(settings, m_name, CONFIG_DISPLAY_DISK, &m_enabled);
    if (!config_setting_lookup_color(settings, m_name, CONFIG_DISK_COLOR, m_foreground_color))
        m_foreground_color = Gdk::RGBA(DISK_PRIMARY_DEFAULT_COLOR);
    if (!config_setting_lookup_color(settings, m_name, CONFIG_DISK_SECONDARY_COLOR, m_secondary_color))
        m_secondary_color = Gdk::RGBA(DISK_SECONDARY_DEFAULT_COLOR);

    if (!config_setting_lookup_string(settings, m_name, CONFIG_DISK_DEVICE, m_device))
        m_device = Glib::ustring();

}


void
DiskMonitor::save_settings(Glib::KeyFile * setting)
{
    config_group_set_int(setting, m_name, CONFIG_DISPLAY_DISK, m_enabled);
    config_group_set_color(setting, m_name, CONFIG_DISK_COLOR, m_foreground_color);
    config_group_set_color(setting, m_name, CONFIG_DISK_SECONDARY_COLOR, m_secondary_color);
    config_group_set_string(setting, m_name, CONFIG_DISK_DEVICE, m_device);
}

std::string
DiskMonitor::getPrimMax()
{
    return formatScale(getTotal(), "B/s");
}

std::string
DiskMonitor::getSecMax()
{
    return m_used_device != "" ? std::string(m_used_device) : std::string("?");
}

void
DiskMonitor::setDiskInfos(const std::shared_ptr<DiskInfos>& diskInfos)
{
    m_diskInfos = diskInfos;
}
