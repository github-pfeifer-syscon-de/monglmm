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

#include "Monitor.hpp"

const unsigned int CLOCKS=16;   // Maximum number of clocks to display, beyond that the visual representation is not that useful

class ClkMonitor : public Monitor
{
public:
    ClkMonitor(guint points);
    virtual ~ClkMonitor();

    float readMaxFreq();
    gboolean update(int refreshRate, glibtop * glibtop) override;
    void updateG15(Cairo::RefPtr<Cairo::Context> cr, guint width, guint height) override;

    void load_settings(const Glib::KeyFile * setting) override;

    void save_settings(Glib::KeyFile * setting) override;

    Gtk::Box* create_config_page() override;

    unsigned long getTotal() override;
    std::string getPrimMax() override;
    std::string getSecMax() override;
    Gdk::RGBA *getColor(unsigned int diagram) override;
    double clock_avg(const std::string &cpu_dir, unsigned int clock_steps[], unsigned int clock_time[]);

private:
    unsigned int cpus;
    double clkMHz[CLOCKS];
    unsigned int m_clockStep[CLOCKS][CLOCKS];  // per cpu steps
    unsigned int m_clockTime[CLOCKS][CLOCKS];  // per cpu times
    double maxMHz;
    void decodeMaxClock(std::string &cpu);
    Gdk::RGBA *colors[CLOCKS];
};





#define HW_PRIMARY_DEFAULT_COLOR "#0000FF"
#define HW_SECONDARY_DEFAULT_COLOR "#00FF00"

#define CONFIG_DISPLAY_HW "DisplayHW"
#define CONFIG_HW_COLOR "HWColor"
#define CONFIG_SEONDARY_HW_COLOR "HWSecondaryColor"


