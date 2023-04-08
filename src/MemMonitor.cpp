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

#include <gtkmm.h>
#include <glibmm.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <errno.h>
#include <glib/gi18n.h>
#include <cstdlib>

#include "libgtop_helper.h"
#ifdef LIBGTOP
#include <glibtop/mem.h>
#endif
#include "Monitor.hpp"
#include "G15Worker.hpp"
#include "MonglView.hpp"
#include "MemMonitor.hpp"
#include "NameValue.hpp"


MemMonitor::MemMonitor(guint points)
: Monitor(points, "RAM")
, mem_total{0}
, mem_free{0}
, mem_buffers{0}
, mem_shared{0}
, mem_cached{0}
, swap_total{0}
, swap_free{0}
, mem_reclaimable{0}
{
    m_enabled = false;
}


MemMonitor::~MemMonitor() {
}

unsigned long MemMonitor::getTotal() {
    return mem_total;
}

Gtk::Box *
MemMonitor::create_config_page()
{
    auto mem_box = create_default_config_page(
            _("Display RAM usage"),
            _("RAM used"),
            _("RAM buffer"),
			_("Swap used"));

    return mem_box;
}

gboolean
MemMonitor::update(int refresh_rate, glibtop * glibtop)
{
#ifdef LIBGTOP
    glibtop_mem mem;
    glibtop_get_mem_l(glibtop, &mem);
    mem_total = mem.total / 1024ul;
    mem_buffers = mem.buffer / 1024ul;
    mem_free = mem.free / 1024ul;
    mem_cached = mem.cached / 1024ul;
#else
	NameValue nameValue;
	if (nameValue.read("/proc/meminfo")) {
		mem_total = nameValue.getUnsigned("MemTotal:");
		mem_free = nameValue.getUnsigned("MemFree:");
		mem_buffers = nameValue.getUnsigned("Buffers:");
		mem_cached = nameValue.getUnsigned("Cached:");
		swap_total = nameValue.getUnsigned("SwapTotal:");
		swap_free = nameValue.getUnsigned("SwapFree:");
		mem_shared = nameValue.getUnsigned("Shmem:");
		mem_reclaimable = nameValue.getUnsigned("SReclaimable:");
	}
    else {
        m_enabled = false;
        return false;
    }

#endif

    getValues(0)->set((double)getUsedMemory() / (double)mem_total);
    getValues(1)->set((double)(mem_total - mem_free) / (double)mem_total);
    getValues(2)->set((double)(swap_total - swap_free) / (double)swap_total);

    return true;
}

guint
MemMonitor::defaultValues()
{
	return 3;
}

unsigned long 
MemMonitor::getUsedMemory()
{
	// see https://stackoverflow.com/questions/41224738/how-to-calculate-system-memory-usage-from-proc-meminfo-like-htop
	//  still a bit uncertain about this ...
	//  at least the display of the biggest processes vs. total used leaves only a small amount for others
	//unsigned long cached = mem_cached + mem_reclaimable - mem_shared;
	//unsigned long used = (mem_total - mem_free - (mem_buffers + cached));
	// the simplified calculation is closer to what top displays
	//   still there is a large mismatch for sum of processes.vmRssK and this values...
	//   (all process used 1901436 used 1161140) at least for the purpose of our display doesn't look too bad
	unsigned long used = mem_total - mem_free - mem_buffers - mem_cached;
	return used;
}

void
MemMonitor::printInfo()
{
	unsigned long used = mem_total - mem_free - mem_buffers - mem_cached;
	std::cout <<  "used " << used << std::endl << std::endl;
}

void
MemMonitor::updateG15(Cairo::RefPtr<Cairo::Context> cr, guint width, guint height)
{
    char temp[64];
//    sprintf(temp, "Fr: 0x%x", result);
//    cr->move_to(100.0, 10.0);
//    cr->show_text(temp);

    sprintf(temp, "Mem:%.1fG", (double)mem_total / 1024.0 / 1024.0);
    cr->move_to(1.0, 10.0);
    cr->show_text(temp);
//
//    sprintf(temp, "Fre: %.1fG", (float)mem_free / 1024.0 / 1024.0);
//    cr->move_to(1.0, 20.0);
//    cr->show_text(temp);
//
//    sprintf(temp, "Buf: %.1fG", (float)(mem_buffers + mem_cached) / 1024.0 / 1024.0);
//    cr->move_to(1.0, 30.0);
//    cr->show_text(temp);

    sprintf(temp, "Use:%.1fG", (double)getUsedMemory() / 1024.0 / 1024.0);
    cr->move_to(0.0, 20.0);
    cr->show_text(temp);

    sprintf(temp, "Swap:%.1fG", (double)(swap_total - swap_free) / 1024.0 / 1024.0);
    cr->move_to(0.0, 30.0);
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
MemMonitor::load_settings(const Glib::KeyFile * settings)
{
    config_setting_lookup_int(settings, m_name, CONFIG_DISPLAY_RAM, &m_enabled);
    if (!config_setting_lookup_color(settings, m_name, CONFIG_RAM_COLOR, m_foreground_color))
        m_foreground_color = Gdk::RGBA(MEM_PRIMARY_DEFAULT_COLOR);
    if (!config_setting_lookup_color(settings, m_name, CONFIG_SECONDARY_RAM_COLOR, m_secondary_color))
        m_secondary_color = Gdk::RGBA(MEM_SECONDARY_DEFAULT_COLOR);
    if (!config_setting_lookup_color(settings, m_name, CONFIG_SWAP_COLOR, m_ternary_color))
        m_ternary_color = Gdk::RGBA(SWAP_DEFAULT_COLOR);

}

void
MemMonitor::save_settings(Glib::KeyFile * setting)
{
    config_group_set_int(setting, m_name, CONFIG_DISPLAY_RAM, m_enabled);
    config_group_set_color(setting, m_name, CONFIG_RAM_COLOR, m_foreground_color);
    config_group_set_color(setting, m_name, CONFIG_SECONDARY_RAM_COLOR, m_secondary_color);
    config_group_set_color(setting, m_name, CONFIG_SWAP_COLOR, m_ternary_color);
}


std::string
MemMonitor::getPrimMax()
{
    return formatScale(getTotal()*1024.0, "B");
}

std::string
MemMonitor::getSecMax()	// swap is ternary but this doesn't matter too much
{
  return swap_total > 0l ? formatScale(swap_total*1024.0, "B") : std::string();
}
