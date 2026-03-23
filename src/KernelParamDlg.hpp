/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2026 rpf
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

class KernelParameter;

class ParamComboColumns
: public Gtk::TreeModel::ColumnRecord
{
public:
    Gtk::TreeModelColumn<Glib::ustring> m_text;
    Gtk::TreeModelColumn<std::shared_ptr<KernelParameter>> m_id;
    ParamComboColumns()
    {
        add(m_text);
        add(m_id);
    }
};

class KernelParamDlg
: public Gtk::Dialog
{
public:
    KernelParamDlg(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder, Glib::KeyFile* keyFile, int32_t update_interval);
    explicit KernelParamDlg(const KernelParamDlg& orig) = delete;
    virtual ~KernelParamDlg() = default;

    static KernelParamDlg* show(Glib::KeyFile* keyFile, int32_t update_interval);
protected:
    void refresh();
    bool update();

private:
    Gtk::ComboBox* m_comboNames;
    Gtk::TextView* m_text;
    ParamComboColumns comboColumns;
    sigc::connection m_timer;
};

