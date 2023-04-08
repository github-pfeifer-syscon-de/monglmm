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


#include "HistMonitor.hpp"

HistMonitor::HistMonitor(guint points, const char *_name)
: Monitor(points, _name)
, m_primaryHist{points}
, m_secondaryHist{points}
{
}

HistMonitor::HistMonitor(const HistMonitor& orig)
: Monitor(orig.m_size, orig.m_name)
, m_primaryHist{orig.m_primaryHist}
, m_secondaryHist{orig.m_secondaryHist}
{
}

HistMonitor::~HistMonitor() {
}

void
HistMonitor::roll() {
    getValues(0)->roll();      // These two are not strictly required
    getValues(1)->roll();
    m_primaryHist.roll();
    m_secondaryHist.roll();
}

void
HistMonitor::addPrimarySecondary(guint64 primaryValue, guint64 secondaryValue)
{
    m_primaryHist.set(primaryValue);
    m_secondaryHist.set(secondaryValue);
    guint64 max = std::max(m_primaryHist.getMax(), m_secondaryHist.getMax());
    for (guint i = 0; i < m_size; i++)
    {
        getValues(0)->set(i, m_primaryHist.get(i) / (double)max);
        getValues(1)->set(i, m_secondaryHist.get(i) / (double)max);
    }
    getValues(0)->refreshSum();
    getValues(1)->refreshSum();
}
