/* -*- Mode: c++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * Copyright (C) 2024 RPf 
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

class LevelIconConverter
: public psc::ui::CustomConverter<psc::log::Level>
{
public:
    LevelIconConverter(Gtk::TreeModelColumn<psc::log::Level>& col);
    virtual ~LevelIconConverter() = default;

    Gtk::CellRenderer* createCellRenderer() override;
    static Glib::RefPtr<Gdk::Pixbuf> getIconForLevel(psc::log::Level level);
    void convert(Gtk::CellRenderer* rend, const Gtk::TreeModel::iterator& iter) override;
    static constexpr auto NUM_LOG_LEVEL{8u};     // maybe move this to Log
private:
    std::array<Glib::RefPtr<Gdk::Pixbuf>, NUM_LOG_LEVEL> m_levelPixmap;
};

class LevelTextConverter
: public psc::ui::CustomConverter<psc::log::Level>
{
public:
    LevelTextConverter(Gtk::TreeModelColumn<psc::log::Level>& col)
    : psc::ui::CustomConverter<psc::log::Level>(col)
    {
    }
    virtual ~LevelTextConverter() = default;

    void convert(Gtk::CellRenderer* rend, const Gtk::TreeModel::iterator& iter) override {
        psc::log::Level value;
        iter->get_value(m_col.index(), value);
        auto textRend = static_cast<Gtk::CellRendererText*>(rend);
        auto svalue = Glib::ustring{psc::log::Log::getLevelFull(value)};
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
    Gtk::TreeModelColumn<psc::log::Level> m_level;

    LogColumns()
    {
        auto dateConverter = std::make_shared<DateConverter>(m_date);
        add<psc::log::LogTime>("Date", dateConverter);
        add<Glib::ustring>("Message", m_message);
        add<Glib::ustring>("Location", m_location);
        auto levelIconConverter = std::make_shared<LevelIconConverter>(m_level);
        add<psc::log::Level>("Icon", levelIconConverter, 0.5f);
        auto levelTextConverter = std::make_shared<LevelTextConverter>(m_level);
        add<psc::log::Level>("Level", levelTextConverter);
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
    void addLog(const Gtk::TreeModel::iterator& i, psc::log::LogViewEntry& logViewEntry);
    Glib::ustring toUstring(const std::string& str);
    int isUtf8Start(char c);
    void buildLevelCombo();
    void buildSelectionModels();
    uint32_t m_rowLimit{5000}; // this seems a reasonable limit but if you use a fast system you may increase it if needed
    static constexpr auto CONFIG_GRP = "LogProperties";
    static constexpr auto CONFIG_LOG_VIEW_ROW_LIMIT = "logViewRowLimit";
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
    Gtk::ComboBoxText* m_comboLevel;
};

