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

class MemMonitor : public Monitor
{
public:
    MemMonitor(guint points);
    virtual ~MemMonitor();


    gboolean update(int refresh_rate, glibtop * glibtop) override;

    void load_settings(const Glib::KeyFile * setting) override;

    void save_settings(Glib::KeyFile * setting) override;

    Gtk::Box* create_config_page() override;

    void updateG15(Cairo::RefPtr<Cairo::Context> cr, guint width, guint height) override;

    unsigned long getTotal() override;
    std::string getPrimMax() override;
    std::string getSecMax() override;
    guint defaultValues() override;

    void printInfo();
private:
    unsigned long mem_total;
    unsigned long mem_free;
    unsigned long mem_buffers;
    unsigned long mem_shared;
    unsigned long mem_cached;
    unsigned long swap_total;
    unsigned long swap_free;
    unsigned long mem_reclaimable;

    unsigned long getUsedMemory();
};


#define MEM_DEFAULT_SIZE 3
#define MEM_PRIMARY_DEFAULT_COLOR "#FF0000"
#define MEM_SECONDARY_DEFAULT_COLOR "#00FF00"
#define SWAP_DEFAULT_COLOR "#808000"

#define CONFIG_DISPLAY_RAM "DisplayRAM"
#define CONFIG_RAM_COLOR "RAMColor"
#define CONFIG_SECONDARY_RAM_COLOR "RAMSecondaryColor"
#define CONFIG_SWAP_COLOR "SWAPColor"
