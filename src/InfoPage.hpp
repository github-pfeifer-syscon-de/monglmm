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

#include "Page.hpp"

class InfoPage : public Page
{
public:
    InfoPage();
    virtual ~InfoPage();

    void updateG15(Cairo::RefPtr<Cairo::Context> cr, guint width, guint height) override;
private:
    float sinTab[60];
    float cosTab[60];

    float getX(int min);
    float getY(int min);
    void clock(Cairo::RefPtr<Cairo::Context> cr, const Glib::DateTime &dt, float cx, float r );
};

