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

#include <Log.hpp>
#include <unistd.h>     // getpwduid
#include <sys/types.h>  // getpwduid
#include <pwd.h>        // getpwduid
#include <grp.h>        // getgrgid

#include "ProcessProperties.hpp"

#include "Process.hpp"

PropertyColumns ProcessProperties::m_propertyColumns;

ProcessProperties::ProcessProperties(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder, const std::shared_ptr<Process>& process, int32_t update_interval)
: Gtk::Dialog{cobject}
, m_process{process}
{
    auto object = builder->get_object("properties");
    auto propTree = Glib::RefPtr<Gtk::TreeView>::cast_dynamic(object);
    if (propTree) {
        m_properties = Gtk::ListStore::create(m_propertyColumns);
        propTree->append_column("Name", m_propertyColumns.m_name);
        propTree->append_column("Value", m_propertyColumns.m_value);
        propTree->set_model(m_properties);
        refresh();
        if (update_interval > 0) {
        m_timer = Glib::signal_timeout().connect_seconds(
                sigc::mem_fun(*this, &ProcessProperties::refresh), update_interval);
        }
    }
    else {
        psc::log::Log::logAdd(psc::log::Level::Warn, "Not found Gtk::TreeView named properties");
    }
}

ProcessProperties::~ProcessProperties()
{
    if (m_timer.connected())
        m_timer.disconnect(); // No more updating

}

void
ProcessProperties::addRow(const Glib::ustring& name, const Glib::ustring& value)
{
	auto i = m_properties->append();
	auto row = *i;
	row.set_value(m_propertyColumns.m_name, name);
	row.set_value(m_propertyColumns.m_value, value);
}

bool
ProcessProperties::refresh()
{
    m_properties->clear();
    addRow("Name", m_process->getDisplayName());
    addRow("Pid", Glib::ustring::sprintf("%d", m_process->getPid()));
    addRow("Load", Glib::ustring::sprintf("%5.1lf%%", m_process->getRawLoad()*100.0));
    addRow("VMPeak", formatSize(m_process->getVmPeakK()));
    addRow("VMSize", formatSize(m_process->getVmSizeK()));
    addRow("VMData", formatSize(m_process->getVmDataK()));
    addRow("VMStack", formatSize(m_process->getVmStackK()));
    addRow("VMExec", formatSize(m_process->getVmExecK()));
    addRow("VMRss", formatSize(m_process->getVmRssK()));
    addRow("RssAnon", formatSize(m_process->getRssAnonK()));
    addRow("RssFile", formatSize(m_process->getRssFileK()));
    addRow("Threads", Glib::ustring::sprintf("%d", m_process->getThreads()));
    struct passwd *pws = getpwuid(m_process->getUid());
    if (pws != nullptr) {
        addRow("User", Glib::ustring::sprintf("%s (%d)", pws->pw_name, m_process->getUid()));
    }
    else {
        addRow("Uid", Glib::ustring::sprintf("%d", m_process->getUid()));
    }
    struct group *grp = getgrgid(m_process->getGid());
    if (grp != nullptr) {
        addRow("Group", Glib::ustring::sprintf("%s (%d)", grp->gr_name, m_process->getGid()));
    }
    else {
        addRow("Gid", Glib::ustring::sprintf("%d", m_process->getGid()));
    }
    addRow("State", Glib::ustring::sprintf("%c", m_process->getState()));
    return true;
}

Glib::ustring
ProcessProperties::formatSize(unsigned long value)
{
    return Glib::format_size(value*1024l, Glib::FormatSizeFlags::FORMAT_SIZE_IEC_UNITS);   //  based on 1024, FORMAT_SIZE_DEFAULT on 1000
}