/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
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
#include <fstream>
#include <giomm.h>
#include <StringUtils.hpp>
#include <map>
#include <netdb.h>
#include <arpa/inet.h>  // htons
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <fcntl.h>
#include <vector>

#include "DiskInfo.hpp"
#include "Process.hpp"

static bool
property_test()
{
    std::cout << "property_test" << std::endl;
    pid_t pid = getpid();
    std::cout << "pid " << pid << std::endl;
    Process process{Glib::ustring::sprintf("/proc/%d", pid), pid, 100};
    process.update_status();
    process.update_stat();
    if (process.getStage() == psc::gl::TreeNodeState::Finished) {
        return false;
    }
    std::cout << "name " << process.getDisplayName() << std::endl;
    if ("process_test" != process.getDisplayName()) {
        std::cout << "Not the expected name!" << std::endl;
        return false;
    }
    std::cout << "vmRssK " << process.getVmRssK() << "k" << std::endl;
    if (process.getVmRssK() < 10000 || process.getVmRssK() > 100000) {
        std::cout << "The size " << process.getVmRssK() <<  " is not in expected range!" << std::endl;
        return false;
    }
    return true;
}

// can't decide what is the best method?
static bool
net_test_etcservices()
{
    std::map<uint32_t, Glib::ustring> portNames;
    std::ifstream stat;
    std::ios_base::iostate exceptionMask = stat.exceptions() | std::ios::failbit | std::ios::badbit ; // | std::ios::eofbit
    stat.exceptions(exceptionMask);
    try {   // alternativ  /usr/share/iana-etc/port-numbers.iana
        stat.open("/etc/services");
        std::string line;
        while (!stat.eof()) {   //  will only be set after we read past end, so we end up with a exception
            [[maybe_unused]] auto next = stat.peek();       // just to check eof
            if (stat.eof()) {
                break;
            }
            std::getline(stat, line);
            if (!line.starts_with('#')
             && line.length() > 3) {
                std::vector<Glib::ustring> parts;
                StringUtils::splitRepeat(line, ' ', parts);
                if (parts.size() >= 2) {
                    auto name = parts[0];
                    auto portProt = parts[1];
                    auto pos = portProt.find('/');
                    if (pos != portProt.npos) {
                        auto port = portProt.substr(0, pos);
                        auto prot = portProt.substr(pos + 1);
                        //std::cout << __FILE__ << "::setRemoteServiceName"
                        //          << " found " << name
                        //          << " port " << port
                        //          << " prot " << prot << std::endl;
                        if ("tcp" == prot) {
                            uint32_t porti = std::stoi(port);
                            portNames.insert(std::make_pair(porti, name));
                        }
                    }
                }
            }
        }
    }
    catch (const std::ios_base::failure& e) {
        std::cout << "Could not open /etc/services " << errno
             << " " << strerror(errno)
             << " ecode " << e.what() << std::endl;
    }
    if (stat.is_open()) {
       stat.close();
    }
    std::cout << __FILE__ "::net_test_etcservices"
                  << "entries " << portNames.size()
                  << std::endl;

    return true;
}


static bool
net_test_getservent_r()
{
    std::map<uint32_t, std::string> portNames;
    struct servent result_buf{};
    struct servent *result{};
    char buf[1024]{};
    while (true) {
        int ret = getservent_r(&result_buf,
                        buf, sizeof(buf),
                        &result);
        if (ret != 0 || result == nullptr) {
            break;
        }
        uint32_t port = ntohs(static_cast<uint16_t>(result->s_port));
        if (strstr(result->s_proto, "tcp") != nullptr) {
            portNames.insert(
                    std::pair(
                        port, result->s_name));
        }
    }
    std::cout << __FILE__ "::net_test_getservent_r"
              << "entries " << portNames.size()
              << std::endl;

    return true;
}
//    if (m_portNames.empty()) {
//        struct servent result_buf;
//        struct servent *result;
//        char buf[1024];
//        // getservbyport seems to have been deprecated ... (always returns null)
//        //   from the looks i would expect there is a free required ??? but the examples do not show any sign of this
//        int ret = getservbyport_r(htons(static_cast<uint16_t>(netConn->getRemotePort())), "tcp",
//                                  &result_buf, buf, sizeof(buf), &result);
//        if (ret == 0) {
//            if (strlen(buf) > 0) {
//                netConn.setRemoteSericeName(buf);   // same result_buf.s_name;
//            }
//            //if (result != nullptr) {
//            //    std::cout << __FILE_NAME__ << "::getRemoteServiceName "
//            //              << " name " << result->s_name
//            //              << " proto " << result->s_proto << std::endl;
//            //}
//        }
//        else {
//            std::cout << "errno " << errno << " " << strerror(errno) << std::endl;
//            std::cout << "Checking " << m_remotePort << " servent is null" << std::endl;
//        }
//        if (netConn.getRemoteSericeName().empty()) {   // still empty use number ...
//            netConn.setRemoteSericeName(Glib::ustring::sprintf("%d", m_remotePort));
//        }
//    }

static void
disk_print(std::map<std::string, PtrDiskInfo>& mapDisks)
{
    for (auto entryDisk : mapDisks) {
        auto& dev = entryDisk.second;
        std::cout << "disk " << entryDisk.first
                 // << " rwSect " << dev->getTotalRWSectors()
                  << " read " << dev->getBytesReadPerS() << " B/s"
                  << " write " << dev->getBytesWrittenPerS() << " B/s" << std::endl;
    }
}

static bool
writeTest(const std::string& file, size_t writeBytes, size_t& sum)
{
    //std::cout << "Creating file " << file << std::endl;
    std::srand(0u);
    // | O_DIRECT | O_DSYNC write synchronous, but slows down by factor 10, and reports 2 times io ?
    //   -> fsync at end seems sufficient
    auto fd = open(file.c_str(), O_CREAT | O_WRONLY );
    if (fd < 0) {
        std::cout << "Error " << strerror(errno) << " creating " << file << std::endl;
        return false;
    }
    std::array<int, 1024ul*64ul> buf;
    auto bufBytes{buf.size() * sizeof(int)};
    for (size_t i = 0; i < writeBytes / bufBytes; ++i ) {
        for (size_t j = 0; j < buf.size(); ++j) {
            buf[j] = std::rand();
        }
        auto len = write(fd, buf.data(), bufBytes);
        if (len < 0) {
            std::cout << "Error " << strerror(errno) << " writing " << file << std::endl;
            return false;
        }
        sum += len;
    }
    fsync(fd);
    close(fd);
    return true;
}

// this is a check your output test.
//   It will create a temp file, and write some data,
//   then show the statistic output.
//   @param  realistic: false: suited for everyday use, but limited result
//                      true:  read and understand the implications e.g. uses home
static bool
disk_test(bool realistic)
{
    std::cout << "disk_device ---------------------------------" << std::endl;
    const size_t writeBytes = realistic ? 1024ul*1024ul*512ul : 1024ul*1024ul*5ul;
    auto dir = realistic ? Glib::get_home_dir() : Glib::get_tmp_dir();
    std::map<std::string, PtrDiskInfo> mapDisks;
    gint64 start_time = g_get_monotonic_time();
    DiskInfo::getDiskStats(mapDisks, 0ul);
    std::string basename,file;
    Glib::RefPtr<Gio::File> gfile;
    do {
        basename = Glib::ustring::sprintf("test%ld.txt", start_time);
        file = Glib::canonicalize_filename(basename.c_str(), dir);
        gfile = Gio::File::create_for_path(file);
        ++start_time;
    } while (gfile->query_exists());    // avoid using existing
    size_t sum{};
    if (!writeTest(file, writeBytes, sum)) {
        return false;
    }
    gint64 end_time = g_get_monotonic_time();    // the promise is this does not get screwed up by time adjustments
    gint64 diff_us{end_time - start_time};
    DiskInfo::getDiskStats(mapDisks, diff_us);
    disk_print(mapDisks);
    auto writeMB = static_cast<double>(sum) / (1024.0*1024.0);
    auto timeS = static_cast<double>(diff_us) / 1.0e6;
    std::cout << "write " << writeMB << "MB"
              << " time " << diff_us << "us"
              << " " << timeS << "s" << std::endl;
    std::cout << "Estimated write rate " << writeMB / timeS << "MB/s" << std::endl;
    gfile->remove();
    //auto mnt = gfile->find_enclosing_mount();
    //auto drv =  mnt->get_drive();
    //auto vol = mnt->get_volume();
    //std::cout << "Drive " << (drv ? drv->get_name() : "")
    //          << " vol " << (vol ? vol->get_name() : "") << std::endl;
    std::cout << "disk_mount ---------------------------------" << std::endl;
    auto mnts = MountInfo::getMounts();
    for (auto& mnt : mnts) {
        mnt->refreshFilesys(nullptr);
        std::cout << "found mount " << mnt->getMount()
                  << " dev " << mnt->getDevName()
                  << " used " << mnt->getUsage()
                  << " free " << mnt->getFreePercent() << "%"
                  << " avail " << Monitor::formatScale(mnt->getAvailBytes(),"B") <<  std::endl;
    }
    return true;
}


int
main(int argc, char** argv)
{
    setlocale(LC_ALL, "");      // make locale dependent, and make glib accept u8 const !!!
    Gio::init();
    if (!property_test()) {
        return 1;
    }
    if (!net_test_etcservices()) {
        return 2;
    }
    if (!net_test_getservent_r()) {
        return 3;
    }
    if (!disk_test(false)) {
        return 4;
    }

    return 0;
}
