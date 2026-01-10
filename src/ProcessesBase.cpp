/* -*- Mode: c++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
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

#include <iostream>
#include <string>
#include <cstdlib>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <Log.hpp>


#include "ProcessesBase.hpp"

ProcessesBase::ProcessesBase()
{
}

ProcessesBase::~ProcessesBase()
{
    mProcesses.clear();
}

pProcess
ProcessesBase::findPid(long pid)
{
    pProcess process;
    auto entry = mProcesses.find(pid);
    if (entry != mProcesses.end()) {
        process = entry->second;
    }
    return process;
}

void
ProcessesBase::update()
{
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(sdir)) != nullptr) {
        /* find all the processes in /proc */
        for (auto& p : mProcesses) {
            auto& proc = p.second;
            if (proc->isActive()) {
                proc->setStage(psc::gl::TreeNodeState::Running);
            }
            proc->setTouched(false);
        }
        while ((ent = readdir(dir)) != nullptr) {
            if (ent->d_type == DT_DIR) {
                long pid = std::atol(ent->d_name);
                if (pid > 0) {
                    auto p = mProcesses.find(pid);
                    pProcess proc;
                    if (p != mProcesses.end()) {
                        proc = p->second;
                    }
                    else {
                        auto path = Glib::ustring::sprintf("%s/%ld", sdir, pid);
                        proc = createProcess(path, pid);
                        mProcesses.insert(std::pair(pid, proc));
                    }
                    proc->update();
                }
            }
        }
        closedir(dir);
        for (auto p = mProcesses.begin(); p != mProcesses.end(); ) {
            auto& proc = p->second;
            if (!proc->isTouched()) {   // if we didn't touch the entry process died
                if (proc->getStage() < psc::gl::TreeNodeState::Close) {    // close in 2 steps to show status
                    proc->setStage(psc::gl::TreeNodeState::Close);
                    ++p;
                }
                else {
                    auto parent = proc->getParent();
                    if (parent) {   // remove any reference
                        parent->remove(proc.get());
                    }
                    p = mProcesses.erase(p);    // make unreachable, the inc needs to be post increment (the doc says it is incremented internally this might be right in the case of a vector) !!!
                }
            }
            else {
                ++p;
            }
        }
    }
    buildTree();
}

pProcess
ProcessesBase::createProcess(std::string path, long pid)
{
    return std::make_shared<Process>(path, pid, 0);
}

void
ProcessesBase::buildTree()
{
    for (auto& p : mProcesses) {
        auto& proc = p.second;      // as process assignments might change rebuild tree

        long ppid = proc->getPpid();
        auto pp = mProcesses.find(ppid);
        if (pp != mProcesses.end()) {
            pProcess pproc = pp->second;
            proc->setParent(pproc.get(), proc);
        }
        else {
            // some kernel internal porcess have ppid 0 dont expect much impact by them ?
            //std::cout << "Process " << ppid << " not founnd!" << std::endl;
        }
    }
    if (!m_procRoot) {
        auto pr = mProcesses.find(ROOT_PID);  // seems as we can be sure proc 1 is the root
        if (pr != mProcesses.end()) {
            m_procRoot = pr->second;
        }
        else {
            // as update and redraw happen asynchronously this will happen on first call
            psc::log::Log::logAdd(psc::log::Level::Error, "No process 1 to root proc tree!");
        }
    }
}

