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
#include <stdio.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <errno.h>
#include <gtkmm.h>
#include <thread>
#include <future>
#include <math.h>

#include "libgtop_helper.h"
#ifdef LIBGTOP
#include <glibtop/netlist.h>
#include <glibtop/netload.h>
#endif
#include "HistMonitor.hpp"
#include "G15Worker.hpp"
#include "MonglView.hpp"
#include "NetMonitor.hpp"
#include "FileByLine.hpp"

NetMonitor::NetMonitor(guint points)
: HistMonitor{points, "NET"}
{
    m_enabled = false;
}


NetMonitor::~NetMonitor() {
}

 /*
  1 interface
  2 recv bytes
  3 recv packets
  4 recv errs
  5 recv drop
  6 recv fifo
  7 recv frame
  8 recv compressed
  9 recv multicast
 10 transm bytes
 11 transm packets
 12 transm errs
 13 transm drop
 14 transm fifo
 15 transm colls
 16 transm carrier
 17 transm compress
*/
struct net_stat {
    unsigned long recvBytes,recvPackets,recvErrs,recvDrop,recvFrame,recvCompressed,recvMulticast,
                transmBytes,transmPackets,transmErrs,transmDrop,transmFifo,transmColls,transmCarrier,transmCompress;
};


static struct net_stat previous_net_stat = { 0l, 0l, 0l, 0l, 0l, 0l, 0l, 0l,
                                            0l, 0l, 0l, 0l, 0l, 0l, 0l};
static gint64 previous_time = 0l;




void
NetMonitor::reinit()
{
    previous_time = 0l;
    // So we wont pickup sum on reactivation
    memset(&previous_net_stat, 0, sizeof(struct net_stat));
}


void
NetMonitor::net_device_changed(Gtk::Entry *device_entry)
{
    m_device = device_entry->get_text();
    reinit();
}


Gtk::Box *
NetMonitor::create_config_page()
{
    auto net_box = create_default_config_page(
            _("Display NET usage"),
            _("NET receive color"),
            _("NET transmit color"));

    auto device_entry = Gtk::manage(new Gtk::Entry());
    device_entry->set_text(m_device);
    add_widget2box(net_box, _("Device (.e.g eth0 or enp4s0)"), device_entry, 1.0f);
    device_entry->signal_changed().connect(
	sigc::bind<Gtk::Entry *>(
	    sigc::mem_fun(*this, &NetMonitor::net_device_changed),
	device_entry));

    return net_box;
}



gboolean
NetMonitor::update(int refreshRate, glibtop * glibtop)
{
    gboolean found = FALSE;
    char dev[80];
    struct net_stat net;
#ifdef LIBGTOP
    glibtop_netlist netlist;
    char** netNames = glibtop_get_netlist_l(glibtop, &netlist);

    for (unsigned int i = 0; i < netlist.number; ++i) {
        char *name = netNames[i];
        if (name == nullptr)
            break;
        if (!strcmp(name, "lo"))    // ignore lo
        {
            //std::cout << "net " << name << std::endl;
            glibtop_netload netload;

            glibtop_get_netload_l(glibtop, &netload, name);
            //std::cout << "  net " << dev
            //          << " flags " << netload.flags
            //          << " byte in " << netload.bytes_in
            //          << " byte out " << netload.bytes_out
            //          << " pack in " << netload.packets_in
            //          << " pack out " << netload.packets_out
            //          << std::endl;
            if ((m_device.length() > 0
              && strncmp(name, m_device.data(), m_device.length()) == 0) ||
                (m_device.length() == 0
              && netload.bytes_total > 0l))   // or shoud have traffic
            {
                strncpy(dev, name, sizeof(dev)-1);

                net.recvBytes = netload.bytes_in;
                net.transmBytes = netload.bytes_out;
                found = TRUE;
                break;
            }
        }
    }
    g_strfreev(netNames);
#else
    FileByLine netinfo;
    if (!netinfo.open("/proc/net/dev", "r")) {
        g_warning("monitors: Could not open /proc/net/dev: %d, %s",
                  errno, strerror(errno));
        m_enabled = FALSE;
        return(FALSE);
    }
    size_t len;
    int fscanf_result = 0;
    while (TRUE) {
        const char *line = netinfo.nextLine(&len);
        if (line == nullptr) {
            break;
        }
        if (len > 6 && line[6] == ':') {

            fscanf_result = sscanf(line, " %s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n",
                                        dev,&net.recvBytes,&net.recvPackets,&net.recvErrs,&net.recvDrop,&net.recvFrame,&net.recvCompressed,&net.recvMulticast,
                                        &net.transmBytes,&net.transmPackets,&net.transmErrs,&net.transmDrop,&net.transmFifo,&net.transmColls,&net.transmCarrier,&net.transmCompress);
            if (fscanf_result >= 16)
            {
                if (m_device.length() > 0
                    && strncmp(dev, m_device.data(), m_device.length()) == 0)
                {
                    found = TRUE;
                    break;
                }
                if ((m_device.length() == 0)
                    && strcmp(dev, "lo:") != 0
                    && (net.recvPackets > 0
                    ||  net.transmPackets > 0))  // default use the first device with traffic but not lo
                {
                    found = TRUE;
                    break;
                }
            }
        }
    }

#endif
    if (found)
    {
        unsigned long writeValue = 0l;
        unsigned long readValue = 0l;
        double delta_s = 1.0;

        m_used_device = std::string(dev);
        if (previous_net_stat.recvBytes > 0l
         || previous_net_stat.recvBytes > 0l) {
            // what do about wrapping ? -> it will give us a huge peak :)
            unsigned long net_delta_recvBytes = net.recvBytes - previous_net_stat.recvBytes;
            unsigned long net_delta_transmBytes = net.transmBytes - previous_net_stat.transmBytes;
            writeValue = net_delta_transmBytes;
            readValue = net_delta_recvBytes;
        }
        gint64 actual_time = g_get_monotonic_time();    // the promise is this does not get screwed up by time adjustments
        if (previous_time != 0)
        {
            gint64 delta_us = (actual_time - previous_time);
            if (delta_us == 0)                       // shoud hardly happen but just to be safe
                delta_us = refreshRate * 1E6;
            delta_s = 1.0E6/(double)delta_us;        // factor that converts to byte/s
        }
        else
            delta_s = 1.0/(double)refreshRate;

        previous_time = actual_time;


        /* Copy current to previous. */
        memcpy(&previous_net_stat, &net, sizeof(struct net_stat));
        // Raw display value

        // keep values in per second
        addPrimarySecondary((guint64)(readValue * delta_s), (guint64)(writeValue * delta_s));
    }
    return(TRUE);
}

unsigned long
NetMonitor::getTotal()
{
    float fmax = MAX(m_primaryHist.getMax(), m_secondaryHist.getMax());
    return (unsigned long)fmax;
}

void
NetMonitor::updateG15(Cairo::RefPtr<Cairo::Context> cr, guint width, guint height)
{
    double fmax = MAX(m_primaryHist.getMax(), m_secondaryHist.getMax());

    cr->move_to(1.0, 10.0);
    cr->show_text("Net");
    std::string total = formatScale(fmax, "B/s");
    cr->move_to(1.0, 20.0);
    cr->show_text(m_used_device);
    cr->move_to(1.0, 30.0);
    cr->show_text(total);

    cr->rectangle(60.5, 0.5, width-61, height-1);
    cr->stroke();
    double x = width - 1.5;
    for (int i = m_size-1; i >= 0; --i)
    {
        cr->move_to(x, height-1);
        float y = (float)(height-1) - ((MAX(getValues(0)->get(i), getValues(1)->get(i)))*(gfloat)(height-2));
        cr->line_to(x, y);
        x -= 1.0;
        if (x <= 60.0) {
            break;
        }
    }
    cr->stroke();

}

void
NetMonitor::load_settings(const Glib::KeyFile  * settings)
{
    config_setting_lookup_int(settings, m_name, CONFIG_DISPLAY_NET, &m_enabled);
    if (!config_setting_lookup_color(settings, m_name, CONFIG_NET_COLOR, m_foreground_color))
        m_foreground_color = Gdk::RGBA(NET_PRIMARY_DEFAULT_COLOR);

    if (!config_setting_lookup_color(settings, m_name, CONFIG_NET_SECONDARY_COLOR, m_secondary_color))
        m_secondary_color = Gdk::RGBA(NET_SECONDARY_DEFAULT_COLOR);

    if (!config_setting_lookup_string(settings, m_name, CONFIG_NET_DEVICE, m_device))
        m_device = Glib::ustring();
}

void
NetMonitor::save_settings(Glib::KeyFile  * setting)
{
    config_group_set_int(setting, m_name, CONFIG_DISPLAY_NET, m_enabled);
    config_group_set_color(setting, m_name, CONFIG_NET_COLOR, m_foreground_color);
    config_group_set_color(setting, m_name, CONFIG_NET_SECONDARY_COLOR, m_secondary_color);
    config_group_set_string(setting, m_name, CONFIG_NET_DEVICE, m_device);

}

std::string
NetMonitor::getPrimMax()
{
    return formatScale(getTotal(), "B/s");
}

std::string
NetMonitor::getSecMax()
{
    return m_used_device != "" ? std::string(m_used_device) : std::string("?");
}
