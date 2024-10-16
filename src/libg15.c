/*
    This file is part of g15tools.

    g15tools is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    g15tools is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libg15; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    (c) 2006-2007 The G15tools Project - g15tools.sf.net

    $Revision: 292 $ -  $Date: 2008-11-11 08:51:25 -0500 (Tue, 11 Nov 2008) $ $Author: aneurysm9 $
     see https://sourceforge.net/projects/g15tools/
*/

#include <pthread.h>
#include "libg15.h"
#include <stdio.h>
#include <stdarg.h>
#include <libusb.h> // version 1.0
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/stat.h>
//#include "config.h"
#include "math.h"
#define PACKAGE_STRING "libG15static reworked"

static libusb_context *context = NULL;
static libusb_device_handle *keyboard_device = NULL;
static int libg15_debugging_enabled = 0;
static int enospc_slowdown = 0;

static int found_devicetype = -1;
static int shared_device = 0;
static int g15_keys_endpoint = 0;
static int g15_lcd_endpoint = 0;
static uint32_t g15_interface;
static pthread_mutex_t libusb_mutex;
static int WRITE_TIMEOUT_MS = 1000;
static int MAX_CLAIM_RETRY = 10;
static struct sG15Worker* g15Worker = NULL;

#ifndef TRUE
enum
  {
    FALSE,
    TRUE
  };
  typedef int bool;
#endif



struct libg15_transfer {
    struct libusb_transfer* transfer;
    unsigned char *buffer;
    const size_t length;        // buffer allocated length
    unsigned int result;        // result e.g. key_code
    int memory_type;            // see enum transfer_memory
    int active;                 // see enum transfer_active
    int free;                   // see enum transfer_free
};

typedef struct libg15_transfer libg15_transfer;

 enum transfer_memory // struct libg15_transfer.memory_type
  {
    TRANSFER_MEMORY_MALLOC,
    TRANSFER_MEMORY_DEVICE
  };

  enum transfer_active // struct libg15_transfer.active
  {
      TRANSFER_INACTIVE,
      TRANSFER_ACTIVE
  };

  enum transfer_free // struct libg15_transfer.free
  {
      TRANSFER_KEEP,
      TRANSFER_FREE
  };
/* keyboard (Gxx) transfer related */
static libg15_transfer key_transfer = {
    .transfer = NULL, .buffer = NULL, .length = G15_KEY_READ_LENGTH, .result = 0u,
    .memory_type = TRANSFER_MEMORY_MALLOC, .active = TRANSFER_INACTIVE,
    .free = TRANSFER_KEEP};

/* display (lcd) transfer related */
static libg15_transfer lcd_transfer = {
    .transfer = NULL, .buffer = NULL, .length = G15_BUFFER_LEN, .result = 0u,
    .memory_type = TRANSFER_MEMORY_MALLOC, .active = TRANSFER_INACTIVE,
    .free = TRANSFER_KEEP};

/* to add a new device, simply create a new DEVICE() in this list */
/* Fields are: "Name",VendorID,ProductID,Capabilities */
const libg15_devices_t g15_devices[] = {
    DEVICE("Logitech G15",0x46d,0xc222,G15_LCD|G15_KEYS),
    DEVICE("Logitech G11",0x46d,0xc225,G15_KEYS),
    DEVICE("Logitech Z-10",0x46d,0x0a07,G15_LCD|G15_KEYS|G15_DEVICE_IS_SHARED),
    DEVICE("Logitech G15 v2",0x46d,0xc227,G15_LCD|G15_KEYS|G15_DEVICE_5BYTE_RETURN),
    DEVICE("Logitech Gamepanel",0x46d,0xc251,G15_LCD|G15_KEYS|G15_DEVICE_IS_SHARED),
    DEVICE(NULL,0,0,0)
};

static void handle_transfer_events(int skipCancelled);
void lcd_transfer_cb(struct libusb_transfer *transfer);
void key_transfer_cb(struct libusb_transfer *transfer);
static unsigned int processKeyEvent5Byte(unsigned char *buffer);
static unsigned int processKeyEvent9Byte(unsigned char *buffer);


/* return device capabilities */
int g15DeviceCapabilities() {
    if(found_devicetype >= 0)
        return g15_devices[found_devicetype].caps;
    else
        return 0;       // no device no caps
}

/* debugging wrapper */
static int g15_log(unsigned int level, const char *fmt, ...)
{
    if (libg15_debugging_enabled
     && level >= libg15_debugging_enabled) {    // this might not be to anyones taste, but as we want still look for errors a log is best to identify some remaining issues
	if (g15Worker) {
            char log[128];
	    va_list argp;
            va_start(argp, fmt);
	    vsnprintf(log, sizeof(log)-1, fmt, argp);	// only vsnprintf works here, snprintf will fail with varargs!
            va_end(argp);
	    log_callback(g15Worker, level, log);
	}
    }

    return 0;
}

static void
libusb_log_callback(libusb_context *ctx, enum libusb_log_level level, const char *str)
{
    int g15_level = G15_LOG_INFO;
    if (level < LIBUSB_LOG_LEVEL_INFO) {
        g15_level = G15_LOG_WARN;
    }
    g15_log(g15_level, str);
}

/* enable or disable debugging */
void libg15Debug(int option)
{
    libg15_debugging_enabled = option;
    if (context == NULL) {      // will be set with open
        return;
    }

    int level = LIBUSB_LOG_LEVEL_WARNING;      // while testing use warn
    if (option == G15_LOG_INFO)
        level = LIBUSB_LOG_LEVEL_INFO;
    else if (option == G15_LOG_WARN)
        level = LIBUSB_LOG_LEVEL_WARNING;
    int ret = libusb_set_option(context, LIBUSB_OPTION_LOG_LEVEL, level);
    if (ret != 0) {
        g15_log(G15_LOG_WARN,"libusb_set_option log_level error %d", ret);
    }
    libusb_set_log_cb(context, libusb_log_callback, LIBUSB_LOG_CB_CONTEXT);
}

static void free_transfer(libg15_transfer *transfer)
{
    if (transfer->transfer) {
        libusb_free_transfer(transfer->transfer);
        transfer->transfer = NULL;
    }
    if (transfer->buffer) {
        if (transfer->memory_type == TRANSFER_MEMORY_DEVICE) {
            int ret = libusb_dev_mem_free(keyboard_device, transfer->buffer, transfer->length);
            if (ret != 0) {
                g15_log(G15_LOG_WARN, "libusb_dev_mem_free error %d %s length %d", ret, libusb_strerror(ret), transfer->length);
            }
        }
        else {
            free(transfer->buffer);
        }
        transfer->buffer = NULL;
    }
}

static int alloc_transfer(libg15_transfer *transfer, libusb_device_handle *device, int endpoint, libusb_transfer_cb_fn callback)
{
    transfer->transfer = libusb_alloc_transfer(0);

    transfer->buffer = libusb_dev_mem_alloc(device, transfer->length);   // do it the fancy way
    if (transfer->buffer == NULL) {
        g15_log(G15_LOG_INFO, "libusb_dev_mem_alloc no DMA, will use standard memory.");
        transfer->buffer = malloc(transfer->length);
        transfer->memory_type = TRANSFER_MEMORY_MALLOC;
    }
    else {
        transfer->memory_type = TRANSFER_MEMORY_DEVICE;
    }
    libusb_fill_interrupt_transfer(transfer->transfer, device, endpoint, transfer->buffer, transfer->length, callback, transfer, WRITE_TIMEOUT_MS);
    transfer->result = 0;

    return transfer->transfer != NULL && transfer->buffer ? G15_NO_ERROR : G15_ERROR_WRITING_BUFFER;
}


/* return number of connected and supported devices */
int g15NumberOfConnectedDevices() {
    unsigned int found = 0;

    //libusb_device *devs[];
    //ssize_t count = libusb_get_device_list(context, &devs);
    //for (ssize_t i = 0; i < count; ++i) {
    for (int j=0; g15_devices[j].name != NULL; j++) {
        // play it simple finding only the first device seems acceptable
        libusb_device_handle* handle = libusb_open_device_with_vid_pid(context, g15_devices[j].vendorid, g15_devices[j].productid);
        if (handle != NULL) {
            ++found;
            libusb_close(handle);
        }
    }
    //}
    //libusb_free_device_list(list, count);
    g15_log(G15_LOG_INFO,"Found %d supported devices", found);
    return found;
}

static int initLibUsb()
{
    if (!context) {   // init only once
        int ret = libusb_init(&context);
        if (ret != 0)
        {
            g15_log(G15_LOG_WARN,"libusb init error %d", ret);
            return G15_ERROR_OPENING_USB_DEVICE;
        }
        g15_log(G15_LOG_DEBUG,"libusb init ok %d", ret);
        if (libg15_debugging_enabled > 0) {     // if debug was called before set it now
            libg15Debug(libg15_debugging_enabled);
        }
    }

    return G15_NO_ERROR;
}

static int find_config(libusb_device_handle* handle, struct libusb_config_descriptor *cfg, uint32_t conf)
{
    int ret = G15_NO_ERROR;
    g15_keys_endpoint = 0;
    g15_lcd_endpoint = 0;

    for (uint32_t intf = 0; intf < cfg->bNumInterfaces; intf++) {
        const struct libusb_interface *ifp = &cfg->interface[intf];
        /* if endpoints are already known, finish up */
        if (g15_keys_endpoint && g15_lcd_endpoint)
            break;
        g15_log(G15_LOG_DEBUG, "Device has %d alternate Settings", ifp->num_altsetting);

        for (int alt = 0; alt < ifp->num_altsetting; alt++) {
            const struct libusb_interface_descriptor *as = &ifp->altsetting[alt];
            /* verify that the interface is for a HID device */
            if (as->bInterfaceClass == LIBUSB_CLASS_HID) {
                g15_log(G15_LOG_DEBUG, "Interface 0x%x has %hhd Endpoints", conf, as->bNumEndpoints);
                //usleep(50*1000);
                int kernel_active = libusb_kernel_driver_active(handle, intf);
                if (kernel_active == 1) {
                    ret = libusb_detach_kernel_driver(handle, intf);
                    if (ret != 0) {
                         g15_log(G15_LOG_WARN,"Error %d %s trying to pry device from kernel, continue.", ret, libusb_strerror(ret));
                    }
                }

                /* don't set configuration if device is shared */
                if (shared_device == 0) {
                    ret = libusb_set_configuration(handle, 1);
                    if (ret) {
                        const char *usb_err = libusb_strerror(ret);
                        g15_log(G15_LOG_WARN,"Error %d %s setting the configuration, this is fatal", ret, usb_err);
                        return ret;
                    }
                }
                //usleep(50*1000);
                int retries = 0;
                while((ret = libusb_claim_interface(handle, intf)) && retries < MAX_CLAIM_RETRY) {
                    usleep(50*1000);
                    retries++;
                    g15_log(G15_LOG_INFO,"Trying to claim interface retry %d", retries);
                }

                if (ret) {
                    g15_log(G15_LOG_WARN,"Error claiming interface, good day cruel world");
                    if (kernel_active == 1) {
                        libusb_attach_kernel_driver(handle, intf);
                    }
                    return ret;
                }
                g15_interface = intf;

                for (int endp = 0; endp < as->bNumEndpoints; endp++){
                    const struct libusb_endpoint_descriptor *ep = &as->endpoint[endp];
                    g15_log(G15_LOG_DEBUG, "Found %s endpoint %d with address 0x%x maxpacketsize=%d ",
                            (0x80&ep->bEndpointAddress?"\"Extra Keys\"":"\"LCD\""),
                            (int)(ep->bEndpointAddress&0x0f), (int)ep->bEndpointAddress, (int)ep->wMaxPacketSize);

                    if(0x80 & ep->bEndpointAddress) {
                        g15_keys_endpoint = ep->bEndpointAddress;
                    } else {
                        g15_lcd_endpoint = ep->bEndpointAddress;
                    }
                }

                if (!g15_keys_endpoint
                 || !g15_lcd_endpoint) {   // if we coud not get a complete config, unwind
                    libusb_release_interface(handle, intf);
                    if (kernel_active == 1) {
                        libusb_attach_kernel_driver(handle, intf);
                    }
                }
                //if (ret) {
                //    g15_log(G15_LOG_INFO, "Error setting Alternate Interface\n");
                //}
            }
        }
    }
    return ret;
}

static libusb_device_handle *findAndOpenDevice(libg15_devices_t handled_device, int device_index)
{
    //libusb_device *devs[];
    //ssize_t count = libusb_get_device_list(context, &devs);
    //for (ssize_t i = 0; i < count; ++i) {
    // play it simple finding only the first device seems acceptable (more we wont handle anyway)
    libusb_device_handle* handle = libusb_open_device_with_vid_pid(context, handled_device.vendorid, handled_device.productid);
    if (handle != NULL) {
        found_devicetype = device_index;
        g15_log(G15_LOG_INFO,"Found %s opened it", handled_device.name);
        libusb_device* dev = libusb_get_device(handle);
        struct libusb_device_descriptor desc;
        int ret = libusb_get_device_descriptor(dev, &desc);
        if (ret != 0) {
            g15_log(G15_LOG_WARN, "Device %s error %d %s on get_device_descriptor",
                    handled_device.name, ret, libusb_strerror(ret));
            return NULL;
        }
        g15_log(G15_LOG_DEBUG, "Device has %hhd possible configurations", desc.bNumConfigurations);

        /* if device is shared with another driver, such as the Z-10 speakers sharing with alsa, we have to disable some calls */
        if(g15DeviceCapabilities() & G15_DEVICE_IS_SHARED)
          shared_device = 1;

        for (uint32_t conf = 0; conf < desc.bNumConfigurations; conf++) {
            struct libusb_config_descriptor *cfg;
            ret = libusb_get_config_descriptor(dev, conf, &cfg);
            if (ret != 0) {
                g15_log(G15_LOG_WARN, "Device %s error %d %s on get_config_descriptor",
                        handled_device.name, ret, libusb_strerror(ret));
                continue;
            }

            ret = find_config(handle, cfg, conf);
            libusb_free_config_descriptor(cfg);
            if (ret) {
                return NULL;
            }
        }
        g15_log(G15_LOG_DEBUG, "Done opening the keyboard %p", handle);
        //usleep(500*1000); // sleep a bit for good measure
        return handle;
    }

    //}
    //libusb_free_device_list(list, count);
    return 0;
}


static  libusb_device_handle * findAndOpenG15() {
    int i;
    for (i=0; g15_devices[i].name != NULL; i++){
        g15_log(G15_LOG_DEBUG, "Trying to find usb-dev 0x%hx", g15_devices[i].productid);
        keyboard_device = findAndOpenDevice(g15_devices[i], i);
        if (keyboard_device) {
            break;
        }
        //else
        //    g15_log(G15_LOG_DEBUG, "not found 0x%hx", g15_devices[i].productid);
    }
    return keyboard_device;
}



int initLibG15()
{
    int retval = G15_NO_ERROR;
    retval = initLibUsb();
    if (retval)
        return retval;

    g15_log(G15_LOG_DEBUG,PACKAGE_STRING);
    //#ifdef SUN_LIBUSB	// this will to be supported...
    //    g15_log(G15_LOG_INFO,"Using Sun libusb.");
    //#endif
    //g15NumberOfConnectedDevices();  // this seems mostly useless,rather useable for debug

    keyboard_device = findAndOpenG15();
    if (!keyboard_device)
        return G15_ERROR_OPENING_USB_DEVICE;

    pthread_mutex_init(&libusb_mutex, NULL);
    return retval;
}

static void cancel_transfer(const char* key, libg15_transfer *transfer, libusb_transfer_cb_fn callback)
{
    if (transfer->transfer != NULL) {
        transfer->free = TRANSFER_FREE;          // indicate we are about to complete
        if (transfer->active == TRANSFER_ACTIVE) { // with active transfer bring it down by event handling
            g15_log(G15_LOG_WARN, "%s transfer active libusb_cancel_transfer status %d", key, transfer->transfer->status);
            int ret = libusb_cancel_transfer(transfer->transfer);
            if (ret == LIBUSB_SUCCESS) {      // if this succeeds
                handle_transfer_events(TRUE);    // give handling some chance to finish up
            }
            else {
                g15_log(G15_LOG_WARN, "%s transfer active libusb_cancel_transfer ret %d", key, ret);
                transfer->active = TRANSFER_INACTIVE;   // so direct handling may work as alternative
            }
        }
        if (transfer->transfer != NULL                // bring it down directly
         && transfer->active == TRANSFER_INACTIVE) {
            transfer->transfer->status = LIBUSB_TRANSFER_CANCELLED;
            callback(transfer->transfer);             // let callback do the work
        }
    }
}

/* just free device so reinit may work  */
int freeLibG15usb()
{
    int retval = G15_NO_ERROR;
    if (keyboard_device) {
        cancel_transfer("key", &key_transfer, key_transfer_cb);
        cancel_transfer("lcd", &lcd_transfer, lcd_transfer_cb);
        retval = libusb_release_interface(keyboard_device, g15_interface);
        if (retval != LIBUSB_SUCCESS)  {
            g15_log(G15_LOG_WARN, "libusb_release_interface ret %d", retval);
        }
        //usleep(50*1000);

        if (retval != LIBUSB_ERROR_NO_DEVICE) {     // with this status close wont work...
            libusb_close(keyboard_device);
        }
        keyboard_device = NULL;
        g15_log(G15_LOG_DEBUG, "libusb_close device done ret %d", retval);
        return retval;
    }
    return -1;
}

/* release the device and the lib, returning it to a known state */
int exitLibG15()
{
    int retval = G15_NO_ERROR;
    if (keyboard_device){
	retval = freeLibG15usb();
        pthread_mutex_destroy(&libusb_mutex);
    }
    else {
        retval = -1;
    }
    if (context) {
        libusb_exit(context);
        context = NULL;
    }
    g15_log(G15_LOG_DEBUG, "exitLibG15 ret %d", retval);
    return retval;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void dumpPixmapIntoLCDFormat(unsigned char *lcd_buffer, unsigned char const *data)
{
/*

  For a set of bytes (A, B, C, etc.) the bits representing pixels will appear on the LCD like this:

	A0 B0 C0
	A1 B1 C1
	A2 B2 C2
	A3 B3 C3 ... and across for G15_LCD_WIDTH bytes
	A4 B4 C4
	A5 B5 C5
	A6 B6 C6
	A7 B7 C7

	A0
	A1  <- second 8-pixel-high row starts straight after the last byte on
	A2     the previous row
	A3
	A4
	A5
	A6
	A7
	A8

	A0
	...
	A0
	...
	A0
	...
	A0
	A1 <- only the first three bits are shown on the bottom row (the last three
	A2    pixels of the 43-pixel high display.)


*/

    unsigned int output_offset = G15_LCD_OFFSET;
    unsigned int base_offset = 0;
    unsigned int curr_row = 0;
    unsigned int curr_col = 0;

    /* Five 8-pixel rows + a little 3-pixel row.  This formula will calculate
       the minimum number of bytes required to hold a complete column.  (It
       basically divides by eight and rounds up the result to the nearest byte,
       but at compile time.
      */

#define G15_LCD_HEIGHT_IN_BYTES  ((G15_LCD_HEIGHT + ((8 - (G15_LCD_HEIGHT % 8)) % 8)) / 8)

    for (curr_row = 0; curr_row < G15_LCD_HEIGHT_IN_BYTES; ++curr_row)
    {
        for (curr_col = 0; curr_col < G15_LCD_WIDTH; ++curr_col)
        {
            unsigned int bit = curr_col % 8;
		/* Copy a 1x8 column of pixels across from the source image to the LCD buffer. */

            lcd_buffer[output_offset] =
			(((data[base_offset                        ] << bit) & 0x80) >> 7) |
			(((data[base_offset +  G15_LCD_WIDTH/8     ] << bit) & 0x80) >> 6) |
			(((data[base_offset + (G15_LCD_WIDTH/8 * 2)] << bit) & 0x80) >> 5) |
			(((data[base_offset + (G15_LCD_WIDTH/8 * 3)] << bit) & 0x80) >> 4) |
			(((data[base_offset + (G15_LCD_WIDTH/8 * 4)] << bit) & 0x80) >> 3) |
			(((data[base_offset + (G15_LCD_WIDTH/8 * 5)] << bit) & 0x80) >> 2) |
			(((data[base_offset + (G15_LCD_WIDTH/8 * 6)] << bit) & 0x80) >> 1) |
			(((data[base_offset + (G15_LCD_WIDTH/8 * 7)] << bit) & 0x80) >> 0);
            ++output_offset;
            if (bit == 7)
              base_offset++;
        }
	/* Jump down seven pixel-rows in the source image, since we've just
	   done a row of eight pixels in one pass (and we counted one pixel-row
  	   while we were going, so now we skip the next seven pixel-rows.) */
	base_offset += G15_LCD_WIDTH - (G15_LCD_WIDTH / 8);
    }
}
#pragma GCC diagnostic pop

int handle_usb_errors(const char *prefix, int ret) {

    const char *usb_err = libusb_strerror(ret);
    g15_log(G15_LOG_WARN, "Usb error: %s %s (%d)", prefix, usb_err, ret);
    switch (ret) {
// leagacy error codes, dont expect that many workarounds for modern usb ...
//        case -ETIMEDOUT:
//            return G15_ERROR_READING_USB_DEVICE;  /* backward-compatibility */
//            break;
//            case -ENOSPC: /* the we dont have enough bandwidth, apparently.. something has to give here.. */
//                g15_log(G15_LOG_INFO,"usb error: ENOSPC.. reducing speed\n");
//                enospc_slowdown = 1;
//                break;
//            case -ENODEV:   /* the device went away - we probably should attempt to reattach */
//            case -ENXIO:    /* host controller bug */
//                g15_log(G15_LOG_INFO,"usb no dev error: %s %s (%d)\n",prefix,usb_err,ret);
//                //exitLibG15();
//		int errval = freeLibG15usb();
//		fprintf(stderr, "usb lost %s %s (%d) free %d\n", prefix, usb_err, ret, errval);
//		// leave open to next access
//		break;
//            case -EINVAL: /* invalid request */
//            case -EAGAIN: /* try again */
//            case -EFBIG: /* too many frames to handle */
//            case -EMSGSIZE: /* msgsize is invalid */
//                 g15_log(G15_LOG_INFO,"usb error: %s %s (%d)\n",prefix,usb_err,ret);
//                 break;
//            case -EPIPE: /* endpoint is stalled */
//                 g15_log(G15_LOG_INFO,"usb error: %s EPIPE! clearing...\n",prefix);
//                 pthread_mutex_lock(&libusb_mutex);
//                libusb_clear_halt(keyboard_device, 0x81);
//                 pthread_mutex_unlock(&libusb_mutex);
//                 break;
            case LIBUSB_ERROR_NO_DEVICE:
                key_transfer.active = TRANSFER_INACTIVE;    // as exiting by event handling wont work
                lcd_transfer.active = TRANSFER_INACTIVE;    //   and this seems like a logical consequence
                int errval = freeLibG15usb();        // free device
		g15_log(G15_LOG_INFO, "usb lost free %d", errval);
                break;
    }
    return ret;
}

void lcd_transfer_cb(struct libusb_transfer *transfer)
{
    struct libg15_transfer *usr_transfer = transfer->user_data;
    if (transfer->status == LIBUSB_TRANSFER_CANCELLED
     || transfer->status == LIBUSB_TRANSFER_NO_DEVICE) {
        if (usr_transfer->free == TRANSFER_FREE
         || transfer->status == LIBUSB_TRANSFER_NO_DEVICE) {
            // tear down structure if requested
            free_transfer(usr_transfer);
        }
    }
    usr_transfer->active = TRANSFER_INACTIVE;
}

void key_transfer_cb(struct libusb_transfer *transfer)
{
    libg15_transfer *usr_transfer = transfer->user_data;

    //fprintf(stderr,"libusb_transfer_cb status %d len %d\n", transfer->status, transfer->actual_length);
    if (transfer->status == LIBUSB_TRANSFER_CANCELLED
     || transfer->status == LIBUSB_TRANSFER_NO_DEVICE) {
        if (transfer->status == LIBUSB_TRANSFER_NO_DEVICE
         || usr_transfer->free == TRANSFER_FREE) {
            free_transfer(usr_transfer);
        }
        usr_transfer->active = TRANSFER_INACTIVE;
    }
    else {
        if (transfer->status == LIBUSB_TRANSFER_COMPLETED) {
            //fprintf(stderr,"handling len %d\n", actual_buffer_length);
            unsigned char *buf = usr_transfer->buffer;
            unsigned int pressed_keys = 0u;
            switch(transfer->actual_length) {
              case 5:
                  pressed_keys = processKeyEvent5Byte(buf);
                  break;
              case 9:
                  pressed_keys = processKeyEvent9Byte(buf);
                  break;
                default:
                    g15_log(G15_LOG_INFO, "Unrecognized keyboard packet [%d]: %x, %x, %x, %x, %x, %x, %x", transfer->actual_length, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);
                    //if(buffer[0] == 1) {
                    //    if (actual_buffer_length == 7) {	// with a ret of 7 we get scan-code buffer[1] g5 = 62, g6 = 63
                    //                                  //   and as the device will forget the next packet if set the lcd, read next
                    //    }
                    //}
                    break;
            }
            usr_transfer->result |= pressed_keys;    // or it together so we may get multiple keys when polling
        }
        int ret = libusb_submit_transfer(key_transfer.transfer);   // just do it again...
        if (ret != 0) {
            usr_transfer->active = TRANSFER_INACTIVE;
            handle_usb_errors("Keyboard submit transfer",ret);
        }
    }
}

int writePixmapToLCD(unsigned char const *data)
{
    int ret = 0;
    int transfercount=0;
    unsigned char lcd_buffer[G15_BUFFER_LEN];
    /* The pixmap conversion function will overwrite everything after G15_LCD_OFFSET, so we only need to blank
       the buffer up to this point.  (Even though the keyboard only cares about bytes 0-23.) */
    memset(lcd_buffer, 0, G15_LCD_OFFSET);  /* G15_BUFFER_LEN); */
    // changed this for a prepared buffer
    memcpy(lcd_buffer + G15_LCD_OFFSET, data, G15_LCD_WIDTH * (int)ceilf(G15_LCD_HEIGHT / 8.0f));
    //dumpPixmapIntoLCDFormat(lcd_buffer, data);

    if(!(g15_devices[found_devicetype].caps & G15_LCD))
        return 0;

    /* the keyboard needs this magic byte */
    lcd_buffer[0] = 0x03;
  /* in an attempt to reduce peak bus utilisation, we break the transfer into 32 byte chunks and sleep a bit in between.
    It shouldnt make much difference, but then again, the g15 shouldnt be flooding the bus enough to cause ENOSPC, yet
    apparently does on some machines...
    I'm not sure how successful this will be in combatting ENOSPC, but we'll give it try in the real-world. */

    int g15ret = G15_NO_ERROR;
    if(enospc_slowdown != 0){
#ifndef LIBUSB_BLOCKS
        pthread_mutex_lock(&libusb_mutex);
#endif
	if (keyboard_device) {
            for(transfercount = 0;transfercount<=31;transfercount++){
                int act_length = 32;
                ret = libusb_interrupt_transfer(keyboard_device, g15_lcd_endpoint, lcd_buffer+(32*transfercount), act_length, &act_length, WRITE_TIMEOUT_MS);
                if (ret != LIBUSB_SUCCESS) {
                    handle_usb_errors("LCDPixmap Slow Write", ret);
                    g15ret = G15_ERROR_WRITING_PIXMAP;
                    break;
                }
                usleep(100);
            }
        }
#ifndef LIBUSB_BLOCKS
        pthread_mutex_unlock(&libusb_mutex);
#endif
    }
    else {
	if (!keyboard_device) {
	    pthread_mutex_lock(&libusb_mutex);
	    // try to reattach
	    keyboard_device = findAndOpenG15();
	    pthread_mutex_unlock(&libusb_mutex);
	}
	if (keyboard_device) {
            if (lcd_transfer.transfer == NULL) {
                alloc_transfer(&lcd_transfer, keyboard_device, g15_lcd_endpoint, lcd_transfer_cb);
            }
            if (lcd_transfer.transfer != NULL
             && lcd_transfer.active == TRANSFER_ACTIVE) {   // if not yet finished
                handle_transfer_events(FALSE);  // finish up previous transfer
                if (lcd_transfer.active == TRANSFER_ACTIVE) {   // if hanging
                    g15_log(G15_LOG_WARN, "last lcd transfer still active: %d status %d", lcd_transfer.active, lcd_transfer.transfer->status);
                    libusb_cancel_transfer(lcd_transfer.transfer);
                    handle_transfer_events(FALSE);  // try to handle these
                }
            }
            if (lcd_transfer.transfer != NULL
             && lcd_transfer.active == TRANSFER_INACTIVE) {                 // only try again if previous is finished
                memcpy(lcd_transfer.buffer, lcd_buffer, G15_BUFFER_LEN);    // transfer entire buffer in one hit, in background by dma
                int ret = libusb_submit_transfer(lcd_transfer.transfer);
                if (ret != 0) {
                    handle_usb_errors("Lcd submit transfer", ret);
                    free_transfer(&lcd_transfer);
                }
                else {
                    lcd_transfer.active = TRANSFER_ACTIVE;
                }
            }
            else {
                g15_log(G15_LOG_WARN, "previous transfer unfinished");
                g15ret = G15_ERROR_WRITING_PIXMAP;      // something seems serious wrong
            }
        }
    }

    return g15ret;
}

int setLCDContrast(unsigned int level)
{
    int retval = 0;
    unsigned char usb_data[] = { 2, 32, 129, 0 };

    if (!keyboard_device)
        return G15_ERROR_TRY_AGAIN;

    if(shared_device>0)
        return G15_ERROR_UNSUPPORTED;

    switch(level)
    {
        case 1:
            usb_data[3] = 22;
            break;
        case 2:
            usb_data[3] = 26;
            break;
        default:
            usb_data[3] = 18;
    }
    #ifndef LIBUSB_BLOCKS
    pthread_mutex_lock(&libusb_mutex);
    #endif
    retval = libusb_control_transfer(keyboard_device, LIBUSB_REQUEST_TYPE_CLASS + LIBUSB_RECIPIENT_INTERFACE, 9, 0x302, 0, usb_data, 4, 10000);
    #ifndef LIBUSB_BLOCKS
    pthread_mutex_unlock(&libusb_mutex);
    #endif
    return retval;
}

int setLEDs(unsigned int leds)
{
    int retval = 0;
    unsigned char m_led_buf[4] = { 2, 4, 0, 0 };
    m_led_buf[2] = ~(unsigned char)leds;

    if (!keyboard_device)
        return G15_ERROR_TRY_AGAIN;

    if(shared_device>0)
        return G15_ERROR_UNSUPPORTED;

    #ifndef LIBUSB_BLOCKS
    pthread_mutex_lock(&libusb_mutex);
    #endif
    retval = libusb_control_transfer(keyboard_device, LIBUSB_REQUEST_TYPE_CLASS + LIBUSB_RECIPIENT_INTERFACE, 9, 0x302, 0, m_led_buf, 4, 10000);
    #ifndef LIBUSB_BLOCKS
    pthread_mutex_unlock(&libusb_mutex);
    #endif
    return retval;
}

int setLCDBrightness(unsigned int level)
{
    int retval = 0;
    unsigned char usb_data[] = { 2, 2, 0, 0 };

    if (!keyboard_device)
        return G15_ERROR_TRY_AGAIN;

    if(shared_device>0)
        return G15_ERROR_UNSUPPORTED;

    switch(level)
    {
        case 1 :
            usb_data[2] = 0x10;
            break;
        case 2 :
            usb_data[2] = 0x20;
            break;
        default:
            usb_data[2] = 0x00;
    }
    #ifndef LIBUSB_BLOCKS
    pthread_mutex_lock(&libusb_mutex);
    #endif
    retval = libusb_control_transfer(keyboard_device, LIBUSB_REQUEST_TYPE_CLASS + LIBUSB_RECIPIENT_INTERFACE, 9, 0x302, 0, usb_data, 4, 10000);
    #ifndef LIBUSB_BLOCKS
    pthread_mutex_unlock(&libusb_mutex);
    #endif
    return retval;
}

/* set the keyboard backlight. doesnt affect lcd backlight. 0==off,1==medium,2==high */
int setKBBrightness(unsigned int level)
{
    int retval = 0;
    unsigned char usb_data[] = { 2, 1, 0, 0 };

    if (!keyboard_device)
        return G15_ERROR_TRY_AGAIN;

    if(shared_device>0)
        return G15_ERROR_UNSUPPORTED;
    switch(level)
    {
        case 1 :
            usb_data[2] = 0x1;
            break;
        case 2 :
            usb_data[2] = 0x2;
            break;
        default:
            usb_data[2] = 0x0;
    }
    #ifndef LIBUSB_BLOCKS
    pthread_mutex_lock(&libusb_mutex);
    #endif
    retval = libusb_control_transfer(keyboard_device, LIBUSB_REQUEST_TYPE_CLASS + LIBUSB_RECIPIENT_INTERFACE, 9, 0x302, 0, usb_data, 4, 10000);
    #ifndef LIBUSB_BLOCKS
    pthread_mutex_unlock(&libusb_mutex);
    #endif
    return retval;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static unsigned char g15KeyToLogitechKeyCode(int key)
{
   // first 12 G keys produce F1 - F12, thats 0x3a + key
    if (key < 12)
    {
        return 0x3a + key;
    }
   // the other keys produce Key '1' (above letters) + key, thats 0x1e + key
    else
    {
        return 0x1e + key - 12; // sigh, half an hour to find  -12 ....
    }
}
#pragma GCC diagnostic pop

static unsigned int processKeyEvent9Byte(unsigned char *buffer)
{

    unsigned int pressed_keys = 0u;

    g15_log(G15_LOG_INFO, "Keyboard: %x, %x, %x, %x, %x, %x, %x, %x, %x", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7], buffer[8]);

    if (buffer[0] == 0x02)
    {
        if (buffer[1]&0x01)
            pressed_keys |= G15_KEY_G1;

        if (buffer[2]&0x02)
            pressed_keys |= G15_KEY_G2;

        if (buffer[3]&0x04)
            pressed_keys |= G15_KEY_G3;

        if (buffer[4]&0x08)
            pressed_keys |= G15_KEY_G4;

        if (buffer[5]&0x10)
            pressed_keys |= G15_KEY_G5;

        if (buffer[6]&0x20)
            pressed_keys |= G15_KEY_G6;


        if (buffer[2]&0x01)
            pressed_keys |= G15_KEY_G7;

        if (buffer[3]&0x02)
            pressed_keys |= G15_KEY_G8;

        if (buffer[4]&0x04)
            pressed_keys |= G15_KEY_G9;

        if (buffer[5]&0x08)
            pressed_keys |= G15_KEY_G10;

        if (buffer[6]&0x10)
            pressed_keys |= G15_KEY_G11;

        if (buffer[7]&0x20)
            pressed_keys |= G15_KEY_G12;

        if (buffer[1]&0x04)
            pressed_keys |= G15_KEY_G13;

        if (buffer[2]&0x08)
            pressed_keys |= G15_KEY_G14;

        if (buffer[3]&0x10)
            pressed_keys |= G15_KEY_G15;

        if (buffer[4]&0x20)
            pressed_keys |= G15_KEY_G16;

        if (buffer[5]&0x40)
            pressed_keys |= G15_KEY_G17;

        if (buffer[8]&0x40)
            pressed_keys |= G15_KEY_G18;

        if (buffer[6]&0x01)
            pressed_keys |= G15_KEY_M1;
        if (buffer[7]&0x02)
            pressed_keys |= G15_KEY_M2;
        if (buffer[8]&0x04)
            pressed_keys |= G15_KEY_M3;
        if (buffer[7]&0x40)
            pressed_keys |= G15_KEY_MR;

        if (buffer[8]&0x80)
            pressed_keys |= G15_KEY_L1;
        if (buffer[2]&0x80)
            pressed_keys |= G15_KEY_L2;
        if (buffer[3]&0x80)
            pressed_keys |= G15_KEY_L3;
        if (buffer[4]&0x80)
            pressed_keys |= G15_KEY_L4;
        if (buffer[5]&0x80)
            pressed_keys |= G15_KEY_L5;

        if (buffer[1]&0x80)
            pressed_keys |= G15_KEY_LIGHT;

    }
    return pressed_keys;
}

static unsigned int processKeyEvent5Byte(unsigned char *buffer)
{

    unsigned int pressed_keys = 0u;

    g15_log(G15_LOG_INFO, "Keyboard: %x, %x, %x, %x, %x", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);

    if (buffer[0] == 0x02)
    {
        if (buffer[1]&0x01)
            pressed_keys |= G15_KEY_G1;

        if (buffer[1]&0x02)
            pressed_keys |= G15_KEY_G2;

        if (buffer[1]&0x04)
            pressed_keys |= G15_KEY_G3;

        if (buffer[1]&0x08)
            pressed_keys |= G15_KEY_G4;

        if (buffer[1]&0x10)
            pressed_keys |= G15_KEY_G5;

        if (buffer[1]&0x20)
            pressed_keys |= G15_KEY_G6;

        if (buffer[1]&0x40)
            pressed_keys |= G15_KEY_M1;

        if (buffer[1]&0x80)
            pressed_keys |= G15_KEY_M2;

        if (buffer[2]&0x20)
            pressed_keys |= G15_KEY_M3;

        if (buffer[2]&0x40)
            pressed_keys |= G15_KEY_MR;

        if (buffer[2]&0x80)
            pressed_keys |= G15_KEY_L1;

        if (buffer[2]&0x2)
            pressed_keys |= G15_KEY_L2;

        if (buffer[2]&0x4)
            pressed_keys |= G15_KEY_L3;

        if (buffer[2]&0x8)
            pressed_keys |= G15_KEY_L4;

        if (buffer[2]&0x10)
            pressed_keys |= G15_KEY_L5;

        if (buffer[2]&0x1)
            pressed_keys |= G15_KEY_LIGHT;
    }
    return pressed_keys;
}

// the handling is in background, but the invocation of callback functions
//    will be done if this function is called,
//    but at least we dont need to fear too many multithreading issues
static void handle_transfer_events(bool skipCancelled)
{
    int need_handle = FALSE;
    if (key_transfer.transfer != NULL) {
        if (skipCancelled
        || (key_transfer.transfer->status != LIBUSB_TRANSFER_CANCELLED
        && key_transfer.transfer->status != LIBUSB_TRANSFER_NO_DEVICE)) {
            need_handle = TRUE;
        }
    }
    if (lcd_transfer.transfer != NULL) {
        if (skipCancelled
        || (lcd_transfer.transfer->status != LIBUSB_TRANSFER_CANCELLED
        && lcd_transfer.transfer->status != LIBUSB_TRANSFER_NO_DEVICE)) {
            need_handle = TRUE;
        }
    }
    if (need_handle) {
        struct timeval tv;
        memset(&tv, 0, sizeof(struct timeval));
        tv.tv_usec = 10 * 1000;   // 10ms timeout
        int ret = libusb_handle_events_timeout(context, &tv);
        if (ret != LIBUSB_SUCCESS) {
            g15_log(G15_LOG_WARN, "g15 libusb_handle_events_timeout %d %s", ret, libusb_strerror(ret));
        }
    }
}



int getPressedKeys(unsigned int *pressed_keys)
{
    int ret = G15_NO_ERROR;
    if (!keyboard_device) {
        pthread_mutex_lock(&libusb_mutex);
        // try to reattach
        keyboard_device = findAndOpenG15();
        pthread_mutex_unlock(&libusb_mutex);
    }
    if (keyboard_device) {
        if (key_transfer.transfer == NULL) {
            alloc_transfer(&key_transfer, keyboard_device, g15_keys_endpoint,key_transfer_cb);
        }
        if (key_transfer.active == TRANSFER_INACTIVE) {
            ret = libusb_submit_transfer(key_transfer.transfer);
            if (ret != 0) {
		handle_usb_errors("Keyboard submit transfer", ret);
                free_transfer(&key_transfer);
                return ret;
            }
            else {
                key_transfer.active = TRANSFER_ACTIVE;
            }
        }
        handle_transfer_events(FALSE);
        if (key_transfer.transfer->status != LIBUSB_TRANSFER_ERROR &&
            key_transfer.transfer->status != LIBUSB_TRANSFER_STALL &&
            key_transfer.transfer->status != LIBUSB_TRANSFER_OVERFLOW) {    // anything else coud be "normal"
            *pressed_keys = key_transfer.result;      // get all the changes since last call
            key_transfer.result = 0;
            ret = G15_NO_ERROR;
        }
        else {
            g15_log(G15_LOG_WARN, "Unexpected status %d on keyboard read", key_transfer.transfer->status);
            ret = G15_ERROR_READING_USB_DEVICE;
        }
    }
    return ret;
}

void setLibG15Log(struct sG15Worker* _g15Worker)
{
    g15Worker = _g15Worker;
}