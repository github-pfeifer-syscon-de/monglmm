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
#include <fstream>
#include <error.h>
#include <cstdio>
#include <stdlib.h> // popen
#include <cstring>
#include <psc_format.hpp>
#include <StringUtils.hpp>
#include <filesystem>
#include <zlib.h>

#include "KernelParameter.hpp"

KernelParameter::KernelParameter(std::string_view name
                            , std::string_view info
                            , std::string_view test
                            , std::string_view persist)
: m_name{name}
, m_info{info}
, m_test{test}
, m_persist{persist}
{
}

std::string
KernelParameter::getName()
{
    return m_name;
}

std::string
KernelParameter::getInfo()
{
    return m_info;
}


std::string
KernelParameter::queryTimed()
{
    return "";
}

std::string
KernelParameter::getTest()
{
    return m_test;
}

std::string
KernelParameter::getPersist()
{
    return m_persist;
}

bool
KernelParameter::isTimed()
{
    return false;
}

bool
KernelParameter::isError()
{
    return m_error;
}

std::string
KernelParameter::getErrorMessage()
{
    return m_errorMessage;
}


std::vector<std::shared_ptr<KernelParameter>>
KernelParameter::getAllParameters()
{
    std::vector<std::shared_ptr<KernelParameter>> ret;
    ret.reserve(20);
    ret.emplace_back(std::move(std::make_shared<KernelParamSwappiness>()));
    ret.emplace_back(std::move(std::make_shared<VfsCachePressure>()));
    ret.emplace_back(std::move(std::make_shared<DirtyRatio>()));
    ret.emplace_back(std::move(std::make_shared<HugePages>()));
    ret.emplace_back(std::move(std::make_shared<SamePageMerge>()));
    ret.emplace_back(std::move(std::make_shared<KernelPressure>()));
    ret.emplace_back(std::move(std::make_shared<CpuFrequencyScaling>()));
    ret.emplace_back(std::move(std::make_shared<CpuSecurityMitigations>()));
    ret.emplace_back(std::move(std::make_shared<TicklessKernelOperation>()));
    ret.emplace_back(std::move(std::make_shared<ReadCopyUpdate>()));
    ret.emplace_back(std::move(std::make_shared<MaxMapCount>()));
    ret.emplace_back(std::move(std::make_shared<IoScheduler>()));
    ret.emplace_back(std::move(std::make_shared<ZSwap>()));
    ret.emplace_back(std::move(std::make_shared<VmStats>()));
    ret.emplace_back(std::move(std::make_shared<ReadAhead>()));

    return ret;
}

std::string
KernelParameter::cat(const std::string& name)
{
    std::string info;
    std::ifstream stat;
    // this seems the best of exceptions and state-checking
    std::ios_base::iostate exceptionMask = stat.exceptions() | std::ios::failbit | std::ios::badbit;
    stat.exceptions(exceptionMask);
    m_error = false;
    try {
        stat.open(name);
        std::string line;
        while (std::getline(stat, line)) {  // this check(eof) is just for in case the standard did change ...
            if (!line.empty()) {
                info += line + "\n";
            }
        }
    }
    catch (const std::ios_base::failure& e) {
        if (!stat.eof()) {
            m_error = true;
            m_errorMessage = psc::fmt::format("Could not open/read {} error {}", name, e.what());
        }
        //std::cout <<  << name << " "  << errno
        //     << " " << strerror(errno)
        //     << " ecode " << ;
        //Log.addLog(Level.INFO, oss1);
        // e.code() == std::io_errc::stream doesn't help either
    }
    if (stat.is_open()) {
        stat.close();
    }
    return info;
}

std::string
KernelParameter::zcat(const std::string& name, const std::string& parameter)
{
    std::string info;
    std::ifstream stat;
    // this seems the best of exceptions and state-checking
    std::ios_base::iostate exceptionMask = stat.exceptions() | std::ios::failbit | std::ios::badbit;
    stat.exceptions(exceptionMask);
    m_error = false;
    bool zlib_need_close{};
    z_stream zstrm{};
    try {
        stat.open(name);
        int zlib_status = inflateInit2(&zstrm, ZLIB_GZIP_AUTO_DETECT);     // 0 is use window size of stream ...  ZLIB_WINDOW_BITS | ENABLE_ZLIB_GZIP
        if (stat.good() && zlib_status == Z_OK) {
            zlib_need_close = true;
            std::array<unsigned char,ZLIB_CHUNK_SIZE> in;
            std::array<unsigned char,ZLIB_CHUNK_SIZE> out;
            std::string lastLines;
            while (stat.good() && info.empty()) {
                auto bytes_read = stat.readsome(reinterpret_cast<char *>(in.data()), in.size());
                if (bytes_read > 0) {
                    int flush = stat.eof() ? Z_FINISH : Z_NO_FLUSH;
                    zstrm.avail_in = bytes_read;
                    zstrm.next_in = in.data();
                    do {
                        zstrm.avail_out = out.size();
                        zstrm.next_out = out.data();
                        zlib_status = inflate(&zstrm, flush);
                        switch (zlib_status) {
                            case Z_OK:
                            case Z_STREAM_END:
                            case Z_BUF_ERROR:
                                break;
                            default:
                                m_error = true;
                                m_errorMessage = psc::fmt::format("Gzip error open/read {} error {}", name, zlib_status);
                                break;
                        }
                        if (zlib_status != Z_OK) {
                            break;
                        }
                        unsigned have = out.size() - zstrm.avail_out;
                        std::string lines(reinterpret_cast<const char*>(out.data()), have);
                        std::string check = lastLines + lines;    // concat last to avoid missing line when hitting border
                        auto pos = check.find(parameter);
                        if (pos != check.npos) {
                            auto nl = check.find("\n", pos);
                            if (nl != check.npos) {
                                info = check.substr(pos, nl - pos + 1);
                                break;
                            }
                        }
                        lastLines = lines;
                    } while (info.empty() && zstrm.avail_out == 0);
                }
            }
        }
    }
    catch (const std::ios_base::failure& e) {
        if (!stat.eof()) {
            m_error = true;
            m_errorMessage = psc::fmt::format("Could not open/read {} error {}", name, e.what());
        }
    }
    if (zlib_need_close) {
        inflateEnd(&zstrm);
    }
    if (stat.is_open()) {
        stat.close();
    }
    return info;
}

KernelParamSwappiness::KernelParamSwappiness()
: KernelParameter("Swappiness"
    , "Indicates the willingness for swapping process memory.\nHigher values for servers"
    , "sudo sysctl vm.swappiness=10"
    , "add/edit file in /etc/sysctl.d/ add vm.swappiness=10")
{
}

std::string
KernelParamSwappiness::query()
{
    return psc::fmt::format("vm.swappiness={}", cat(PROC_SWAPPINESS));
}

std::string
KernelParamSwappiness::getManualCommand()
{
    return sudo_cat + PROC_SWAPPINESS;
}


VfsCachePressure::VfsCachePressure()
: KernelParameter("Vfs cache pressure"
    , "Lower values prioritizes filesystem metadata in memory.\nHigher values for servers"
    , "sudo sysctl vm.vfs_cache_pressure=50"
    , "add/edit file in /etc/sysctl.d/ add vm.vfs_cache_pressure=50")
{
}

std::string
VfsCachePressure::query()
{
    return psc::fmt::format("vm.vfs_cache_pressure={}", cat(PROC_VFS_CACHE));
}

std::string
VfsCachePressure::getManualCommand()
{
    return sudo_cat + PROC_VFS_CACHE;
}

DirtyRatio::DirtyRatio()
: KernelParameter("Dirty ratio"
    , "Percentage of memory used for updated file data before flushed to disk\n(lower values lower risk of lost data in case of crash).\n"
      "dirty_background_ratio: percentage of memory is filled before write data in background."
    , "sudo sysctl vm.dirty_ratio=10  or... vm.dirty_background_ratio=5"
    ,"add/edit file in /etc/sysctl.d/ with need values")
{
}

std::string
DirtyRatio::query()
{
    return psc::fmt::format("vm.dirty_ratio={}vm.dirty_background_ratio={}",
         cat(DIRTY_RATIO)
        , cat(DIRTY_BACKGROUND_RATIO));
}

std::string
DirtyRatio::getManualCommand()
{
    return sudo_cat + DIRTY_RATIO + "\n"
         + sudo_cat + DIRTY_BACKGROUND_RATIO;
}

HugePages::HugePages()
: KernelParameter("Transparent huge pages"
    , "Kernel may map pages larger than the default (4k) transparently (enhances effort to defragment memory if always).\nThe value \"madvise\" is optimal for desktops allows applications to choose.\nActive value in []."
    , "echo madvice > sudo tee /sys/kernel/mm/transparent_hugepage/defrag"
    , "edit /etc/default/grub add \"transparent_hugepage=madvise\" for GRUB_CMDLINE_LINUX_DEFAULT")
{
}

std::string
HugePages::query()
{
    std::string info;
    info.reserve(128);
    auto huge = cat(HUGE_PAGES);
    if (!huge.empty()) {
        info += "enabled: " + huge;
    }
    auto defrag = cat(HUGE_PAGES_DEFRAG);
    if (!defrag.empty()) {
        info += "defrag: " + defrag;
    }
    return info;
}

std::string
HugePages::getManualCommand()
{
    return sudo_cat + HUGE_PAGES + "\n"
         + sudo_cat + HUGE_PAGES_DEFRAG;

}

SamePageMerge::SamePageMerge()
: KernelParameter("Kernel samepage merging"
    , "Kernel may merge pages shared between virtual machines for example (cost is page scanning).\nActive if 1."
    , "echo 0 > sudo tee /sys/kernel/mm/ksm/run")
{
}

std::string
SamePageMerge::query()
{
    std::string info;
    auto smp = cat(SAME_PAGE_MERGE);
    if (!smp.empty()) {
        info = "/sys/kernel/mm/ksm/run: " + smp;
    }
    return info;
}

std::string
SamePageMerge::getManualCommand()
{
    return sudo_cat + SAME_PAGE_MERGE;
}

KernelPressure::KernelPressure()
: KernelParameter("Kernel pressure"
    , "Kernel information about pressure situations")
{
}

std::string
KernelPressure::query()
{
    std::string info;
    info.reserve(256);
    auto cpu = cat(PRESSURE_CPU);
    if (!cpu.empty()) {
        info += "cpu:\n" + cpu;
    }
    auto memory = cat(PRESSURE_MEMORY);
    if (!memory.empty()) {
        info += "memory:\n" + memory;
    }
    auto io = cat(PRESSURE_IO);
    if (!io.empty()) {
        info += "io:\n" + io;
    }
    return info;    
}

std::string
KernelPressure::getManualCommand()
{
    return sudo_cat + PRESSURE_CPU + "\n"
        + sudo_cat + PRESSURE_MEMORY + "\n"
        + sudo_cat + PRESSURE_IO;
}

SchedulerAutoGroup::SchedulerAutoGroup()
: KernelParameter("Scheduler auto group"
    , "Group process scheduling by terminal sessions. If disabled processes are scheduled individually."
    , "sudo sysctl kernel.sched_autogroup_enabled=1"
    , "add/edit file in /etc/sysctl.d/ with need values")
{
}

std::string
SchedulerAutoGroup::query()
{
    std::string info;
    std::string sae = cat(SCHEDULER_AUTO_GROUP);
    if (!sae.empty()) {
        info = "kernel.sched_autogroup_enabled=" + sae;
    }
    return info;
}

std::string
SchedulerAutoGroup::getManualCommand()
{
    return sudo_cat + SCHEDULER_AUTO_GROUP;
}


CpuFrequencyScaling::CpuFrequencyScaling()
: KernelParameter("Cpu frequency scaling"
    , std::format("Allows {} the schedutil is a efficent choice.", cat("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors"))
    , "for cpu in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do\necho schedutil > sudo tee $cpu")
{
}

std::string
CpuFrequencyScaling::query()
{
    std::string info;
    info.reserve(256);
     for (const auto& entry : std::filesystem::directory_iterator(CPU_FREQUENCY_DIR)) {
        auto name = entry.path().filename().string();
        if (name.starts_with(CPU_FREQUENCY_BASE) && name.length() <= 5) {   // only looking for cpu0...99
            auto info_path = entry.path();
            info_path += CPU_FREQUENCY_ADD;
            info += psc::fmt::format("{}={}", info_path.string(), cat(info_path.string()));
        }
    }
    return info;
}

std::string
CpuFrequencyScaling::getManualCommand()
{
    return sudo_cat + CPU_FREQUENCY_DIR + CPU_FREQUENCY_BASE + "0" + CPU_FREQUENCY_ADD;
}

CpuSecurityMitigations::CpuSecurityMitigations()
: KernelParameter("Cpu security mitigations"
    , "List the security mitigations in place"
    ,""
    , "see https://www.kernel.org/doc/html/latest/admin-guide/kernel-parameters.html parameter mitigations=")
{
}

std::string
CpuSecurityMitigations::query()
{
    std::string info;
    info.reserve(1024);
    for (const auto& entry : std::filesystem::directory_iterator(CPU_SECURUTY_VULNERABILITES)) {
        auto name = entry.path().filename();
       info += psc::fmt::format("{}: {}", name.string(), cat(entry.path().string()));
    }
    return info;
}

std::string
CpuSecurityMitigations::getManualCommand()
{
    return std::string("See content of ") + CPU_SECURUTY_VULNERABILITES;
}

TicklessKernelOperation::TicklessKernelOperation()
: KernelParameter("Tickless kernel operation"
    , "Switch between fixed timing intervals or dynamic scheduling"
    ,""
    , "see https://www.kernel.org/doc/html/latest/admin-guide/kernel-parameters.html parameter nohz=")
{
}

std::string
TicklessKernelOperation::query()
{
    return zcat(KERNEL_PARAMETERS, TICKLESS_PARAMETER);
}

std::string
TicklessKernelOperation::getManualCommand()
{
    return std::string("sudo zcat ") +KERNEL_PARAMETERS + " | grep -F " + TICKLESS_PARAMETER;
}

ReadCopyUpdate::ReadCopyUpdate()
: KernelParameter("Read copy update"
    , "Seconds kernel waits to report serious error for reads or updates (usable for kernel debugging, extensive workloads)"
    ,""
    , psc::fmt::format("see https://www.kernel.org/doc/html/latest/admin-guide/kernel-parameters.html parameter {}=", RCU_PARAM))
{
}

std::string
ReadCopyUpdate::query()
{
    std::string info;
    auto rcu = cat(READ_COPY_UPDATE);
    if (!rcu.empty()) {
        info += std::string(RCU_PARAM) + "=" + rcu;
    }
    return info;
}

std::string
ReadCopyUpdate::getManualCommand()
{
    return sudo_cat  + READ_COPY_UPDATE;
}

MaxMapCount::MaxMapCount()
: KernelParameter("Max map count"
    , "Maximum number of memory mapped regions a process can use."
    , psc::fmt::format("sudo sysctl {}={}", MMC_PARAM, MMC_SUGGEST)
    , "add/edit file in /etc/sysctl.d/ with need values")
{
}

std::string
MaxMapCount::query()
{
    std::string info;
    auto mmc = cat(MAX_MAP_COUNT);
    if (!mmc.empty()) {
        info = std::string(MMC_PARAM) + "=" + mmc;
    }
    return info;
}

std::string
MaxMapCount::getManualCommand()
{
    return sudo_cat  + MAX_MAP_COUNT;
}

IoScheduler::IoScheduler()
: KernelParameter("IO Scheduler"
    , std::format("Allows selecting IO-Scheduler\n   mq-deadline: general purpose\n   kyber: for fast storage\n   bfq: prefer interactive tasks\n   none: hand requests to hardware\n   used value in []")
    , psc::fmt::format("echo bfq | sudo tee {}DEVICE{}", IO_SCHEDULE_DIR, IO_SCHEDULE_ADD)
    , "add/edit file in /etc/udev/rules.d/60-ioschedulers.rules add ACTION==\"add|change\",KERNEL==\"sd[a-z]\",ATTR{queue/scheduler}=\"bfq\" for nvme's use mvme[0-9]n[0-9] instead of sd[a-z]")
{
}

std::string
IoScheduler::query()
{
    std::string info;
    info.reserve(256);
    for (const auto& entry : std::filesystem::directory_iterator(IO_SCHEDULE_DIR)) {
        auto name = entry.path().filename().string();
        auto info_path = entry.path();
        info_path += IO_SCHEDULE_ADD;
        auto value = cat(info_path.string());
        if (!value.empty()) {
            info += name + "=" + value;
        }
    }
    return info;
}

std::string
IoScheduler::getManualCommand()
{
    std::string info;
    info.reserve(256);
    for (const auto& entry : std::filesystem::directory_iterator(IO_SCHEDULE_DIR)) {
        auto name = entry.path().filename().string();
        auto info_path = entry.path();
        info_path += IO_SCHEDULE_ADD;
        info += sudo_cat  + info_path.string() + "\n";
    }
    return info;
}


ZSwap::ZSwap()
: KernelParameter("Compress swapped-data"
    , std::format("Compress swapped memory on out-/input")
    , std::format("sudo echo 1 > {}", ZSWAP_PARAM)
    , "edit /etc/default/grub add \"zswap.enabled=1\" and/or \"zswap.compressor=lz4\" for GRUB_CMDLINE_LINUX_DEFAULT")
{
}

std::string
ZSwap::query()
{
    std::string info;
    info.reserve(128);
    auto zswap = cat(ZSWAP_PARAM);
    if (!zswap.empty()) {
        info += std::string(ZSWAP_PARAM) + "=" + zswap;
    }
    auto zcomp = cat(ZSWAP_COMPRESSOR);
    if (!zcomp.empty()) {
        info += std::string(ZSWAP_COMPRESSOR) + "=" + zcomp;
    }
    return info;
}

std::string
ZSwap::getManualCommand()
{
    return sudo_cat + ZSWAP_PARAM + "\n"
         + sudo_cat +ZSWAP_COMPRESSOR;
}

VmStats::VmStats()
: KernelParameter("VmStats"
    , "Swap/Paging statistics\n"
        "See vmstat for more values\n"
        "Units are 4k pages, io is difference to last")
{
}

std::string
VmStats::queryTimed()
{
    std::string info;
    std::ifstream stat;
    // for now avoid throwing exceptions as fail/eof look the same
    //std::ios_base::iostate exceptionMask = stat.exceptions() | std::ios::failbit | std::ios::badbit;
    //stat.exceptions(exceptionMask);
    try {
        stat.open(VMSTATS);
        uint64_t spwapin{}; // swapping
        uint64_t spwapout{};
        uint64_t pgpgin{};  // blocks passed to IO
        uint64_t pgpgout{};
        while (stat.good()) {
            std::string line;
            std::getline(stat, line);
            // "pgpgin"/"pgpgout"
            auto pos = line.find(' ');
            uint64_t val{};
            if (pos != line.npos) {
                val = std::stoul(line.substr(pos));
            }
            if (line.starts_with("pswpin")) {
                spwapin = val;
            }
            if (line.starts_with("pswpout")) {
                spwapout = val;
            }
            if (line.starts_with("pgpgin")) {
                pgpgin = val;
            }
            if (line.starts_with("pgpgout")) {
                pgpgout = val;
            }
        }
        info = psc::fmt::format("{:>12} {:>12} {:>12} {:>12}\n"
                    , spwapin, spwapout, pgpgin - m_last_pgin, pgpgout - m_last_pgout);
        m_last_pgin = pgpgin;
        m_last_pgout = pgpgout;
    }
    catch (const std::ios_base::failure& e) {
        std::cout << "Could not open/read " << VMSTATS << " " << errno
             << " " << strerror(errno)
             << " ecode " << e.code();
        //Log.addLog(Level.INFO, oss1);
        // e.code() == std::io_errc::stream doesn't help either
    }
    if (stat.is_open()) {
        stat.close();
    }

    return info;
}

std::string
VmStats::query()
{
    std::string info = psc::fmt::format("{:>12} {:>12} {:>12} {:>12}\n"
        , "swap: in", "out", "io: in", "out");
    info += queryTimed();
    return info;
}

bool
VmStats::isTimed()
{
    return true;
}

std::string
VmStats::getManualCommand()
{
    return sudo_cat + VMSTATS + " | grep -F pswp";
}

ReadAhead::ReadAhead()
: KernelParameter("Read ahead"
    , "Sectors that are read ahead sector size + count"
    , "Use \"sudo blockdev --setra VALUE DEVICE\" to change"
    , "add/edit file in /etc/udev/rules.d/60-readahead.rules add ACTION==\"add|change\",KERNEL==\"sd[a-z]\",ATTR{queue/read_ahead_kb}=\"8192\" for nvme's use mvme[0-9]n[0-9] instead of sd[a-z]")
{
}

std::string
ReadAhead::getBlockdev(const std::string& dev)
{   // this requires read permissions for device -> root
    std::string info;
    info.reserve(64);
    auto cmd = std::string("/bin/") + BLOCKDEV_CMD + dev;
    m_error = false;
    std::FILE* fp = popen(cmd.c_str(), "r");     /* Open the command for reading. */
    if (fp != nullptr) {
        std::array<char, 64> buffer{};      /* Read the output a line at a time  */
        auto ptr = std::fgets(buffer.data(), buffer.size(), fp);
        if (ptr != nullptr) {
            std::string line{buffer.data(), std::strlen(buffer.data())};
            info += line + "\n";
        }
        int res = pclose(fp);
        if (WIFEXITED(res) && WEXITSTATUS(res) != 0) {
            m_error = true;
            m_errorMessage = psc::fmt::format("Process exit {} device {}", WEXITSTATUS(res), dev);
        }
        else if (WIFSIGNALED(res)) {
            m_error = true;
            m_errorMessage = psc::fmt::format("Process signal {} device {}", WTERMSIG(res), dev);
        }
    }
    else {
        m_error = true;
        m_errorMessage = psc::fmt::format("Failed to run command {} device {}", cmd, dev);
    }
    return info;
}

std::string
ReadAhead::query()
{
    std::string info;
    info.reserve(128);
    for (const auto& entry : std::filesystem::directory_iterator(DEVICE_DIR)) {
        auto name = entry.path().filename().string();
        auto stat_path = entry.path();
        stat_path += "/stat";
        auto stats = cat(stat_path.string());
        StringUtils::trim(stats);
        auto stat_vals = StringUtils::splitConsec(stats, ' ');
        if (stat_vals.size() > 2 && stat_vals[0] != "0") {  // don't include unused devs e.g. cd-rom
            auto dev = std::string("/dev/") + name;
            auto readAhead = getBlockdev(dev);
        }
    }
    return info;
}

std::string
ReadAhead::getManualCommand()
{
    std::string info;
    info.reserve(128);
    for (const auto& entry : std::filesystem::directory_iterator(DEVICE_DIR)) {
        auto name = entry.path().filename().string();
        auto stat_path = entry.path();
        stat_path += "/stat";
        auto stats = cat(stat_path.string());
        StringUtils::trim(stats);
        auto stat_vals = StringUtils::splitConsec(stats, ' ');
        if (stat_vals.size() > 2 && stat_vals[0] != "0") {  // don't include unused devs e.g. cd-rom
            auto dev = std::string("/dev/") + name;
            info += std::string("sudo blockdev --getss --getra ") + dev + "\n";
        }
    }
    return info;
}
