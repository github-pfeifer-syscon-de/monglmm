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

#include <iostream>

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
    if (process.getVmRssK() < 10000 || process.getVmRssK() > 50000) {
        std::cout << "The size " << process.getVmRssK() <<  " is not in expected range!" << std::endl;
        return false;
    }
    return true;
}

int
main(int argc, char** argv)
{
    Glib::init();
    if (!property_test()) {
        return 1;
    }
    return 0;
}