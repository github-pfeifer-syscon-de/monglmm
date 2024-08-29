/*
 * Copyright (C) 2018 rpf
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

#include <string.h>
#include <string>
#include <iostream>
#include <fstream>
#include <gtkmm.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <Log.hpp>
#include <StringUtils.hpp>

#include "Process.hpp"
#include "Monitor.hpp"
#include "NameValue.hpp"

Process::Process(std::string _path, long _pid, guint _size)
: psc::gl::TreeNode2::TreeNode2()
, stage{psc::gl::TreeNodeState::New}
, path{std::move(_path)}
, lastCpuTime{0l}
, m_size{_size}
, data_cpu{std::make_shared<Buffer<double>>(_size)}
, data_mem{std::make_shared<Buffer<double>>(_size)}
, cpuTime{0}
, pid{0}
, state{'?'}
, ppid{0}
, vmPeakK{0}
, vmSizeK{0}
, vmDataK{0}
, vmStackK{0}
, vmExecK{0}
, vmRssK{0}
, rssAnonK{0}
, rssFileK{0}
, stat_state{'?'}
, stat_ppid{0}
, pgrp{0}
, session{0}
, tty_nr{0}
, tpgid{0}
, flags{0}
, minflt{0}
, cminflt{0}
, majflt{0}
, cmajflt{0}
, utime{0}
, stime{0}
, cutime{0}
, cstime{0}
, priority{0}
, nice{0}
, num_threads{0}
, itrealvalue{0}
, starttime{0}
, vsize{0}
, rss{0}
, rsslim{0}
, m_load{0.0}
{
    pid = _pid;
}

void
Process::roll()
{
    data_mem->roll();
    data_cpu->roll();
}

long
Process::getPid() const
{
    return pid;
}

unsigned long
Process::getCpuUsage()
{
    if (lastCpuTime == 0l) {
        return 0l;
    }
    unsigned long dt = cpuTime - lastCpuTime;
    //std::cout << name << " sc " << scClock << " dt " << dt << std::endl;
    return dt;
}

unsigned long
Process::getCpuUsageBuf()
{
    return data_cpu->sum();
}

float
Process::getLoad()
{
    double cpu = m_load;
    int c10 = (int)(cpu * 10.0);
    return (float)((float)c10 / 10.0f);    // to not force redraw of processes too often use some load granularity
}

unsigned long
Process::getCpuUsageSum()
{
    return cpuTime;
}

void
Process::update()
{
    touched = true;
    if (!isActive()) {
        return;
    }
    update_status();
    update_stat();
    // tried to make ../nnn/io stats available but these are not worth the effort as they produce various errors (with already taking the effort to read them at once)
}

void
Process::update(std::shared_ptr<Monitor> cpu, std::shared_ptr<Monitor> mem) {
    if (!isActive()) {
        return;
    }
    roll();
    // Show factor of total
    if (cpu->getTotal() > 0l) {
        m_load = (double)getCpuUsage() / (double)cpu->getTotal();
        data_cpu->set(m_load);
    }
    if (mem->getTotal() > 0l) {
        data_mem->set((double)getMemGraph() / (double)mem->getTotal());
    }
    //std::cout << name << " cpu " << (double)getCpuUsage() << " total " <<  (double)cpu->getTotal() << std::endl;
    //std::cout << name << " mem " <<  (double)getMemUsage() << " total " << (double)mem->getTotal() << std::endl;
}

void Process::update_stat()
{
    std::string sstat = path + "/stat";

    std::ifstream  stat;

    std::ios_base::iostate exceptionMask = stat.exceptions() | std::ios::failbit | std::ios::badbit | std::ios::eofbit;
    stat.exceptions(exceptionMask);
    /* Open statistics file  */
    try {
        stat.open(sstat);
        if (stat.is_open()) {
            if (!stat.eof()) {
                std::string  str;
                std::getline(stat, str);
                guint pos = str.find(")");    // need to determine end for process as it may contain spaces...
                if (pos != std::string::npos) {
                    ++pos;
                    std::istringstream is(str.substr(pos));
                    is >> stat_state
                       >> stat_ppid
                       >> pgrp
                       >> session       // The session ID of the process.
                       >> tty_nr
                       >> tpgid
                       >> flags
                       >> minflt
                       >> cminflt
                       >> majflt
                       >> cmajflt
                       >> utime
                       >> stime
                       >> cutime
                       >> cstime
                       >> priority
                       >> nice
                       >> num_threads
                       >> itrealvalue
                       >> starttime
                       >> vsize
                       >> rss
                   >> rsslim;
                }
            }
        }
    }
    catch (std::ios_base::failure &e) {
        if (!stat.eof()) {  // as we may hit eof while reading ...
	  psc::log::Log::logAdd(psc::log::Level::Notice, Glib::ustring::sprintf("Proc read %s what %s msg %s", sstat, e.what(), e.code().message()));
            //std::cerr << sstat << " what " << e.what() << " val " << e.code().value() << " Err " << e.code().message() << std::endl;
            stage = psc::gl::TreeNodeState::Finished;     // do not ask again
        }
    }
    if (stat.is_open()) {
        stat.close();
    }
    lastCpuTime = cpuTime;
    cpuTime =  (utime + stime);
}

void
Process::update_status()
{
	// 	/proc/[pid]/statm replicates some of the values
    std::string sstat = path + "/status";

	NameValue nameValue;
	if (nameValue.read(sstat)) {
		name = nameValue.getString("Name:");
		auto sstate = nameValue.getString("State:");
		state = sstate[0];
		ppid = nameValue.getUnsigned("PPid:");
		vmPeakK = nameValue.getUnsigned("VmPeak:");
		vmSizeK = nameValue.getUnsigned("VmSize:");
		vmDataK = nameValue.getUnsigned("VmData:");
		vmStackK = nameValue.getUnsigned("VmStk:");
		vmExecK = nameValue.getUnsigned("VmExe:");
		vmRssK = nameValue.getUnsigned("VmRSS:");
		rssAnonK = nameValue.getUnsigned("RssAnon:");
		rssFileK = nameValue.getUnsigned("RssFile:");
        auto uid = nameValue.getString("Uid:");
        std::vector<Glib::ustring> uids;
        StringUtils::split(uid,'\t', uids);
        if (uids.size() > 0) {  // Real, Effective, Saved and FileSystem UID
            m_uid = static_cast<uint32_t>(std::stoul(uids[0]));
        }
        else {
            m_uid = ROOT_UID;   // presume root ?
        }
        auto gid = nameValue.getString("Gid:");
        std::vector<Glib::ustring> gids;
        StringUtils::split(gid,'\t', gids);
        if (gids.size() > 0) {  // ??? Real, Effective, Saved and FileSystem GID
            m_gid = static_cast<uint32_t>(std::stoul(gids[0]));
        }
        else {
            m_uid = ROOT_GID;   // presume root ?
        }
	}
	else {
        stage = psc::gl::TreeNodeState::Finished;     // do not ask again
	}
}


Glib::ustring
Process::getDisplayName()
{
    Glib::ustring wname(name);
    return wname;
}

bool
Process::isPrimary()
{
    return m_uid == ROOT_UID;
}

void
Process::killProcess()
{
    kill(pid, SIGTERM);
}

std::shared_ptr<Buffer<double>>
Process::getCpuData()
{
	return data_cpu;
}

std::shared_ptr<Buffer<double>>
Process::getMemData()
{
	return data_mem;
}

void
Process::setTouched(bool _touched)
{
	touched = _touched;
}

bool
Process::isTouched()
{
	return touched;
}

const char*
Process::getName()
{
	return name.c_str();
}

psc::gl::TreeNodeState
Process::getStage()
{
	return stage;
}

void
Process::setStage(psc::gl::TreeNodeState _stage)
{
	stage = _stage;
}

bool
Process::isActive()
{
	return stage < psc::gl::TreeNodeState::Finished;
}

long
Process::getMemUsage()
{
	// vmSize is the whole space the process laid out, not necessarily used
	// VmRSS    matches top resv value (exceeds the value that we use as total, and the sum of process is ~ram size which does not match the situation, but right for display)
	return vmRssK;
}

long
Process::getMemGraph()
{
	// rssAnonK is below VmRSS value (to ensures we stay below used in graph, as memory mapping is highly optimized&dynamic we cant expect to get it right by this simple means, the smap coud provide more detail but that is much slower as the proc man page says)
	return rssAnonK;
}

long Process::getPpid() const
{
	return ppid;
}
