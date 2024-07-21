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
#include <typeinfo>

#include "ProcessProperties.hpp"
#include "NetworkProperties.hpp"
#include "Process.hpp"


ProcessProperties::ProcessProperties(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder, const long processId, Glib::KeyFile* keyFile, int32_t update_interval)
: Gtk::Dialog{cobject}
, m_processes{std::make_shared<ProcessesBase>()}
, m_processId{processId}
, m_keyFile{keyFile}
, m_update_interval{update_interval}
, m_propertyColumns{std::make_shared<ProcessColumns>()}
{
    auto object = builder->get_object("properties");
    m_procTree = Glib::RefPtr<Gtk::TreeView>::cast_dynamic(object);
    if (m_procTree) {
        m_properties = Gtk::TreeStore::create(*m_propertyColumns);
        m_procTree->set_model(m_properties);
        m_kfTableManager = std::make_shared<psc::ui::KeyfileTableManager>(m_propertyColumns, keyFile, CONFIG_GRP);
        m_kfTableManager->setup(this);
        m_kfTableManager->setup(m_procTree);
        refresh();
        if (update_interval > 0) {
            m_timer = Glib::signal_timeout().connect_seconds(
                    sigc::mem_fun(*this, &ProcessProperties::refresh), update_interval);
        }
    }
    else {
        psc::log::Log::logAdd(psc::log::Level::Warn, "Not found Gtk::TreeView named properties");
    }
    object = builder->get_object("stop");
    auto stopButton = Glib::RefPtr<Gtk::Button>::cast_dynamic(object);
    if (stopButton) {
        stopButton->signal_clicked().connect(
                sigc::mem_fun(*this, &ProcessProperties::stopProcess));
    }
    else {
        psc::log::Log::logAdd(psc::log::Level::Warn, "Not found Gtk::Button named stop");
    }
    object = builder->get_object("net");
    auto netButton = Glib::RefPtr<Gtk::Button>::cast_dynamic(object);
    if (netButton) {
        netButton->signal_clicked().connect(
                sigc::mem_fun(*this, &ProcessProperties::showNetwork));
    }
    else {
        psc::log::Log::logAdd(psc::log::Level::Warn, "Not found Gtk::Button named net");
    }
}


void
ProcessProperties::on_response(int response_id)
{
    // signal-hide does not work as the dialog will be hidden and we wont get a usable size
    if (m_timer.connected()) {
        m_timer.disconnect(); // No more updating
    }
    m_kfTableManager->saveConfig(this);
    Gtk::Dialog::on_response(response_id);
}

void
ProcessProperties::stopProcess()
{
    auto treeSel = m_procTree->get_selection();
    if (treeSel->count_selected_rows() > 0) {
        auto iter = treeSel->get_selected();
        while (iter) {
            auto row = *iter;
            auto process = row.get_value(m_propertyColumns->m_process);
            if (process) {
                psc::log::Log::logAdd(psc::log::Level::Info, Glib::ustring::sprintf("Kill %s %d!", process->getName(), process->getPid()));
                process->killProcess();
                ++iter;
            }
        }
    }
    else {
        auto gappl = Gtk::Application::get_default();
        auto appl = Glib::RefPtr<Gtk::Application>::cast_dynamic(gappl);
        if (appl) {
            Gtk::MessageDialog messagedialog(*(appl->get_active_window()), "Please select a process to kill", false, Gtk::MessageType::MESSAGE_INFO);
            messagedialog.run();
            messagedialog.hide();
        }
        else {
            std::cout << "Please select a process to kill" << std::endl;
        }
    }
}


void
ProcessProperties::addProcess(const Gtk::TreeModel::iterator& i, pProcess& process)
{
	auto row = *i;
	row.set_value(m_propertyColumns->m_name, process->getDisplayName());
    row.set_value(m_propertyColumns->m_pid, process->getPid());
    row.set_value(m_propertyColumns->m_load, process->getRawLoad());
    row.set_value(m_propertyColumns->m_vmPeak, process->getVmPeakK());
    row.set_value(m_propertyColumns->m_vmSize, process->getVmSizeK());
    row.set_value(m_propertyColumns->m_vmData, process->getVmDataK());
    row.set_value(m_propertyColumns->m_vmStack, process->getVmStackK());
    row.set_value(m_propertyColumns->m_vmExec, process->getVmExecK());
    row.set_value(m_propertyColumns->m_vmRss, process->getVmRssK());
    row.set_value(m_propertyColumns->m_rssAnon, process->getRssAnonK());
    row.set_value(m_propertyColumns->m_rssFile, process->getRssFileK());
    row.set_value(m_propertyColumns->m_threads, process->getThreads());
    row.set_value(m_propertyColumns->m_user, getUid2Name(process->getUid()));
    row.set_value(m_propertyColumns->m_group, getGid2Name(process->getGid()));
    row.set_value(m_propertyColumns->m_state, Glib::ustring::sprintf("%c", process->getState()));
    row.set_value(m_propertyColumns->m_process, process);
    for (auto& child : process->getChildren()) {
        auto processChild = dynamic_pointer_cast<Process>(child) ;
        if (processChild) {
            auto n = m_properties->append(row.children());
            addProcess(n, processChild);
        }
        else {
            psc::log::Log::logAdd(psc::log::Level::Notice, Glib::ustring::sprintf("Traversing process %s", typeid(child).name()));
        }
    }
}

bool
ProcessProperties::selectProcess(const Gtk::TreeModel::iterator& i, pProcess& selectedProcess)
{
    if (i) {
        auto row = *i;
        auto process = row.get_value(m_propertyColumns->m_process);
        if (selectedProcess == process) {
            m_procTree->get_selection()->select(row);
            return true;
        }
        for (auto& child : row.children()) {
            if (selectProcess(child, selectedProcess)) {
                return true;
            }
        }
    }
    return false;
}

bool
ProcessProperties::refresh()
{
    auto sel = m_procTree->get_selection();
    auto iterSel = sel->get_selected();
    pProcess selectedProcess;
    if (iterSel) {      // save selection if valid
        auto row = *iterSel;
        selectedProcess = row.get_value(m_propertyColumns->m_process);
        //std::cout << "selectedProcess " << selectedProcess->getName() << std::endl;
    }
    m_processes->update();  // use our own model as we get a garbled display if we are messing with the live time of main processes (side effect synced update, but hight effort to scan all)
    m_properties->clear();
    auto process = m_processes->findPid(m_processId);
    if (process) {
        auto iterChld = m_properties->append();
        addProcess(iterChld, process);
        m_procTree->expand_all();   // required for selection to work
        if (selectedProcess) {
            Gtk::TreeNodeChildren chlds = m_properties->children();
            for (auto iter = chlds.begin(); iter != chlds.end(); ++iter) {
                selectProcess(iter, selectedProcess);
            }
        }
        return true;
    }
    return false;   // if not found no use of keep updating
}

Glib::ustring
ProcessProperties::getUid2Name(uint32_t uid)
{
    auto entry = m_users.find(uid); // lookup cache as the following might be expensive
    if (entry != m_users.end()) {
        return entry->second;
    }
    Glib::ustring userName;
    struct passwd *pws = getpwuid(uid);
    if (pws != nullptr) {
        userName = Glib::ustring(pws->pw_name);
    }
    userName += Glib::ustring::sprintf(" (%d)", uid);
    m_users.insert(std::pair(uid, userName));
    return userName;
}

Glib::ustring
ProcessProperties::getGid2Name(uint32_t gid)
{
    auto entry = m_groups.find(gid);   // lookup cache as the following might be expensive
    if (entry != m_groups.end()) {
        return entry->second;
    }
    Glib::ustring groupName;
    struct group *grp = getgrgid(gid);
    if (grp != nullptr) {
        groupName = Glib::ustring(grp->gr_name);
    }
    groupName += Glib::ustring::sprintf(" (%d)", gid);
    m_groups.insert(std::pair(gid, groupName));
    return groupName;
}

ProcessProperties*
ProcessProperties::show(const long processId, Glib::KeyFile* keyFile, int32_t update_interval)
{
    ProcessProperties* procProp = nullptr;
    auto refBuilder = Gtk::Builder::create();
    try {
        auto gappl = Gtk::Application::get_default();
        auto appl = Glib::RefPtr<Gtk::Application>::cast_dynamic(gappl);
        refBuilder->add_from_resource(
            appl->get_resource_base_path() + "/process_properties.ui");
        refBuilder->get_widget_derived("proc-prop-dlg", procProp, processId, keyFile, update_interval);
        if (procProp) {
            procProp->set_transient_for(*appl->get_active_window());
            /*int ret =*/
        }
        else {
            std::cerr << "ProcessProperties::show((): No \"proc-prop-dlg\" object in process_properties.ui"
                << std::endl;
        }
    }
    catch (const Glib::Error& ex) {
        std::cerr << "ProcessProperties::show((): " << ex.what() << std::endl;
    }
    return procProp;
}

void
ProcessProperties::showNetwork()
{
    NetworkProperties* netProp = NetworkProperties::show(m_processId, m_keyFile, m_update_interval);
    if (netProp) {
        netProp->run();
        netProp->hide();
        delete netProp;
    }
}