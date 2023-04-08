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

#pragma once

#include <memory>
#include <glibmm.h>

#include "Monitor.hpp"
#include "Diagram.hpp"
#include "libgtop_helper.h"

class DiagramMonitor : public Diagram {
public:
    DiagramMonitor(std::shared_ptr<Monitor> _monitor, NaviContext *_naviContext, TextContext *_textCtx);
    virtual ~DiagramMonitor();

    void update(gint updateInterval, glibtop *glibtop);
    void save_settings(Glib::KeyFile *keyFile);
    Gtk::Box* create_config_page();
    void close();

    std::shared_ptr<Monitor> getMonitor() {
        return m_monitor;
    }
private:
    std::shared_ptr<Monitor> m_monitor;
};

