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

// Monitor that is scaled by actual maximum

#pragma once

#include "Monitor.hpp"

class HistMonitor : public Monitor {
public:
    HistMonitor(guint points, const char *_name);
    HistMonitor(const HistMonitor& orig);
    virtual ~HistMonitor();

    void roll() override;
    void addPrimarySecondary(guint64 primaryValue, guint64 secondaryValue);

protected:
    Buffer<guint64> m_primaryHist;
    Buffer<guint64> m_secondaryHist;
private:

};

