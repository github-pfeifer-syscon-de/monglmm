/*
 * Copyright (C) 2021 rpf
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

#include "StringUtils.hpp"
#include "DiagramMonitor.hpp"

DiagramMonitor::DiagramMonitor(std::shared_ptr<Monitor> _monitor, NaviContext *_naviContext, TextContext *_textCtx)
: Diagram2{_monitor->getSize(), _naviContext, _textCtx}
, m_monitor{_monitor}
{
    for (guint i = 0; i < m_monitor->getNumDiagram(); ++i) {
		auto val = m_monitor->getValues(i);
        fill_buffers(i, val, m_monitor->getColor(i));
    }
}



void
DiagramMonitor::update(gint updateInterval, glibtop *glibtop)
{
    m_monitor->roll();
    m_monitor->update(updateInterval, glibtop);
    for (guint i = 0; i < m_monitor->getNumDiagram(); ++i) {
        fill_buffers(i, m_monitor->getValues(i), m_monitor->getColor(i));
    }
    const Glib::ustring pmax(m_monitor->getPrimMax());
    const Glib::ustring smax(m_monitor->getSecMax());
    setMaxs(pmax, smax);
}

void
DiagramMonitor::save_settings(Glib::KeyFile *config)
{
    m_monitor->save_settings(config);
}


Gtk::Box*
DiagramMonitor::create_config_page()
{
    return m_monitor->create_config_page();
}


void
DiagramMonitor::close()
{
    if (m_monitor) {
        m_monitor->close();
    }
}
