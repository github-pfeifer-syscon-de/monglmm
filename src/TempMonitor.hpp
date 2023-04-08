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

#include <gtkmm.h>
#include <vector>

#include "Monitor.hpp"
#include "Sensor.hpp"
#include "Sensors.hpp"


class TempMonitor : public Monitor
{
public:
    TempMonitor(guint points);
    virtual ~TempMonitor();

    gboolean update(int refreshRate, glibtop * glibtop) override;
    void updateG15(Cairo::RefPtr<Cairo::Context> cr, guint width, guint height) override;

    void load_settings(const Glib::KeyFile * setting) override;

    void save_settings(Glib::KeyFile * setting) override;

    Gtk::Box* create_config_page() override;
    unsigned long getTotal() override;
    std::string getPrimMax() override;
    std::string getSecMax() override;
    void close() override;

    Gtk::Box *create_config_page(const char *enabledLabel);
    void sensor_changed(guint index, Gtk::ComboBox *combo);
    void color_changed(guint index, Gtk::ColorButton *color);
    Gdk::RGBA *getColor(unsigned int diagram) override;
private:
    void createEntry(Gtk::Box *box, guint index);
    std::vector<std::shared_ptr<Sensors>> m_allSensors;

    std::vector<std::shared_ptr<Sensor>> m_actSensors;

    std::vector<Gdk::RGBA> m_colors;
};


#define CONFIG_DISPLAY_TEMP "DisplayTemp"
#define CONFIG_TEMP_COLOR "TempColor"
#define CONFIG_TEMP_SENSOR "TempSensor"


