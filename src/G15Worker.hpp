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
#include <memory>

#include "Page.hpp"

static const int N_G15_CMD = 6;

class G15Worker
{
public:
    G15Worker(std::vector<std::shared_ptr<Page>> pages, Glib::Dispatcher *_pDispatcher);
    virtual ~G15Worker();

    void do_work();
    void stop();
    void refresh();
    gint getErrorCode();    // will report init error
    gint getCmdErrorCode(); // last error for cmd
    char *getCmd();
    void cmd_changed(int idx, Gtk::Entry *entry);
    Gtk::Box *create_config();
    void read_config(Glib::KeyFile *m_config);
    void save_config(Glib::KeyFile *m_config);
    bool isRunning();

private:
    void g15_update();
    int rundirect(const Glib::ustring &cmd);
    void run(const Glib::ustring &cmd);

    std::vector<std::shared_ptr<Page>> page;
    guint g15display;
    gint errorCode;
    gint cmdErrorCode;
    gboolean running;
    gboolean dorefresh;
    gboolean netstart;
    gchar m_cmd[64];
    Glib::Dispatcher *m_pDispatcher;
    Glib::ustring gcmd[N_G15_CMD];
    Glib::ustring m1on;
    Glib::ustring m1off;
};
