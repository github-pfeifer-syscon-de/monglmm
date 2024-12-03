/* -*- Mode: c++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
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

#include <iostream>
#include <Log.hpp>


#include "LogProperties.hpp"

LevelIconConverter::LevelIconConverter(Gtk::TreeModelColumn<psc::log::Level>& col)
: psc::ui::CustomConverter<psc::log::Level>(col)
{
    for (uint32_t i = 0; i < m_levelPixmap.size(); ++i) {
        m_levelPixmap[i] = getIconForLevel(static_cast<psc::log::Level>(i));
    }
}

Gtk::CellRenderer*
LevelIconConverter::createCellRenderer()
{
    return Gtk::manage<Gtk::CellRendererPixbuf>(new Gtk::CellRendererPixbuf());
}

Glib::RefPtr<Gdk::Pixbuf>
LevelIconConverter::getIconForLevel(psc::log::Level level)
{
    Glib::RefPtr<Gdk::Pixbuf> pixmap;
    const char* icon_name = nullptr;
    switch (level) {
    case psc::log::Level::Severe:
    case psc::log::Level::Alert:
    case psc::log::Level::Crit:
        icon_name = "process-stop";
        break;
    case psc::log::Level::Error:
        icon_name = "dialog-error";
        break;
    case psc::log::Level::Warn:
        icon_name = "dialog-warning";
        break;
    case psc::log::Level::Notice:
    case psc::log::Level::Info:
        icon_name = "dialog-information";
        break;
    case psc::log::Level::Debug:
        icon_name = "edit-find";
        break;
    }
    auto icon_theme = Gtk::IconTheme::get_default();
    try {
        if (icon_name) {
            pixmap = icon_theme->load_icon(icon_name, 24, Gtk::IconLookupFlags::ICON_LOOKUP_USE_BUILTIN);        // USE_BUILTIN
            //std::cout << " pix " << (pixmap ? "yes" : "no") << std::endl;
        }
        else {
        }
    }
    catch (const Glib::Error& exc) {
        std::cout << "Error lookup icon " << static_cast<int>(level) << " exc " << exc.what() << std::endl;
    }
    if (!pixmap) {
        pixmap = icon_theme->load_icon("gtk-cancel", 24, Gtk::IconLookupFlags::ICON_LOOKUP_USE_BUILTIN);
        std::cout << " no name, using default! " << std::endl;
    }
    return pixmap;
}

void
LevelIconConverter::convert(Gtk::CellRenderer* rend, const Gtk::TreeModel::iterator& iter)
{
    psc::log::Level value;
    iter->get_value(m_col.index(), value);
    auto idx = static_cast<uint32_t>(value);
    if (idx < m_levelPixmap.size()) {
        auto svalue = m_levelPixmap[idx];
        if (svalue) {
            auto pixRend = static_cast<Gtk::CellRendererPixbuf*>(rend);
            //std::cout << "convert " << static_cast<int>(value) << " pix " << (svalue ? "yes" : "no") << std::endl;
            pixRend->property_pixbuf() = svalue;
        }
    }
}

LogProperties::LogProperties(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder, Glib::KeyFile* keyFile, int32_t update_interval)
: Gtk::Dialog{cobject}
, m_propertyColumns{std::make_shared<LogColumns>()}
{
    builder->get_widget("comboNames", m_comboNames);
    builder->get_widget("comboBoot", m_comboBoot);
    builder->get_widget("search", m_search);
    builder->get_widget("btnRefresh", m_btnRefresh);
    builder->get_widget("level", m_comboLevel);
    buildLevelCombo();
    buildSelectionModels();
    m_comboNames->signal_changed().connect([&] {
        refresh();
    });
    m_comboBoot->signal_changed().connect([&] {
        refresh();
    });
    m_btnRefresh->signal_clicked().connect([&] {
        refresh();
    });
    m_search->signal_changed().connect([&] {
        refresh();
    });
    m_comboLevel->signal_changed().connect([&] {
        refresh();
    });
    auto object = builder->get_object("properties");
    m_logTable = Glib::RefPtr<Gtk::TreeView>::cast_dynamic(object);
    if (m_logTable) {
        m_properties = Gtk::TreeStore::create(*m_propertyColumns);
        m_logTable->set_model(m_properties);
        m_kfTableManager = std::make_shared<psc::ui::KeyfileTableManager>(m_propertyColumns, keyFile, CONFIG_GRP);
        m_kfTableManager->setAllowSort(false);  // as we may create large number of entries with converted data
        m_kfTableManager->setup(this);
        m_kfTableManager->setup(m_logTable);
        refresh();
        //if (update_interval > 0) {
        //    m_timer = Glib::signal_timeout().connect_seconds(
        //            sigc::mem_fun(*this, &LogProperties::refresh), update_interval);
        //}
    }
    else {
        psc::log::Log::logAdd(psc::log::Level::Warn, "Not found Gtk::TreeView named properties");
    }
}

void
LogProperties::buildLevelCombo()
{
    for (uint32_t i = 0; i < LevelIconConverter::NUM_LOG_LEVEL; ++i) {
        auto level = static_cast<psc::log::Level>(i);
        m_comboLevel->append(
            Glib::ustring::sprintf("%d", i),
            Glib::ustring::sprintf("%s%s", (i > 0 ? ">= " : ""), psc::log::Log::getLevelFull(level)));
    }
    m_comboLevel->set_active_id(Glib::ustring::sprintf("%d", static_cast<int>(psc::log::Level::Debug)));    // debug results in see all
}

void
LogProperties::buildSelectionModels()
{
    m_nameModel = Gtk::ListStore::create(comboColumns);
    m_bootModel = Gtk::ListStore::create(comboColumns);
    Gtk::TreeModel::iterator bootSelected;
    try {
        auto logView = psc::log::LogView::create();
        auto bootId = logView->getBootId(); // either boot or today
        auto idents = logView->getIdentifiers();
        for (auto& logId : idents) {
            //std::cout << "Found " << logId->getType() << " = " << logId->getName() << " qurey " << logId->getQuery() << std::endl;
            if (psc::log::LogViewType::Time == logId->getType()) {
                auto ins = m_bootModel->append();
                if (logId->isBootId(bootId)) {
                    bootSelected = ins;
                }
                auto row = *ins;
                row.set_value(comboColumns.m_text, Glib::ustring{logId->getName()});
                row.set_value(comboColumns.m_id, logId);
            }
            else {
                auto ins = m_nameModel->append();
                auto row = *ins;
                row.set_value(comboColumns.m_text, Glib::ustring{logId->getName()});
                row.set_value(comboColumns.m_id, logId);
            }
        }
    }
    catch (const psc::log::LogViewException& exc) {
        auto tmp = Glib::ustring::sprintf("Error view log %s", exc.what());
        Gtk::MessageDialog msg = Gtk::MessageDialog(tmp, false, Gtk::MessageType::MESSAGE_ERROR);
        msg.run();
    }
    m_comboNames->set_model(m_nameModel);
    m_comboNames->pack_start(comboColumns.m_text);  // set "visible" column !!!
    m_comboBoot->set_model(m_bootModel);
    m_comboBoot->pack_start(comboColumns.m_text);  // set "visible" column !!!
    m_comboBoot->set_active(bootSelected);
}

void
LogProperties::on_response(int response_id)
{
    // signal-hide does not work for this as the dialog will be hidden and we wont get a usable size
    if (m_timer.connected()) {
        m_timer.disconnect(); // No more updating
    }
    m_kfTableManager->saveConfig(this);
    Gtk::Dialog::on_response(response_id);
}

int
LogProperties::isUtf8Start(char c)
{
    int utf8len = 0;
    if ((c & 0b11100000) == 0b11000000) {
        utf8len = 2;
    }
    else if ((c & 0b11110000) == 0b11100000) {
        utf8len = 3;
    }
    else if ((c & 0b11111000) == 0b11110000) {
        utf8len = 4;
    }
    return utf8len;
}

// as log entries may contain binary data
//   make them utf-8 compatible
Glib::ustring
LogProperties::toUstring(const std::string& str)
{
    Glib::ustring ustring;
    ustring.reserve(str.size());
    bool inEscape = false;
    int utf8len = 0;
    for (char c : str) {
        if (utf8len > 0) {
            ustring += c;
            --utf8len;
        }
        else {
            int utf8len = isUtf8Start(c);
            if (utf8len > 0) {
                ustring += c;
                --utf8len;
            }
            else {
                if (c < 0x20 || c > 0x7f) {
                    if (!inEscape) {
                        ustring += '<';
                        inEscape = true;
                    }
                    ustring.append(Glib::ustring::sprintf("%02x", c));
                }
                else {
                    if (inEscape) {
                        ustring += '>';
                        inEscape = false;
                    }
                    ustring += c;
                }
            }
        }
    }
    if (inEscape) {
        ustring += '>';
        inEscape = false;
    }
    return ustring;
}

void
LogProperties::addLog(const Gtk::TreeModel::iterator& i, psc::log::pLogViewEntry& logEntry)
{
	auto row = *i;
	row.set_value(m_propertyColumns->m_date, logEntry->getLocalTime());
	row.set_value(m_propertyColumns->m_message, toUstring(logEntry->getMessage()));
	row.set_value(m_propertyColumns->m_location, toUstring(logEntry->getLocation()));
    row.set_value(m_propertyColumns->m_level, logEntry->getLevel());
}

static bool
contains(const Glib::ustring& value, const Glib::ustring& search)
{
    return value.find(search) != value.npos;
}

bool
LogProperties::refresh()
{
    m_properties->clear();
    try {
        auto logLevel = psc::log::Level::Debug;
        auto levelId = m_comboLevel->get_active_id();
        if (!levelId.empty()) {
            auto ilevel = std::stoi(levelId);
            logLevel = static_cast<psc::log::Level>(ilevel);
        }
        std::list<std::shared_ptr<psc::log::LogViewIdentifier>> query;
        auto selectedName = m_comboNames->get_active();  // use internal id
        if (selectedName) {
            auto row = *selectedName;
            auto value = row.get_value(comboColumns.m_id);
            query.push_back(value);
        }
        auto selectedBoot = m_comboBoot->get_active();
        if (selectedBoot) {
            auto row = *selectedBoot;
            auto value = row.get_value(comboColumns.m_id);
            query.push_back(value);
        }
        if (query.size() >= 2) {    // otherwise the number of entries might get to large
            Glib::ustring search = m_search->get_text();
            auto logView = psc::log::LogView::create();
            logView->setQuery(query);
            for (auto iter = logView->begin(); iter != logView->end(); ++iter) {
                auto logEntry = *iter;
                if ((search.empty()
                 || contains(logEntry->getLocation(), search)
                 || contains(logEntry->getMessage(), search))
                 && logEntry->getLevel() <= logLevel) {
                    if (m_properties->children().size() <= ROW_LIMIT) {
                        auto iterChld = m_properties->append();
                        addLog(iterChld, logEntry);
                    }
                    else {
                        auto iterChld = m_properties->append();
                        auto row = *iterChld;
                        row.set_value(m_propertyColumns->m_message, Glib::ustring::sprintf("More than %d rows, stopped loading ...", ROW_LIMIT));
                        row.set_value(m_propertyColumns->m_level, psc::log::Level::Crit);
                        break;
                    }
                }
            }
        }
        else {
            //std::cout << "Only have query " << query.size() << std::endl;
        }
    }
    catch (const psc::log::LogViewException& exc) {
        auto tmp = Glib::ustring::sprintf("Error view log %s", exc.what());
        Gtk::MessageDialog msg = Gtk::MessageDialog(tmp, false, Gtk::MessageType::MESSAGE_ERROR);
        msg.run();
        msg.hide();
    }
    return true;
}

LogProperties*
LogProperties::show(Glib::KeyFile* keyFile, int32_t update_interval)
{
    LogProperties* logProp = nullptr;
    auto refBuilder = Gtk::Builder::create();
    try {
        auto gappl = Gtk::Application::get_default();
        auto appl = Glib::RefPtr<Gtk::Application>::cast_dynamic(gappl);
        refBuilder->add_from_resource(
            appl->get_resource_base_path() + "/log_properties.ui");
        refBuilder->get_widget_derived("log-prop-dlg", logProp, keyFile, update_interval);
        if (!logProp) {
            psc::log::Log::logAdd(psc::log::Level::Error, "LogProperties::show((): No \"proc-prop-dlg\" object in process_properties.ui");
        }
    }
    catch (const Glib::Error& ex) {
        psc::log::Log::logAdd(psc::log::Level::Error, std::format("LogProperties::show: {}", ex));
    }
    return logProp;
}