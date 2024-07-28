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

#include <iostream>
#include <memory>
#include <map>
#include <gtkmm.h>
#include <KeyfileTableManager.hpp>

#include "ProcessesBase.hpp"

class Process;

class LoadConverter
: public psc::ui::CustomConverter<double>
{
public:
    LoadConverter(Gtk::TreeModelColumn<double>& col)
    : psc::ui::CustomConverter<double>(col)
    {
    }
    virtual ~LoadConverter() = default;

     void convert(Gtk::CellRenderer* rend, const Gtk::TreeModel::iterator& iter) override {
        double in;
        iter->get_value(m_col.index(), in);
        auto val = Glib::ustring::sprintf("%5.1lf%%", in*100.0);
        auto textRend = static_cast<Gtk::CellRendererText*>(rend);
        textRend->property_text() = val;
     }
};

class KSizeConverter
: public psc::ui::CustomConverter<glong>
{
public:
    KSizeConverter(Gtk::TreeModelColumn<glong>& col)
    : psc::ui::CustomConverter<glong>(col)
    {
    }
    virtual ~KSizeConverter() = default;

    void convert(Gtk::CellRenderer* rend, const Gtk::TreeModel::iterator& iter) override {
        glong value = 0;
        iter->get_value(m_col.index(), value);
        auto size = Glib::format_size(value*1024l, Glib::FormatSizeFlags::FORMAT_SIZE_IEC_UNITS);   //  based on 1024, FORMAT_SIZE_DEFAULT on 1000
        auto textRend = static_cast<Gtk::CellRendererText*>(rend);
        textRend->property_text() = size;
     }
};

class USizeConverter
: public psc::ui::CustomConverter<gulong>
{
public:
    USizeConverter(Gtk::TreeModelColumn<gulong>& col)
    : psc::ui::CustomConverter<gulong>(col)
    {
    }
    virtual ~USizeConverter() = default;

    void convert(Gtk::CellRenderer* rend, const Gtk::TreeModel::iterator& iter) override {
        gulong value = 0;
        iter->get_value(m_col.index(), value);
        auto size = Glib::format_size(value, Glib::FormatSizeFlags::FORMAT_SIZE_IEC_UNITS);
        auto textRend = static_cast<Gtk::CellRendererText*>(rend);
        textRend->property_text() = size;
     }
};


class ProcessColumns
: public psc::ui::ColumnRecord
{
public:
    Gtk::TreeModelColumn<Glib::ustring> m_name;
    Gtk::TreeModelColumn<glong> m_pid;
    Gtk::TreeModelColumn<double> m_load;
    Gtk::TreeModelColumn<glong> m_vmPeak;
    Gtk::TreeModelColumn<glong> m_vmSize;
    Gtk::TreeModelColumn<glong> m_vmData;
    Gtk::TreeModelColumn<glong> m_vmStack;
    Gtk::TreeModelColumn<glong> m_vmExec;
    Gtk::TreeModelColumn<glong> m_vmRss;
    Gtk::TreeModelColumn<glong> m_rssAnon;
    Gtk::TreeModelColumn<glong> m_rssFile;
    Gtk::TreeModelColumn<glong> m_threads;
    Gtk::TreeModelColumn<Glib::ustring> m_user;
    Gtk::TreeModelColumn<Glib::ustring> m_group;
    Gtk::TreeModelColumn<Glib::ustring> m_state;
    Gtk::TreeModelColumn<pProcess> m_process;

    ProcessColumns()
    {
        add<Glib::ustring>("Name", m_name);
        add<glong>("Pid", m_pid, 1.0f);
        auto loadConverter = std::make_shared<LoadConverter>(m_load);
        add<double>("Load", loadConverter, 1.0f);
        auto sizeVmpeakConverter = std::make_shared<KSizeConverter>(m_vmPeak);
        add<glong>("VMPeak", sizeVmpeakConverter, 1.0f);
        auto sizeVmsizeConverter = std::make_shared<KSizeConverter>(m_vmSize);
        add<glong>("VMSize", sizeVmsizeConverter, 1.0f);
        auto sizeVmdataConverter = std::make_shared<KSizeConverter>(m_vmData);
        add<glong>("VMData", sizeVmdataConverter, 1.0f);
        auto sizeVmstackConverter = std::make_shared<KSizeConverter>(m_vmStack);
        add<glong>("VMStack", sizeVmstackConverter, 1.0f);
        auto sizeVmexecConverter = std::make_shared<KSizeConverter>(m_vmExec);
        add<glong>("VMExec", sizeVmexecConverter, 1.0f);
        auto sizeVmrssConverter = std::make_shared<KSizeConverter>(m_vmRss);
        add<glong>("VMRss", sizeVmrssConverter, 1.0f);
        auto sizeRssAnonConverter = std::make_shared<KSizeConverter>(m_rssAnon);
        add<glong>("RSSAnon", sizeRssAnonConverter, 1.0f);
        auto sizeRssFileConverter = std::make_shared<KSizeConverter>(m_rssFile);
        add<glong>("RSSFile", sizeRssFileConverter, 1.0f);
        add<glong>("Threads", m_threads, 1.0f);
        add<Glib::ustring>("User", m_user, 1.0f);
        add<Glib::ustring>("Group", m_group, 1.0f);
        add<Glib::ustring>("State", m_state, 1.0f);
        // keep this as last, for not confuse indexing of additional infos
        Gtk::TreeModel::ColumnRecord::add(m_process);     // this will not be visible, so use std. function
    }
    virtual ~ProcessColumns() = default;
};


class ProcessProperties
: public Gtk::Dialog
{
public:
    ProcessProperties(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder, const long processId, Glib::KeyFile* keyFile, int32_t update_interval);
    explicit ProcessProperties(const ProcessProperties& orig) = delete;
    virtual ~ProcessProperties() = default;
    static ProcessProperties* show(const long processId, Glib::KeyFile* keyFile, int32_t update_interval);
protected:
    void addProcess(const Gtk::TreeModel::iterator& i, pProcess& process);
    bool selectProcess(const Gtk::TreeModel::iterator& i, pProcess& selectedProcess);
    bool refresh();
    Glib::ustring getUid2Name(uint32_t uid);
    Glib::ustring getGid2Name(uint32_t gid);
    void stopProcess();
    void on_response(int response_id) override;
    void showNetwork();
    static constexpr auto CONFIG_GRP = "ProcessProperties";
private:
    std::shared_ptr<ProcessesBase> m_processes;
    long m_processId;
    Glib::KeyFile* m_keyFile;
    int32_t m_update_interval;
    Glib::RefPtr<Gtk::TreeStore> m_properties;
    sigc::connection m_timer;               /* Timer for regular updates */
    std::map<uint32_t, Glib::ustring> m_users;
    std::map<uint32_t, Glib::ustring> m_groups;
    Glib::RefPtr<Gtk::TreeView> m_procTree;
    std::shared_ptr<ProcessColumns> m_propertyColumns;
    std::shared_ptr<psc::ui::KeyfileTableManager> m_kfTableManager;
};

