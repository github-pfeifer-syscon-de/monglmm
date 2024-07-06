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

#pragma once

#include <iostream>
#include <gtkmm.h>
#include <glibmm.h>
#include <string>
#include <memory>
#include <TreeNode2.hpp>

#include "Geom2.hpp"
#include "Monitor.hpp"

class Process
: public psc::gl::TreeNode2 {
public:
    Process(std::string path, long pid, guint _size);
    virtual ~Process() = default;

    void roll();
    void setPid(long pid);
    long getPid();
    void update(std::shared_ptr<Monitor> cpu, std::shared_ptr<Monitor> mem);
    void update_status();
    void update_stat();
    Glib::ustring getDisplayName() override;
    bool isPrimary() override;
    std::shared_ptr<Buffer<double>> getCpuData();
    std::shared_ptr<Buffer<double>> getMemData();
    void setTouched(bool _touched);
    bool isTouched();
    const char* getName() override;
    psc::gl::TreeNodeState getStage() override;
    float getLoad() override;
    inline double getRawLoad() {
        return m_load;
    }
    inline long getVmPeakK() {
        return vmPeakK;
    }
    inline long getVmSizeK() {
        return vmSizeK;
    }
    inline long getVmDataK() {
        return vmDataK;
    }
    inline long getVmStackK() {
        return vmStackK;
    }
    inline long getVmExecK() {
        return vmExecK;
    }
    inline long getVmRssK() {
        return vmRssK;
    }
    inline long getRssAnonK() {
        return rssAnonK;
    }
    inline long getRssFileK() {
        return rssFileK;
    }
    inline long getThreads() {
        return num_threads;
    }
    inline uint32_t getUid() {
        return m_uid;
    }
    inline uint32_t getGid() {
        return m_gid;
    }

    inline char getState() {
        return state;
    }
    void setStage(psc::gl::TreeNodeState _stage);
    bool isActive();
    long getMemUsage();
    long getMemGraph();
    long getPpid();
    unsigned long getCpuUsage();
    unsigned long getCpuUsageBuf();
    unsigned long getCpuUsageSum();
    void killProcess();
    static constexpr auto ROOT_UID = 0u;
    static constexpr auto ROOT_GID = 0u;
private:
    // internal
    bool touched;

    psc::gl::TreeNodeState stage;  // when getting an error on reading set this to false so we wont ask again
    std::string path;
    Position pos;
    unsigned long lastCpuTime;
    guint m_size;
    std::shared_ptr<Buffer<double>> data_cpu;
    std::shared_ptr<Buffer<double>> data_mem;
    Gdk::RGBA m_color;
    unsigned long cpuTime;
    // status fields
    long pid;
    std::string name;
    char state;
    long ppid;
    long vmPeakK;
    long vmSizeK;
    long vmDataK;
    long vmStackK;
    long vmExecK;
    long vmRssK;
    long rssAnonK;
    long rssFileK;
    // stat fields
    char stat_state;    //One of the following characters, indicating process
                        //state:
                        //R  Running
                        //S  Sleeping in an interruptible wait
                        //D  Waiting in uninterruptible disk sleep
                        //Z  Zombie
                        //T  Stopped (on a signal) or (before Linux 2.6.33)
                        //   trace stopped
                        //t  Tracing stop (Linux 2.6.33 onward)
                        //W  Paging (only before Linux 2.6.0)
                        //X  Dead (from Linux 2.6.0 onward)
                        //x  Dead (Linux 2.6.33 to 3.13 only)
                        //K  Wakekill (Linux 2.6.33 to 3.13 only)
                        //W  Waking (Linux 2.6.33 to 3.13 only)
                        //P  Parked (Linux 3.9 to 3.13 only)
    long stat_ppid;     // The PID of the parent of this process.
    long pgrp;          // The process group ID of the process.
    long session;       // The session ID of the process.
    long tty_nr;        // The controlling terminal of the process.  (The minor
                        //device number is contained in the combination of
                        //bits 31 to 20 and 7 to 0; the major device number is
                        //in bits 15 to 8.)
    long tpgid;         // The ID of the foreground process group of the con‐
                        // trolling terminal of the process.
    unsigned long flags; // The kernel flags word of the process.  For bit mean‐
                        //ings, see the PF_* defines in the Linux kernel
                        //source file include/linux/sched.h.  Details depend
                        //on the kernel version.
                        //The format for this field was %lu before Linux 2.6.
    unsigned long minflt; //The number of minor faults the process has made
                        // which have not required loading a memory page from
                        // disk.
    unsigned long cminflt; // The number of minor faults that the process's
                        // waited-for children have made.
    unsigned long majflt; // The number of major faults the process has made
                        // which have required loading a memory page from disk.
    unsigned long cmajflt; // The number of major faults that the process's
                        // waited-for children have made.
    unsigned long utime; // Amount of time that this process has been scheduled
                        // in user mode, measured in clock ticks (divide by
                        // sysconf(_SC_CLK_TCK)).  This includes guest time,
                        // guest_time (time spent running a virtual CPU, see
                        // below), so that applications that are not aware of
                        // the guest time field do not lose that time from
                        // their calculations.
    unsigned long stime; // Amount of time that this process has been scheduled
                        // in kernel mode, measured in clock ticks (divide by
                        // sysconf(_SC_CLK_TCK)).
    long cutime;        // Amount of time that this process's waited-for chil‐
                        // dren have been scheduled in user mode, measured in
                        // clock ticks (divide by sysconf(_SC_CLK_TCK)).  (See
                        // also times(2).)  This includes guest time,
                        // cguest_time (time spent running a virtual CPU, see
                        // below).
    long cstime;        // Amount of time that this process's waited-for chil‐
                        // dren have been scheduled in kernel mode, measured in
                        // clock ticks (divide by sysconf(_SC_CLK_TCK)).
    long priority;      // (Explanation for Linux 2.6) For processes running a
                        //real-time scheduling policy (policy below; see
                        //sched_setscheduler(2)), this is the negated schedul‐
                        //ing priority, minus one; that is, a number in the
                        //range -2 to -100, corresponding to real-time priori‐
                        //ties 1 to 99.  For processes running under a non-
                        //real-time scheduling policy, this is the raw nice
                        //value (setpriority(2)) as represented in the kernel.
                        //The kernel stores nice values as numbers in the
                        //range 0 (high) to 39 (low), corresponding to the
                        //user-visible nice range of -20 to 19.
    long nice;          // The nice value (see setpriority(2)), a value in the
                        // range 19 (low priority) to -20 (high priority).
    long num_threads;   // Number of threads in this process (since Linux 2.6).
                        // Before kernel 2.6, this field was hard coded to 0 as
                        // a placeholder for an earlier removed field.
    long itrealvalue;   // The time in jiffies before the next SIGALRM is sent
                        // to the process due to an interval timer.  Since ker‐
                        // nel 2.6.17, this field is no longer maintained, and
                        // is hard coded as 0.
    unsigned long long starttime;// The time the process started after system boot.  In
                        // kernels before Linux 2.6, this value was expressed
                        // in jiffies.  Since Linux 2.6, the value is expressed
                        // in clock ticks (divide by sysconf(_SC_CLK_TCK)).
    unsigned long vsize; // Virtual memory size in bytes.
    unsigned long rss;  // Resident Set Size: number of pages the process has
                        // in real memory.  This is just the pages which count
                        // toward text, data, or stack space.  This does not
                        // include pages which have not been demand-loaded in,
                        // or which are swapped out.
    unsigned long rsslim; // Current soft limit in bytes on the rss of the
                        // process; see the description of RLIMIT_RSS in
                        // getrlimit(2).
    double m_load;      // load ratio 0..1
    uint32_t m_uid;
    uint32_t m_gid;
};

typedef std::shared_ptr<Process> pProcess;

class CompareByMem  {
    public:
    bool operator()(const pProcess &a, const pProcess &b) {
        if (!a) {
            return false;
        }
        if (!b) {
            return true;
        }
        return a->getMemUsage() > b->getMemUsage();
    }
};

class CompareByCpu  {
    public:
    bool operator()(const pProcess &a, const pProcess &b) {
        if (!a) {
            return false;
        }
        if (!b) {
            return true;
        }
        // getCpuUsage -> last value
        // getCpuUsageBuf -> sum buffered values but less actual
        return a->getCpuUsageBuf() > b->getCpuUsageBuf();
    }
};

