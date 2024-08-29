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
#include <glib/gi18n.h>
#include <cstdlib>
#include <gtkmm.h>
#include <errno.h>

#include "libgtop_helper.h"
#ifdef LIBGTOP
#include <glibtop/cpu.h>
#endif
#include "G15Worker.hpp"
#include "CpuMonitor.hpp"

using namespace std;

// See http://www.linuxhowtos.org/System/procstat.htm
typedef unsigned long long jiffies;

struct cpu_stat {
    jiffies user, nice, sys, idle, iowait, irq, softirq, total;
};

static struct cpu_stat previous_cpu_stat = { 0l, 0l, 0l, 0l, 0l, 0l, 0l, 0l};

CpuMonitor::CpuMonitor(guint points)
: Monitor(points, "CPU")
, cpu_uns{0.0}
, cpu_total{0.0}
, m_cpu_total{0.0}
{
    m_enabled = TRUE;          // enabled by default
}

CpuMonitor::~CpuMonitor() {
}

unsigned long
CpuMonitor::getTotal() {
    return (unsigned long)cpu_total;
}

Gtk::Box *
CpuMonitor::create_config_page(MonglView *monglView)
{
    auto cpu_box = create_default_config_page(
            _("Display CPU usage"),
            _("CPU user"),
            _("CPU system"));

    return cpu_box;
}

gboolean
CpuMonitor::update(int refreshRate, glibtop * glibtop)
{
    struct cpu_stat cpu;
#ifdef LIBGTOP
    glibtop_cpu gcpu;
    glibtop_get_cpu_l(glibtop, &gcpu);
    // frequency gives the measure e.g. 100 -> jiffies = 1/100s

    cpu.user = gcpu.user;
    cpu.idle = gcpu.idle;
    cpu.nice = gcpu.nice;
    cpu.sys = gcpu.sys;
    cpu.iowait = gcpu.iowait;
    cpu.irq = gcpu.irq;
    cpu.softirq = gcpu.softirq;
    cpu.total = gcpu.total;
    struct cpu_stat cpu_delta = { 0l, 0l, 0l, 0l, 0l, 0l, 0l, 0l};
    if (previous_cpu_stat.total > 0) {   // delta only useful if we captured previous values
        cpu_delta.user = cpu.user - previous_cpu_stat.user;
        cpu_delta.nice = cpu.nice - previous_cpu_stat.nice;
        cpu_delta.sys = cpu.sys - previous_cpu_stat.sys;
        cpu_delta.total = cpu.total - previous_cpu_stat.total;
    }

    /* Copy current to previous. */
    memcpy(&previous_cpu_stat, &cpu, sizeof(struct cpu_stat));

    double cpu_un = cpu_delta.user + cpu_delta.nice;
    cpu_uns = cpu_un + cpu_delta.sys;
    cpu_total = cpu_delta.total;
    if (cpu_total > 0l) {
        getValues(0)->set(cpu_un / cpu_total);       // use only user here as we show process below that
        getValues(1)->set(cpu_uns / cpu_total);      // stack sys value as it is not related to process times

        m_cpu_total = cpu_total / (double)((refreshRate > 0) ? refreshRate : 1);    // show per second
    }
#else

    std::ifstream  stat ;
    std::string  str;

    std::ios_base::iostate exceptionMask = stat.exceptions() | std::ios::failbit | std::ios::badbit;
    stat.exceptions(exceptionMask);
    /* Open statistics file and scan out CPU usage. */
    try
    {
        stat.open ("/proc/stat");
        while (!stat.eof())
        {
            stat >> str;
            if (str == "cpu")
            {
                stat >> cpu.user >> cpu.nice >> cpu.sys >> cpu.idle >> cpu.iowait >> cpu.irq >> cpu.softirq;
                break;
            }
        }
        stat.close();
    }
    catch (std::ios_base::failure &e)
    {
        g_warning("monitors: Could not open /proc/stat: %d, %s",
                  errno, strerror(errno) );
        // gives no useful information
        //std::cerr << e.code().message() << std::endl;
        m_enabled = false;
        return(FALSE);
    }


    struct cpu_stat cpu_delta = { 0l, 0l, 0l, 0l, 0l, 0l, 0l, 0l};
    if (previous_cpu_stat.user
      + previous_cpu_stat.sys
      + previous_cpu_stat.idle > 0) {   // delta only useful if we captured previous values
        cpu_delta.user = cpu.user - previous_cpu_stat.user;
        cpu_delta.nice = cpu.nice - previous_cpu_stat.nice;
        cpu_delta.sys = cpu.sys - previous_cpu_stat.sys;
        cpu_delta.idle = cpu.idle - previous_cpu_stat.idle;
        cpu_delta.iowait = cpu.iowait - previous_cpu_stat.iowait;
        cpu_delta.irq = cpu.irq - previous_cpu_stat.irq;
        cpu_delta.softirq = cpu.softirq - previous_cpu_stat.softirq;
    }

    /* Copy current to previous. */
    memcpy(&previous_cpu_stat, &cpu, sizeof(struct cpu_stat));

    double cpu_un = cpu_delta.user + cpu_delta.nice;
    cpu_uns = cpu_un + cpu_delta.sys;
    cpu_total = cpu_uns + cpu_delta.idle + cpu_delta.iowait + cpu_delta.irq + cpu_delta.softirq;
    if (cpu_total > 0l) {
        getValues(0)->set(cpu_un / cpu_total);       // use only user here as we show process below that
        getValues(1)->set(cpu_uns / cpu_total);      // stack sys value as it is not related to process times

        m_cpu_total = cpu_total / (double)((refreshRate > 0) ? refreshRate : 1);    // show per second
    }
#endif
    return TRUE;
}

void
CpuMonitor::updateG15(Cairo::RefPtr<Cairo::Context> cr, guint width, guint height)
{
    char temp[64];

    cr->move_to(1.0, 10.0);
    cr->show_text("Cpu");
    sprintf(temp, "%.1f%%", cpu_uns / cpu_total * 100.0f);
    cr->move_to(1.0, 20.0);
    cr->show_text(temp);

    cr->rectangle(60.5, 0.5, width-61, height-1);
    cr->stroke();
    double x = width - 1.5;
    for (int i = m_size-1; i >= 0; --i)
    {
        cr->move_to(x, height-1);
        float y = (gfloat)(height-1) - getValues(0)->get(i)*(gfloat)(height-2);
        cr->line_to(x, y);
        x -= 1.0;
        if (x <= 60.0) {
            break;
        }
    }
    cr->stroke();

}

void
CpuMonitor::load_settings(const Glib::KeyFile  * settings)
{
    config_setting_lookup_int(settings, m_name, CONFIG_DISPLAY_CPU, &m_enabled);
    if (!config_setting_lookup_color(settings, m_name, CONFIG_CPU_COLOR, m_foreground_color))
        m_foreground_color = Gdk::RGBA(CPU_PRIMARY_DEFAULT_COLOR);
    if (!config_setting_lookup_color(settings, m_name, CONFIG_SEONDARY_CPU_COLOR, m_secondary_color))
	m_secondary_color = Gdk::RGBA(CPU_SECONDARY_DEFAULT_COLOR);

}

void
CpuMonitor::save_settings(Glib::KeyFile * settings)
{
    config_group_set_int(settings, m_name, CONFIG_DISPLAY_CPU, m_enabled);
    config_group_set_color(settings, m_name, CONFIG_CPU_COLOR, m_foreground_color);
    config_group_set_color(settings, m_name, CONFIG_SEONDARY_CPU_COLOR, m_secondary_color);
}

std::string
CpuMonitor::getPrimMax()
{
    std::ostringstream oss1;
    float total = round(m_cpu_total / 100.0);  // as the percent value varies show cores
    oss1.precision(1);
    oss1 << std::fixed <<  total;
    std::string max = oss1.str();
    return max;
}

std::string
CpuMonitor::getSecMax()
{
    return std::string();
}
