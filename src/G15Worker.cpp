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
#include <gtkmm.h>
#include <iostream>
#include <fontconfig/fontconfig.h>
#include <string.h>
#include <thread>
#include <future>

#include "libg15.h"

#include "Monitor.hpp"
#include "G15Worker.hpp"


static const char *G15_CMD = "cmd";
static const char *G15_GRP = "g15";

G15Worker::G15Worker(std::vector<std::shared_ptr<Page>> _pages, Glib::Dispatcher *_pDispatcher)
: page{_pages}
, g15display{0}
, errorCode{-1}
, cmdErrorCode{0}
, running{true}
, dorefresh{true}
, netstart{false}
, m_pDispatcher{_pDispatcher}
, m1on{"pkexec netctl start enp3s0"}
, m1off{"pkexec netctl stop enp3s0"}
{
    gcmd[0] = Glib::ustring("/usr/local/netbeans-9.0/bin/netbeans");
    gcmd[1] = Glib::ustring("/usr/lib/thunderbird/thunderbird");
    gcmd[2] = Glib::ustring("VirtualBox");
    gcmd[3] = Glib::ustring("gimp");
    gcmd[4] = Glib::ustring("xfce4-taskmanager");
    gcmd[5] = Glib::ustring("emacs");
}


G15Worker::~G15Worker()
{
}

bool
G15Worker::isRunning()
{
    return running;
}


// as the g15-function deals with x-bitmaps (network byte order ?)
//   convert cairo image here
static unsigned char *
cairo2g15Image(Cairo::RefPtr<Cairo::ImageSurface> g15pixmap)
{
    int h = (int)ceilf(G15_LCD_HEIGHT / 8.0f);
    unsigned char *data = new unsigned char [G15_LCD_WIDTH * h];
    memset(data, 0, G15_LCD_WIDTH * h);
    unsigned char *pixdata = g15pixmap->get_data();
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < G15_LCD_WIDTH; ++x) {
            for (int b = 0; b < 8; ++b) {
                unsigned int dest_mask = 1 << b;
                int yp =(y * 8 + b);
                if (yp < g15pixmap->get_height()) {
                    unsigned int ya = yp * g15pixmap->get_width();
                    unsigned int xp = x;
                    unsigned char d = pixdata[ya + xp];
                    if ((d & 0xff) > 0x60) {    // works best with font antialias
                        data[y * G15_LCD_WIDTH + x] |= dest_mask;
                    }
                }
            }
        }
    }
    return data;
}

void
G15Worker::g15_update()
{
    Cairo::RefPtr<Cairo::ImageSurface> g15pixmap = Cairo::ImageSurface::create(Cairo::FORMAT_A8, G15_LCD_WIDTH, G15_LCD_HEIGHT);
    if (g15pixmap->cobj() != nullptr)
    {
        // Create context with some useful defaults
        Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(g15pixmap);
        cr->set_source_rgba(1.0, 1.0, 1.0, 1.0);
        cr->set_line_width(1.0);
        cr->set_antialias(Cairo::Antialias::ANTIALIAS_NONE);

        FcPattern* fcFontPattern = FcPatternCreate();
        FcPatternAddString(fcFontPattern, FC_FAMILY, (FcChar8 *)"Monospace");   // mono is the most readable (but still not all glyphs are without artefacts), other "sans-serif""Source code"
        FcPatternAddString(fcFontPattern, FC_STYLE, (FcChar8 *)"Regular");
        FcPatternAddDouble(fcFontPattern, FC_SIZE, 9.0);
        FcPatternAddInteger(fcFontPattern, FC_PIXEL_SIZE, 9);

        FcConfigSubstitute(nullptr, fcFontPattern, FcMatchPattern);    // without leads to blocks
        FcDefaultSubstitute(fcFontPattern);     // without leads to bold
        FcResult result;
        FcPattern* resolved = FcFontMatch(nullptr, fcFontPattern, &result);

        Cairo::RefPtr<Cairo::FontFace> face = Cairo::FtFontFace::create(resolved);
        cr->set_font_face(face);
        //cr->set_font_size(9.0);      // 10 is the default
        Cairo::FontOptions fopt;
        fopt.set_antialias(Cairo::Antialias::ANTIALIAS_NONE);   // as our target will be monchrome GRAY
        fopt.set_hint_metrics(Cairo::HintMetrics::HINT_METRICS_OFF);
        //fopt.set_subpixel_order(Cairo::SubpixelOrder::SUBPIXEL_ORDER_RGB);
        cr->set_font_options(fopt);

        page[g15display]->updateG15(cr, G15_LCD_WIDTH, G15_LCD_HEIGHT);
        unsigned char *data = cairo2g15Image(g15pixmap);
        int ret = writePixmapToLCD(data);
        if (ret != 0) {
            // In case of error reinitalize (disconnected device or alike)
            fprintf(stderr, "G15 WritePixmap ret %d\n", ret);
        }
        delete[] data;

        FcPatternDestroy(resolved);
        FcPatternDestroy(fcFontPattern);
    }
}

void
G15Worker::refresh() {
    dorefresh = TRUE;
}

void
G15Worker::stop() {
    running = FALSE;
}

int
G15Worker::rundirect(const Glib::ustring &ucmd)
{
    const char* cmd = ucmd.c_str();
    fprintf(stderr, "Run %s\n", cmd);
    cmdErrorCode = system(cmd);
    fprintf(stderr, "Run %s ret: %d\n", cmd, cmdErrorCode);
    if (cmdErrorCode != 0)
    {
        strncpy(m_cmd, cmd, sizeof(m_cmd)-1);
        if (m_pDispatcher != nullptr)
            m_pDispatcher->emit();
    }
    return cmdErrorCode;
}

void
G15Worker::run(const Glib::ustring &cmd)
{
    // try to avoid heap garbage
    std::thread sysWorker([this, cmd]
    {
        rundirect(cmd);
    });
    sysWorker.detach();
}

void
G15Worker::do_work() {
#ifdef DEBUG
    libg15Debug(G15_LOG_INFO);
#else
    libg15Debug(G15_LOG_WARN);  //
#endif
    errorCode = initLibG15();
    if (errorCode == G15_NO_ERROR) {
        while (running)  {
            unsigned int pressed_keys = 0;
            int ret = getPressedKeys(&pressed_keys);
            if (ret == G15_NO_ERROR) {
                guint lastDisplay =  g15display;
                if (pressed_keys & G15_KEY_L1) {
                    if (g15display < page.size()-1)
                        ++g15display;
                    else
                        g15display = 0;
                }
                if (pressed_keys & G15_KEY_L2) {
                    g15display = 0;
                }
                if (pressed_keys & G15_KEY_L3) {
                    g15display = 1;
                }
                if (pressed_keys & G15_KEY_L4) {
                    g15display = 2;
                }
                if (pressed_keys & G15_KEY_L5) {
                    g15display = 3;
                }
                if (pressed_keys & G15_KEY_G1) {
                    run(gcmd[0]);
                }
                if (pressed_keys & G15_KEY_G2) {
                    run(gcmd[1]);
                }
                if (pressed_keys & G15_KEY_G3) {
                    run(gcmd[2]);
                }
                if (pressed_keys & G15_KEY_G4) {
                    run(gcmd[3]);
                }
                if (pressed_keys & G15_KEY_G5) {
                    run(gcmd[4]);
                }
                if (pressed_keys & G15_KEY_G6) {
                    run(gcmd[5]);
                }
                if (pressed_keys & G15_KEY_M1) {
                    int ret;
                    if (!netstart) {
                        ret = rundirect(m1on);
                        if (ret == 0) {
                            netstart = TRUE;
                            g15display = page.size() - 4;
                        }
                    }
                    else {
                        ret = rundirect(m1off);
                        if (ret == 0) {
                            netstart = FALSE;
                        }
                    }
                    fprintf(stderr, "Key m1 %s ret: %d\n", netstart ? "y" : "n", ret);
                    setLEDs(netstart ? 0x01 : 0x0);
                }

                if (lastDisplay != g15display) {
                    dorefresh = TRUE;
                }
                else {
                    usleep(100*1000);   // without anything to do sleep a while
                }
                if (running && dorefresh)
                {
                    g15_update();
                    dorefresh = FALSE;
                }
            }
            else {
                if (running) {
                    fprintf(stderr, "Error reading g15 keys: %d\n", ret);
                    sleep(1);
                }
            }

        }
        exitLibG15();
        //fprintf(stderr, "Exiting g15worker\n");
    }
    else {
        if (errorCode != G15_ERROR_OPENING_USB_DEVICE) {
            // as we don't want to unnerve user not using a g15 device with a error message
            //   drawback g15 users will not get a message when the permissions for the device are missing ...
            strncpy(m_cmd, "g15init", sizeof(m_cmd));
            if (m_pDispatcher != nullptr)
                m_pDispatcher->emit();
        }
        fprintf(stderr, "Error init g15 keys: %d\n", errorCode);
    }
    running = false;
}

gint
G15Worker::getErrorCode()
{
    return errorCode;
}

gint
G15Worker::getCmdErrorCode()
{
	return cmdErrorCode;
}

char *
G15Worker::getCmd() {
    return m_cmd;
}

void
G15Worker::cmd_changed(int idx, Gtk::Entry *entry)
{
    Glib::ustring txt = entry->get_text();
    switch (idx) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        gcmd[idx] = txt;
        break;
    case 6:
        m1on = txt;
        break;
    case 7:
        m1off = txt;
        break;
    }
}

Gtk::Box *
G15Worker::create_config()
{
    auto g15_box = Gtk::manage(new Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL, 0));
    for (int i = 0; i < N_G15_CMD; ++i) {
        auto gnEntry = Gtk::manage(new Gtk::Entry());
        gnEntry->set_text(gcmd[i]);
        char name[16];
        snprintf(name, sizeof(name), "G%d", i+1);
        Monitor::add_widget2box(g15_box, name, gnEntry, 0.0f);

        gnEntry->signal_changed().connect(sigc::bind<int, Gtk::Entry *>(
                                          sigc::mem_fun(*this, &G15Worker::cmd_changed),
                                          i, gnEntry));
    }

    auto m1entryOn = Gtk::manage(new Gtk::Entry());
    m1entryOn->set_text(m1on);
    Monitor::add_widget2box(g15_box, "M1on", m1entryOn, 0.0f);

    m1entryOn->signal_changed().connect(sigc::bind<int, Gtk::Entry *>(
                                      sigc::mem_fun(*this, &G15Worker::cmd_changed),
                                      6, m1entryOn));
    auto m1entryOff = Gtk::manage(new Gtk::Entry());
    m1entryOff->set_text(m1off);
    Monitor::add_widget2box(g15_box, "M1off", m1entryOff, 0.0f);

    m1entryOff->signal_changed().connect(sigc::bind<int, Gtk::Entry *>(
                                      sigc::mem_fun(*this, &G15Worker::cmd_changed),
                                      7, m1entryOff));


    return g15_box;
}

void G15Worker::read_config(Glib::KeyFile *m_config)
{
    for (int i = 0; i < N_G15_CMD; ++i) {
        char key[16];
        snprintf(key, sizeof(key), "%s%d", G15_CMD, i);
        config_setting_lookup_string(m_config, G15_GRP, key, gcmd[i]);
    }
    config_setting_lookup_string(m_config, G15_GRP, "m1on", m1on);
    config_setting_lookup_string(m_config, G15_GRP, "m1off", m1off);
}

void G15Worker::save_config(Glib::KeyFile *m_config)
{
    for (int i = 0; i < N_G15_CMD; ++i) {
        char key[16];
        snprintf(key, sizeof(key), "%s%d", G15_CMD, i);
        config_group_set_string(m_config, G15_GRP, key, gcmd[i]);
    }
    config_group_set_string(m_config, G15_GRP, "m1on", m1on);
    config_group_set_string(m_config, G15_GRP, "m1off", m1off);
}
