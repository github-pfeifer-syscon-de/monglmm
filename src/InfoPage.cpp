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

#include <iostream>
#include <sys/utsname.h>
#include <gtkmm.h>
#include <glibmm.h>
#include <math.h>
#include <time.h>
#include <pangomm.h>

#include "InfoPage.hpp"
#include "StringUtils.hpp"

static double
toRadians(double val) {
    return val / 180.0 * M_PI;
}

InfoPage::InfoPage()
{
    for (guint32 i = 0; i < ARRAYSIZE(sinTab); ++i) {
        double deg = (360 * i) / ARRAYSIZE(sinTab);
        double rad = toRadians(deg);
        sinTab[i] = (float)sin(rad);
        cosTab[i] = (float)cos(rad);
    }
}

InfoPage::~InfoPage()
{
}

void
InfoPage::updateG15(Cairo::RefPtr<Cairo::Context> cr, guint width, guint height)
{
    struct utsname utsname;
    int ret = uname(&utsname);
    if (ret == 0)
    {
        cr->move_to(1.0, 10.0);
        cr->show_text(utsname.nodename);
        //cr->move_to(1.0, 20.0);
        //snprintf(tmp, sizeof(tmp), "Do %s", utsname.domainname);
        //cr->show_text(tmp);
        cr->move_to(1.0, 20.0);
        //snprintf(tmp, sizeof(tmp), "Ma %s", utsname.machine);
        cr->show_text(utsname.machine);
        //cr->move_to(1.0, 40.0);
        //snprintf(tmp, sizeof(tmp), "No %s", utsname.nodename);
        //cr->show_text(tmp);
        cr->move_to(1.0, 30.0);
		//char tmp[130];
        //snprintf(tmp, sizeof(tmp), "%s %s", utsname.sysname, utsname.release);
		//char *pos = strchr(tmp, '-');
		//if (pos != nullptr) {
		//	++pos;
		//	pos = strchr(pos, '-');
		//	if (pos != nullptr) {
		//		*pos = '\0';	// cut after build number
		//	}
		//}
		// sysname Linux
		// release 5.10.63-15-rasberrypi4...
		// version #1 SMP Fri Oct d hh::mm:...
        cr->show_text(utsname.release);
    }
    else
    {
        char tmp[64];
        cr->move_to(1.0, 20.0);
        snprintf(tmp, sizeof(tmp), "Err: %d", ret);
        cr->show_text(tmp);
    }
    cr->move_to(1.0, 40.0);
    Glib::DateTime dt = Glib::DateTime::create_now_local();
    Glib::ustring time = dt.format("%a %x d:%j ");  // w:%V
    cr->show_text(time);
    // this might be an alternative more consistent rendering, but has less control of size
    //   other alterantive -> FT_Bitmap_Init allows b/w, but needs workaround for line drawing...
    //cr->move_to(1.0, 30.0);
    //Glib::RefPtr<Pango::Layout> layout = Pango::Layout::create(cr);
    //layout->set_text(time);
    //Pango::FontDescription desc = Pango::FontDescription("Monospace 7.5");
    //layout->set_font_description(desc);
    //layout->show_in_cairo_context(cr);

    clock(cr, dt, width-height, height/2);

}

float
InfoPage::getX(int min) {
    int cnt = ARRAYSIZE(sinTab);
    int mini = min % cnt;
    return sinTab[mini];
}

float
InfoPage::getY(int min) {
    int cnt = ARRAYSIZE(cosTab);
    int mini = min % cnt;
    return cosTab[mini];
}

static void
line(Cairo::RefPtr<Cairo::Context> cr, float x1, float y1, float x2, float y2)
{
    cr->move_to(x1, y1);
    cr->line_to(x2, y2);
    cr->stroke();
}

void
InfoPage::clock(Cairo::RefPtr<Cairo::Context> cr, const Glib::DateTime &dt, float cx, float r )
{
	//int r = MIN(w, h) / 2;
	//int cx = (w - r) / 2;
	float cy = r;
        cx += r;
        cr->move_to(cx+r, cy);
	cr->arc(cx, cy, r, toRadians(0), toRadians(360));
        cr->stroke();

	for (int i = 0; i < 60; i += 5) {
	    float in = 0.9f;
	    line(cr, cx+(getX(i)*in*r), cy-(getY(i)*in*r), cx+(getX(i)*r), cy-(getY(i)*r));
	}
	int hour = dt.get_hour() * 5 + (dt.get_minute() * 5 / 60);    // use /60 as we can convert these consistently
	int min = dt.get_minute();
	int sec = dt.get_second();

	line(cr, cx, cy, cx+(getX(sec)*0.9f*r), cy-(getY(sec)*0.9f*r));
	line(cr, cx, cy, cx+(getX(min)*0.8f*r), cy-(getY(min)*0.8f*r));
	line(cr, cx, cy, cx+(getX(hour)*0.6f*r), cy-(getY(hour)*0.6f*r));
}


