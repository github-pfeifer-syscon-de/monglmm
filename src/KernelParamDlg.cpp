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

#include <iostream>
#include <Log.hpp>

#include "KernelParamDlg.hpp"
#include "KernelParameter.hpp"

KernelParamDlg::KernelParamDlg(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder, Glib::KeyFile* keyFile, int32_t update_interval)
: Gtk::Dialog{cobject}
{
    builder->get_widget("comboNames", m_comboNames);
    builder->get_widget("text", m_text);
    auto nameModel = Gtk::ListStore::create(comboColumns);
    auto params = KernelParameter::getAllParameters();
    for (auto& param : params) {
        auto ins = nameModel->append();
        auto row = *ins;
        row.set_value(comboColumns.m_text, Glib::ustring{param->getName()});
        row.set_value(comboColumns.m_id, param);
    }
    m_comboNames->set_model(nameModel);
    m_comboNames->pack_start(comboColumns.m_text);  // set "visible" column !!!
    m_comboNames->signal_changed().connect(sigc::mem_fun(*this, &KernelParamDlg::refresh));
    m_comboNames->set_active(nameModel->children().begin());
}

void
KernelParamDlg::refresh()
{
    auto selectedName = m_comboNames->get_active();
    if (selectedName) {
        m_timer.disconnect();
        auto row = *selectedName;
        auto value = row.get_value(comboColumns.m_id);
        auto info = value->getName();
        info += "\n" + value->getInfo();
        auto query = value->query();
        auto error = value->getError();
        if (error.has_value()) {
            query = error.value();   // give a hint what might be wrong
            query += "\n" + value->getManualCommand();  // add alternative if e.g. permissions disallowed this
        }
        info += "\n" + query;
        auto test = value->getTest();
        if (!test.empty()) {
            info += "\nTo test use:\n";
            info += test;
        }
        auto persist= value->getPersist();
        if (!persist.empty()) {
            info += "\nTo persist use:\n";
            info += persist;
        }
        m_text->get_buffer()->set_text(info);
        if (value->isTimed()) {
            m_timer = Glib::signal_timeout().connect_seconds(
                            sigc::mem_fun(*this, &KernelParamDlg::update), 1);
        }
    }
}

bool
KernelParamDlg::update()
{
    auto selectedName = m_comboNames->get_active();
    if (selectedName) {
        auto row = *selectedName;
        auto value = row.get_value(comboColumns.m_id);
        auto buf = m_text->get_buffer();
        buf->insert_at_cursor(value->queryTimed());
        auto iter = buf->get_iter_at_mark(buf->get_insert());
        m_text->scroll_to(iter);
    }
    return true;
}

KernelParamDlg*
KernelParamDlg::show(Glib::KeyFile* keyFile, int32_t update_interval)
{
    KernelParamDlg* param_dlg = nullptr;
    auto refBuilder = Gtk::Builder::create();
    try {
        auto gappl = Gtk::Application::get_default();
        auto appl = Glib::RefPtr<Gtk::Application>::cast_dynamic(gappl);
        refBuilder->add_from_resource(
            appl->get_resource_base_path() + "/kernel_param.ui");
        refBuilder->get_widget_derived("kernel-param-dlg", param_dlg, keyFile, update_interval);
        if (!param_dlg) {
            psc::log::Log::logAdd(psc::log::Level::Error, "KernelParamDlg::show((): No \"kernel-param-dlg\" object in kernel_param.ui");
        }
    }
    catch (const Glib::Error& ex) {
        psc::log::Log::logAdd(psc::log::Level::Error, psc::fmt::format("KernelParamDlg::show: {}", ex));
    }
    return param_dlg;
}
