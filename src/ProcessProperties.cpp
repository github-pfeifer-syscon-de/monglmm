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

#include "ProcessProperties.hpp"

#include "Process.hpp"

PropertyColumns ProcessProperties::m_propertyColumns;

ProcessProperties::ProcessProperties(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder, const std::shared_ptr<Process>& process)
: Gtk::Dialog{cobject}
, m_process{process}
{
    auto object = builder->get_object("properties");
    m_propTree = Glib::RefPtr<Gtk::TreeView>::cast_dynamic(object);
    if (m_propTree) {
        m_properties = Gtk::ListStore::create(m_propertyColumns);
        m_propTree->append_column("Name", m_propertyColumns.m_name);
        m_propTree->append_column("Value", m_propertyColumns.m_value);
        m_propTree->set_model(m_properties);
        refresh();
    }
    else {
        psc::log::Log::logAdd(psc::log::Level::Warn, "Not found Gtk::TreeView named properties");
    }
}

void
ProcessProperties::addRow(const Glib::ustring& name, const Glib::ustring& value)
{
	auto i = m_properties->append();
	auto row = *i;
	row.set_value(m_propertyColumns.m_name, name);
	row.set_value(m_propertyColumns.m_value, value);
}

void
ProcessProperties::refresh()
{
    m_properties->clear();
    addRow("Name", m_process->getDisplayName());
    addRow("Pid", Glib::ustring::sprintf("%d", m_process->getPid()));
    addRow("Load", Glib::ustring::sprintf("%.3f%%", m_process->getRawLoad()));
    addRow("VMPeak", formatSize(m_process->getVmPeakK()));
    addRow("VMSize", formatSize(m_process->getVmSizeK()));
    addRow("VMData", formatSize(m_process->getVmDataK()));
    addRow("VMStack", formatSize(m_process->getVmStackK()));
    addRow("VMExec", formatSize(m_process->getVmExecK()));
    addRow("VMRss", formatSize(m_process->getVmRssK()));
    addRow("RssAnon", formatSize(m_process->getRssAnonK()));
    addRow("RssFile", formatSize(m_process->getRssFileK()));
}

Glib::ustring
ProcessProperties::formatSize(unsigned long value)
{
return Glib::format_size(value*1024l, Glib::FormatSizeFlags::FORMAT_SIZE_DEFAULT);   // FORMAT_SIZE_IEC_UNITS based on 1024
}