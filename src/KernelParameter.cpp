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
#include <cstdlib> // popen
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
    ret.emplace_back(std::move(std::make_shared<SystemdServices>()));

    return ret;
}

void
KernelParameter::cat(const std::string& name, KernelParmValue& kernelParmValue)
{
    std::string info;
    std::ifstream stat;
    // this seems the best of exceptions and state-checking
    std::ios_base::iostate exceptionMask = stat.exceptions() | std::ios::failbit | std::ios::badbit;
    stat.exceptions(exceptionMask);
    try {
        stat.open(name);
        std::string line;
        while (std::getline(stat, line)) {  // this check(eof) is just for in case the standard did change ...
            if (!line.empty()) {
                kernelParmValue.addValue(line + "\n");
            }
        }
    }
    catch (const std::ios_base::failure& e) {
        if (!stat.eof()) {
            kernelParmValue.addError(psc::fmt::format("Could not open/read {} error {}", name, e.what()));
        }
    }
    if (stat.is_open()) {
        stat.close();
    }
}

void
KernelParameter::zcat(const std::string& name, const std::string& parameter, KernelParmValue& kernelParmValue)
{
    std::ifstream stat;
    // this seems the best of exceptions and state-checking
    std::ios_base::iostate exceptionMask = stat.exceptions() | std::ios::failbit | std::ios::badbit;
    stat.exceptions(exceptionMask);
    bool zlib_need_close{};
    bool entry_found{};
    z_stream zstrm{};
    try {
        stat.open(name);
        int zlib_status = inflateInit2(&zstrm, ZLIB_GZIP_AUTO_DETECT);     // 0 is use window size of stream ...  ZLIB_WINDOW_BITS | ENABLE_ZLIB_GZIP
        if (stat.good() && zlib_status == Z_OK) {
            zlib_need_close = true;
            std::array<unsigned char,ZLIB_CHUNK_SIZE> in;
            std::array<unsigned char,ZLIB_CHUNK_SIZE> out;
            std::string lastLines;
            while (stat.good() && !entry_found) {
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
                                kernelParmValue.addError(psc::fmt::format("Gzip error open/read {} error {}", name, zlib_status));
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
                                kernelParmValue.addValue(check.substr(pos, nl - pos + 1));
                                entry_found = true; // for now stop searching but may match multiple
                                break;
                            }
                        }
                        lastLines = lines;
                    } while (!entry_found && zstrm.avail_out == 0);
                }
            }
        }
    }
    catch (const std::ios_base::failure& e) {
        if (!stat.eof()) {
            kernelParmValue.addError(psc::fmt::format("Could not open/read {} error {}", name, e.what()));
        }
    }
    if (zlib_need_close) {
        inflateEnd(&zstrm);
    }
    if (stat.is_open()) {
        stat.close();
    }
}

KernelParamSwappiness::KernelParamSwappiness()
: KernelParameter("Swappiness"
    , "Indicates the willingness for swapping process memory.\nHigher values for servers"
    , "sudo sysctl vm.swappiness=10"
    , "add/edit file in /etc/sysctl.d/ add vm.swappiness=10")
{
}

KernelParmValue
KernelParamSwappiness::query()
{
    KernelParmValue kernelParamValue("vm.swappiness=");
    cat(PROC_SWAPPINESS, kernelParamValue);
    return kernelParamValue;
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

KernelParmValue
VfsCachePressure::query()
{
    KernelParmValue kernelParamValue("vm.vfs_cache_pressure=");
    cat(PROC_VFS_CACHE, kernelParamValue);
    return kernelParamValue;
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

KernelParmValue
DirtyRatio::query()
{
    KernelParmValue kernelParamValue;
    kernelParamValue.addValue("vm.dirty_ratio=");
    cat(DIRTY_RATIO, kernelParamValue);
    kernelParamValue.addValue("vm.dirty_background_ratio=");
    cat(DIRTY_BACKGROUND_RATIO, kernelParamValue);
    return kernelParamValue;
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

KernelParmValue
HugePages::query()
{
    KernelParmValue kernelParamValue;
    kernelParamValue.addValue("enabled: ");
    cat(HUGE_PAGES, kernelParamValue);
    kernelParamValue.addValue("defrag: ");
    cat(HUGE_PAGES_DEFRAG, kernelParamValue);
    return kernelParamValue;
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

KernelParmValue
SamePageMerge::query()
{
    KernelParmValue kernelParamValue("/sys/kernel/mm/ksm/run: ");
    cat(SAME_PAGE_MERGE, kernelParamValue);
    return kernelParamValue;
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

KernelParmValue
KernelPressure::query()
{
    KernelParmValue kernelParamValue;
    kernelParamValue.addValue("cpu:\n");
    cat(PRESSURE_CPU, kernelParamValue);
    kernelParamValue.addValue("memory:\n");
    cat(PRESSURE_MEMORY, kernelParamValue);
    kernelParamValue.addValue("io:\n");
    cat(PRESSURE_IO, kernelParamValue);
    return kernelParamValue;
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

KernelParmValue
SchedulerAutoGroup::query()
{
    KernelParmValue kernelParamValue("kernel.sched_autogroup_enabled=");
    cat(SCHEDULER_AUTO_GROUP, kernelParamValue);
    return kernelParamValue;
}

std::string
SchedulerAutoGroup::getManualCommand()
{
    return sudo_cat + SCHEDULER_AUTO_GROUP;
}


CpuFrequencyScaling::CpuFrequencyScaling()
: KernelParameter("Cpu frequency scaling"
    , ""
    , "for cpu in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do\necho schedutil > sudo tee $cpu")
{
}

KernelParmValue
CpuFrequencyScaling::query()
{
    KernelParmValue kernelParamValue;
    for (const auto& entry : std::filesystem::directory_iterator(CPU_FREQUENCY_DIR)) {
        auto name = entry.path().filename().string();
        if (name.starts_with(CPU_FREQUENCY_BASE) && name.length() <= 5) {   // only looking for cpu0...99
            auto info_path = entry.path();
            info_path += CPU_FREQUENCY_ADD;
            kernelParamValue.addValue(info_path.string() + "=");
            cat(info_path.string(), kernelParamValue);
        }
    }
    return kernelParamValue;
}

std::string
CpuFrequencyScaling::getInfo()
{
    if (m_info.empty()) {
        KernelParmValue infoVal;
        cat("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors", infoVal);
        m_info = std::format("Allows {} the schedutil is a efficent choice.", infoVal.getValue());
    }
    return m_info;
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

KernelParmValue
CpuSecurityMitigations::query()
{
    KernelParmValue kernelParamValue;
    std::string info;
    info.reserve(1024);
    for (const auto& entry : std::filesystem::directory_iterator(CPU_SECURUTY_VULNERABILITES)) {
        auto name = entry.path().filename();
        kernelParamValue.addValue(name.string() + ": ");
        cat(entry.path().string(), kernelParamValue);
    }
    return kernelParamValue;
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

KernelParmValue
TicklessKernelOperation::query()
{
    KernelParmValue kernelParamValue;
    zcat(KERNEL_PARAMETERS, TICKLESS_PARAMETER, kernelParamValue);
    return kernelParamValue;
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

KernelParmValue
ReadCopyUpdate::query()
{
    KernelParmValue kernelParamValue(RCU_PARAM);
    kernelParamValue.addValue("=");
    cat(READ_COPY_UPDATE, kernelParamValue);
    return kernelParamValue;
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

KernelParmValue
MaxMapCount::query()
{
    KernelParmValue kernelParamValue(MMC_PARAM);
    kernelParamValue.addValue("=");
    cat(MAX_MAP_COUNT, kernelParamValue);
    return kernelParamValue;
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

KernelParmValue
IoScheduler::query()
{
    KernelParmValue kernelParamValue;
    for (const auto& entry : std::filesystem::directory_iterator(IO_SCHEDULE_DIR)) {
        auto name = entry.path().filename().string();
        auto info_path = entry.path();
        info_path += IO_SCHEDULE_ADD;
        kernelParamValue.addValue(name + "=");
        cat(info_path.string(), kernelParamValue);
    }
    return kernelParamValue;
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

KernelParmValue
ZSwap::query()
{
    KernelParmValue kernelParamValue;
    kernelParamValue.addValue(std::string(ZSWAP_PARAM) + "=");
    cat(ZSWAP_PARAM, kernelParamValue);
    kernelParamValue.addValue(std::string(ZSWAP_COMPRESSOR) + "=");
    cat(ZSWAP_COMPRESSOR, kernelParamValue);
    return kernelParamValue;
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
    std::ios_base::iostate exceptionMask = stat.exceptions() | std::ios::failbit | std::ios::badbit;
    stat.exceptions(exceptionMask);
    uint64_t spwapin{}; // swapping
    uint64_t spwapout{};
    uint64_t pgpgin{};  // blocks passed to IO
    uint64_t pgpgout{};
    try {
        stat.open(VMSTATS);
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
    }
    catch (const std::ios_base::failure& e) {
        if (!stat.eof()) {
            info = std::string("Error ") + e.what();
        }
    }
    if (stat.eof()) {   // consider this as success
        info = psc::fmt::format("{:>12} {:>12} {:>12} {:>12}\n"
                    , spwapin, spwapout, pgpgin - m_last_pgin, pgpgout - m_last_pgout);
        m_last_pgin = pgpgin;
        m_last_pgout = pgpgout;
    }
    if (stat.is_open()) {
        stat.close();
    }
    return info;
}

KernelParmValue
VmStats::query()
{
    KernelParmValue kernelParamValue;
    std::string info = psc::fmt::format("{:>12} {:>12} {:>12} {:>12}\n"
        , "swap: in", "out", "io: in", "out");
    kernelParamValue.addValue(info);
    kernelParamValue.addValue(queryTimed());
    return kernelParamValue;
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

void
ReadAhead::getBlockdev(const std::string& dev, KernelParmValue& kernelParamValue)
{   // this requires read permissions for device -> so works only 4 root
    auto cmd = BLOCKDEV_CMD + dev + " 2>&1";    // added capture stderr
    // spills errors to stdout
    std::FILE* fp = popen(cmd.c_str(), "r");     /* Open the command for reading. */
    if (fp != nullptr) {
        std::array<char, 128> buffer{};      /* Read the output a line at a time  */
        char *ptr;
        while ((ptr = std::fgets(buffer.data(), buffer.size(), fp))) {
            if (std::ferror(fp)) {  // docs is unclear if this will return nullptr on fgets
                break;
            }
            std::string line{buffer.data(), std::strlen(buffer.data())};
            kernelParamValue.addValue(line + "\n");
        }
        int res = pclose(fp);
        if (WIFEXITED(res) && WEXITSTATUS(res) != 0) {
            kernelParamValue.addError(psc::fmt::format("Process exit {} device {}\n", WEXITSTATUS(res), dev));
        }
        else if (WIFSIGNALED(res)) {
            kernelParamValue.addError(psc::fmt::format("Process signal {} device {}\n", WTERMSIG(res), dev));
        }
    }
    else {
        kernelParamValue.addError(psc::fmt::format("Failed to run command {} device {}\n", cmd, dev));
    }
}

bool
ReadAhead::isDeviceUsed(const std::filesystem::path& dev_path)
{
    auto stat_path = dev_path;
    stat_path += "/stat";
    KernelParmValue kernelParamValue;
    cat(stat_path.string(), kernelParamValue);
    if (!kernelParamValue.hasError()) {
        std::string stats = kernelParamValue.getValue();
        StringUtils::trim(stats);
        auto stat_vals = StringUtils::splitConsec(stats, ' ');
        if (stat_vals.size() > 2) {  // don't include unused devs e.g. cd-rom
            auto read = std::stoul(stat_vals[0]);
            return read > 0;
        }
    }
    return false;
}

KernelParmValue
ReadAhead::query()
{
    KernelParmValue kernelParamValue;
    for (const auto& entry : std::filesystem::directory_iterator(DEVICE_DIR)) {
        auto name = entry.path().filename().string();
        auto stat_path = entry.path();
        if (isDeviceUsed(stat_path)) {
            auto dev = std::string("/dev/") + name;
            getBlockdev(dev, kernelParamValue);
        }
    }
    return kernelParamValue;
}

std::string
ReadAhead::getManualCommand()
{
    std::string info;
    info.reserve(128);
    for (const auto& entry : std::filesystem::directory_iterator(DEVICE_DIR)) {
        auto name = entry.path().filename().string();
        auto stat_path = entry.path();
        if (isDeviceUsed(stat_path)) {
            auto dev = std::string("/dev/") + name;
            info += std::string("sudo blockdev --getss --getra ") + dev + "\n";
        }
    }
    return info;
}

SystemdServices::SystemdServices()
: KernelParameter("Systemd services"
    , "Disabling unused services may improve performance (less important for a descent desktop but matters on a raspi) some suggestions:\n"
          "exim4 used for local mail delivery (for jobs...) -> \"dmg\" might be are more lightweight choice\n"
          "nfs used for file sharing -> consider scp/sshfs as these do not require a extra infrastructure\n"
          "cups used for printing -> if printing is available somewhere else"
    , "Use \"sudo systemctl stop SERVICE\" to change"
    , "Use \"sudo systemctl disable SERVICE\" or uninstall related package")
{
}

KernelParmValue
SystemdServices::query()
{
    KernelParmValue kernelParamValue;
    auto cmd = std::string(SYSCTL) + " 2>&1";
    // spills errors to stdout
    std::FILE* fp = popen(cmd.c_str(), "r");     /* Open the command for reading. */
    if (fp != nullptr) {
        std::array<char, 128> buffer{};      /* Read the output a line at a time  */
        char *ptr;
        while ((ptr = std::fgets(buffer.data(), buffer.size(), fp))) {
            if (std::ferror(fp)) {  // docs is unclear if this will return nullptr on fgets
                break;
            }
            std::string line{buffer.data(), std::strlen(buffer.data())};
            kernelParamValue.addValue(line);
        }
        int res = pclose(fp);
        if (WIFEXITED(res) && WEXITSTATUS(res) != 0) {
            kernelParamValue.addError(psc::fmt::format("Process error {}\n", WEXITSTATUS(res)));
        }
        else if (WIFSIGNALED(res)) {
            kernelParamValue.addError(psc::fmt::format("Process signal {}\n", WTERMSIG(res)));
        }
    }
    else {
        kernelParamValue.addError(psc::fmt::format("Failed to run command {}\n", cmd));
    }
    return kernelParamValue;
}

std::string
SystemdServices::getManualCommand()
{
    return SYSCTL;
}
