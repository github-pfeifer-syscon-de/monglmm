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

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <filesystem>

class KernelParmValue
{
public:
    KernelParmValue()
    {
        m_value.reserve(128);
        m_error.reserve(128);
    }
    KernelParmValue(const std::string& value)
    : KernelParmValue()
    {
        addValue(value);
    }
    KernelParmValue(const KernelParmValue& other) = default;
    virtual ~KernelParmValue() = default;

    void addValue(const std::string& val)
    {
        m_value += val;
    }
    std::string getValue()
    {
        return m_value;
    }
    void addError(const std::string& err)
    {
        m_error += err;
    }
    bool hasError() const
    {
        return !m_error.empty();
    }
    std::string getError()
    {
        return m_value + m_error;   // add value as it contains output
    }
private:
    std::string m_value;
    std::string m_error;
};

class KernelParameter {
public:
    KernelParameter(std::string_view name
                    , std::string_view info
                    , std::string_view test = ""
                    , std::string_view persist = "");
    virtual ~KernelParameter() = default;
    virtual std::string getName();
    virtual std::string getInfo();
    virtual KernelParmValue query() = 0;
    virtual std::string getManualCommand() = 0;
    virtual std::string queryTimed();
    virtual std::string getTest();
    virtual std::string getPersist();
    virtual bool isTimed();
    static std::vector<std::shared_ptr<KernelParameter>> getAllParameters();
protected:
    void cat(const std::string& name, KernelParmValue& value);
    void zcat(const std::string& name, const std::string& parameter, KernelParmValue& value);
    static constexpr auto KERNEL_PARAMETERS{"/proc/config.gz"};
    static constexpr auto ZLIB_GZIP_AUTO_DETECT{ 0x20 };
    static constexpr auto ZLIB_CHUNK_BITS{ 10u };   // from zlib perspective this is inefficient, but since we are copying data limit the used amount
    static constexpr auto ZLIB_CHUNK_SIZE{ 1u << ZLIB_CHUNK_BITS };
    const std::string sudo_cat = "sudo cat ";
protected:
    std::string m_name;
    std::string m_info;
    std::string m_test;
    std::string m_persist;
};


class KernelParamSwappiness
: public KernelParameter
{
public:
    KernelParamSwappiness();
    virtual ~KernelParamSwappiness() = default;
    KernelParmValue query() override;
    std::string getManualCommand() override;
protected:
    static constexpr auto PROC_SWAPPINESS{"/proc/sys/vm/swappiness"};
};


class VfsCachePressure
: public KernelParameter
{
public:
    VfsCachePressure();
    virtual ~VfsCachePressure() = default;
    KernelParmValue query() override;
    std::string getManualCommand() override;
protected:
    static constexpr auto PROC_VFS_CACHE{"/proc/sys/vm/vfs_cache_pressure"};
};

class DirtyRatio
: public KernelParameter
{
public:
    DirtyRatio();
    virtual ~DirtyRatio() = default;
    KernelParmValue query() override;
    std::string getManualCommand() override;
protected:
    static constexpr auto DIRTY_RATIO{"/proc/sys/vm/dirty_ratio"};
    static constexpr auto DIRTY_BACKGROUND_RATIO{"/proc/sys/vm/dirty_background_ratio"};
};


class HugePages
: public KernelParameter
{
public:
    HugePages();
    virtual ~HugePages() = default;
    KernelParmValue query() override;
    std::string getManualCommand() override;
protected:
    static constexpr auto HUGE_PAGES{"/sys/kernel/mm/transparent_hugepage/enabled"};
    static constexpr auto HUGE_PAGES_DEFRAG{"/sys/kernel/mm/transparent_hugepage/defrag"};


};

class SamePageMerge
: public KernelParameter
{
public:
    SamePageMerge();
    virtual ~SamePageMerge() = default;
    KernelParmValue query() override;
    std::string getManualCommand() override;
protected:
    static constexpr auto SAME_PAGE_MERGE{"/sys/kernel/mm/ksm/run"};
};

class KernelPressure
: public KernelParameter
{
public:
    KernelPressure();
    virtual ~KernelPressure() = default;
    KernelParmValue query() override;
    std::string getManualCommand() override;
protected:
    static constexpr auto PRESSURE_CPU{"/proc/pressure/cpu"};
    static constexpr auto PRESSURE_MEMORY{"/proc/pressure/memory"};
    static constexpr auto PRESSURE_IO{"/proc/pressure/io"};
};

class SchedulerAutoGroup
: public KernelParameter
{
public:
    SchedulerAutoGroup();
    virtual ~SchedulerAutoGroup() = default;
    KernelParmValue query() override;
    std::string getManualCommand() override;
protected:
    static constexpr auto SCHEDULER_AUTO_GROUP{"/proc/sys/kernel/sched_autogroup_enabled"};
};

class CpuFrequencyScaling
: public KernelParameter
{
public:
    CpuFrequencyScaling();
    virtual ~CpuFrequencyScaling() = default;
    KernelParmValue query() override;
    std::string getManualCommand() override;
    std::string getInfo() override;
protected:
    static constexpr auto CPU_FREQUENCY_DIR{"/sys/devices/system/cpu/"};
    static constexpr auto CPU_FREQUENCY_BASE{"cpu"};
    static constexpr auto CPU_FREQUENCY_ADD{"/cpufreq/scaling_governor"};
};

class CpuSecurityMitigations
: public KernelParameter
{
public:
    CpuSecurityMitigations();
    virtual ~CpuSecurityMitigations() = default;
    KernelParmValue query() override;
    std::string getManualCommand() override;
protected:
    static constexpr auto CPU_SECURUTY_VULNERABILITES{"/sys/devices/system/cpu/vulnerabilities/"};

};

class TicklessKernelOperation
: public KernelParameter
{
public:
    TicklessKernelOperation();
    virtual ~TicklessKernelOperation() = default;
    KernelParmValue query() override;
    std::string getManualCommand() override;
protected:
    static constexpr auto TICKLESS_PARAMETER{"CONFIG_NO_HZ="};

};

class ReadCopyUpdate
: public KernelParameter
{
public:
    ReadCopyUpdate();
    virtual ~ReadCopyUpdate() = default;
    KernelParmValue query() override;
    std::string getManualCommand() override;
protected:
    static constexpr auto READ_COPY_UPDATE{"/sys/module/rcupdate/parameters/rcu_cpu_stall_timeout"};
    static constexpr auto RCU_PARAM{"rcupdate.rcu_cpu_stall_timeout"};

};

class MaxMapCount
: public KernelParameter
{
public:
    MaxMapCount();
    virtual ~MaxMapCount() = default;
    KernelParmValue query() override;
    std::string getManualCommand() override;
protected:
    static constexpr auto MAX_MAP_COUNT{"/proc/sys/vm/max_map_count"};
    static constexpr auto MMC_PARAM{"vm.max_map_count"};
    static constexpr auto MMC_SUGGEST{1048576};
};

class IoScheduler
: public KernelParameter
{
public:
    IoScheduler();
    virtual ~IoScheduler() = default;
    KernelParmValue query() override;
    std::string getManualCommand() override;
protected:
    static constexpr auto IO_SCHEDULE_DIR{"/sys/block/"};
    static constexpr auto IO_SCHEDULE_ADD{"/queue/scheduler"};
};

class ZSwap
: public KernelParameter
{
public:
    ZSwap();
    virtual ~ZSwap() = default;
    KernelParmValue query() override;
    std::string getManualCommand() override;
protected:
    static constexpr auto ZSWAP_PARAM{"/sys/module/zswap/parameters/enabled"};
    static constexpr auto ZSWAP_COMPRESSOR{"/sys/module/zswap/parameters/compressor"};
};

class VmStats
: public KernelParameter
{
public:
    VmStats();
    virtual ~VmStats() = default;
    KernelParmValue query() override;
    std::string queryTimed() override;
    bool isTimed() override;
    uint64_t m_last_pgin{};
    uint64_t m_last_pgout{};
    std::string getManualCommand() override;
protected:
    static constexpr auto VMSTATS{"/proc/vmstat"};
};

class ReadAhead
: public KernelParameter
{
public:
    ReadAhead();
    virtual ~ReadAhead() = default;
    KernelParmValue query() override;
    std::string getManualCommand() override;
protected:
    void getBlockdev(const std::string& dev, KernelParmValue& kernelParamValue);
    bool isDeviceUsed(const std::filesystem::path& dev_path);

    static constexpr auto BLOCKDEV_CMD{"blockdev --getra "};
    static constexpr auto DEVICE_DIR{"/sys/block/"};

};