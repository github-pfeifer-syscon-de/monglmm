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


class CpuMonitor : public Monitor
{
public:
    CpuMonitor(guint points);
    virtual ~CpuMonitor();


    gboolean update(int refreshRate, glibtop * glibtop) override;
    void updateG15(Cairo::RefPtr<Cairo::Context> cr, guint width, guint height) override;

    void load_settings(const Glib::KeyFile * setting) override;

    void save_settings(Glib::KeyFile * setting) override;

    Gtk::Box* create_config_page(MonglView *monglView) override;

    unsigned long getTotal() override;
    std::string getPrimMax() override;
    std::string getSecMax() override;

private:
    double cpu_uns;
    double cpu_total;
    double m_cpu_total;

    static constexpr auto CPU_PRIMARY_DEFAULT_COLOR = "#0000FF";
    static constexpr auto CPU_SECONDARY_DEFAULT_COLOR = "#00FF00";
    static constexpr auto CONFIG_DISPLAY_CPU = "DisplayCPU";
    static constexpr auto CONFIG_CPU_COLOR = "CPUColor";
    static constexpr auto  CONFIG_SEONDARY_CPU_COLOR = "CPUSecondaryColor";
};
