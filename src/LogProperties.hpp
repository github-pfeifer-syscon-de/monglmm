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

#include <gtkmm.h>
#include <KeyfileTableManager.hpp>
#include <chrono>
#include <LogView.hpp>


class DateConverter
: public psc::ui::CustomConverter<psc::log::LogTime>
{
public:
    DateConverter(Gtk::TreeModelColumn<psc::log::LogTime>& col)
    : psc::ui::CustomConverter<psc::log::LogTime>(col)
    {
    }
    virtual ~DateConverter() = default;

    void convert(Gtk::CellRenderer* rend, const Gtk::TreeModel::iterator& iter) override {
        psc::log::LogTime value;
        iter->get_value(m_col.index(), value);
        auto textRend = static_cast<Gtk::CellRendererText*>(rend);
        auto svalue = value.isValid()
                      ? value.format("%F %T")
                      : Glib::ustring{};    
        textRend->property_text() = svalue;
     }
private:
};

class LogColumns
: public psc::ui::ColumnRecord
{
public:
    Gtk::TreeModelColumn<psc::log::LogTime> m_date;
    Gtk::TreeModelColumn<Glib::ustring> m_message;
    Gtk::TreeModelColumn<Glib::ustring> m_location;

    LogColumns()
    {
        auto dateConverter = std::make_shared<DateConverter>(m_date);
        add<psc::log::LogTime>("Date", dateConverter);
        add<Glib::ustring>("Message", m_message);
        add<Glib::ustring>("Location", m_location);
    }
    virtual ~LogColumns() = default;
};

class ComboColumns
: public Gtk::TreeModel::ColumnRecord
{
public:
    Gtk::TreeModelColumn<Glib::ustring> m_text;
    Gtk::TreeModelColumn<std::shared_ptr<psc::log::LogViewIdentifier>> m_id;
    ComboColumns()
    {
        add(m_text);
        add(m_id);
    }
};


class LogProperties
: public Gtk::Dialog
{
public:
    LogProperties(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder, Glib::KeyFile* keyFile, int32_t update_interval);
    explicit LogProperties(const LogProperties& orig) = delete;
    virtual ~LogProperties() = default;

    static LogProperties* show(Glib::KeyFile* keyFile, int32_t update_interval);
protected:

    bool refresh();
    void on_response(int response_id);
    void addLog(const Gtk::TreeModel::iterator& i, psc::log::pLogViewEntry& netConnect);
    Glib::ustring toUstring(const std::string& str);
    int isUtf8Start(char c);
    static constexpr auto CONFIG_GRP = "LogProperties";
private:
    Glib::RefPtr<Gtk::TreeStore> m_properties;
    sigc::connection m_timer;               /* Timer for regular updates */
    Glib::RefPtr<Gtk::TreeView> m_logTable;
    Gtk::ComboBox* m_comboNames;
    Gtk::ComboBox* m_comboBoot;
    Gtk::SearchEntry* m_search;
    Gtk::Button* m_btnRefresh;
    std::shared_ptr<LogColumns> m_propertyColumns;
    std::shared_ptr<psc::ui::KeyfileTableManager> m_kfTableManager;
    ComboColumns comboColumns;
    Glib::RefPtr<Gtk::ListStore> m_nameModel;
    Glib::RefPtr<Gtk::ListStore> m_bootModel;
};

