/* -*- Mode: c++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * Copyright (C) 2024 RPf <gpl3@pfeifer-syscon.de>
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

#include <gtkmm.h>

class Process;


class PropertyColumns
: public Gtk::TreeModel::ColumnRecord
{
public:
    Gtk::TreeModelColumn<Glib::ustring> m_name;
    Gtk::TreeModelColumn<Glib::ustring> m_value;
    PropertyColumns()
    {
        add(m_name);
        add(m_value);
    }
};


class ProcessProperties
: public Gtk::Dialog
{
public:
    ProcessProperties(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder, const std::shared_ptr<Process>& process, int32_t update_interval);
    explicit ProcessProperties(const ProcessProperties& orig) = delete;
    virtual ~ProcessProperties();
protected:
    static PropertyColumns m_propertyColumns;
    void addRow(const Glib::ustring& name, const Glib::ustring& value);
    bool refresh();
    Glib::ustring formatSize(unsigned long value);

private:
    std::shared_ptr<Process> m_process;
    Glib::RefPtr<Gtk::ListStore> m_properties;
    sigc::connection    m_timer;               /* Timer for regular updates     */

};

