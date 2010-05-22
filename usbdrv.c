//---------------------------------------------------------------------------------
//! usbdrv.c
//! USB driver interaction
//!
//! V0.1
//!
//! Based on code (C) M. Pendec 2007
//!
//! (C) J. Whurr 2010

/*
    This file is part of the wsrdr programme.

    wsrdr is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    wsrdr is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with wsrdr.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <ctype.h>
#include <usb.h>

#include "config.h"


// Data
struct usb_dev_handle *devh;
char   usbmsg[1000];
char   buf[1000];

#if __DARWIN_UNIX03
    #warning "defines replace library calls - need to change"
    #define usb_get_driver_np(d, z, b, l)           (0)
    #define usb_detach_kernel_driver_np(devh, z)    (0)
#endif


// Functions to use USB port and access device's memory
// - Open device
// - Close device
// - Find device matching vendor and product
// - Initialize write and read
// - Send USB Message
// - Read USB Message


struct usb_device *find_device(int vendor, int product) {
    struct usb_bus *bus;

    for (bus = usb_get_busses(); bus; bus = bus->next) {
        struct usb_device *dev;

        for (dev = bus->devices; dev; dev = dev->next) {
            if (dev->descriptor.idVendor == vendor
				&& dev->descriptor.idProduct == product)
                return dev;
        }
    }
    return NULL;
};

void _close_readw() {
    int ret = usb_release_interface(devh, 0);
    if (ret!=0)
        printf("could not release interface: %d\n", ret);
    ret = usb_close(devh);
    if (ret!=0)
        printf("Error closing interface: %d\n", ret);
};

void _open_readw() {
    struct usb_device *dev;
    int vendor, product, ret;

#if 0	// ** DEBUG
	usb_urb *isourb;
	struct timeval isotv;
	char isobuf[32768];
#endif	// **

    usb_init();
    // usb_set_debug(0);
    usb_find_busses();
    usb_find_devices();

    vendor = 0x1941;
    product = 0x8021;

    dev = find_device(vendor, product);
    assert(dev);
    devh = usb_open(dev);
    assert(devh);
    signal(SIGTERM, _close_readw);

    ret = usb_get_driver_np(devh, 0, buf, sizeof(buf));
    if (ret == 0) {
        /* interface 0 already claimed by driver buf, attempting to detach it */
        ret = usb_detach_kernel_driver_np(devh, 0);
        /* usb_detach_kernel_driver_np returned ret */
    }

    ret = usb_claim_interface(devh, 0);
    if (ret != 0) {
        printf("Could not open usb device, errorcode - %d\n", ret);
        exit(1);
    }

    ret = usb_set_altinterface(devh, 0);
    assert(ret >= 0);
}

void _init_wread() {
    char tbuf[1000];

    int ret = usb_get_descriptor(devh, 1, 0, tbuf, 0x12);
    // usleep(14*1000);
    ret = usb_get_descriptor(devh, 2, 0, tbuf, 9);
    // usleep(10*1000);((c = getopt (argc, argv, "akwosiurthp:zx")) != -1)
    ret = usb_get_descriptor(devh, 2, 0, tbuf, 0x22);
    // usleep(22*1000);
    ret = usb_release_interface(devh, 0);
    if (ret != 0) printf("failed to release interface before set_configuration: %d\n", ret);
    ret = usb_set_configuration(devh, 1);
    ret = usb_claim_interface(devh, 0);
    if (ret != 0) printf("claim after set_configuration failed with error %d\n", ret);
    ret = usb_set_altinterface(devh, 0);
    // usleep(22*1000);
    ret = usb_control_msg(devh, USB_TYPE_CLASS + USB_RECIP_INTERFACE, 0xa, 0, 0, tbuf, 0, 1000);
    // usleep(4*1000);
    ret = usb_get_descriptor(devh, 0x22, 0, tbuf, 0x74);
}

int _read_usb_msg(char *buffer) {
    char tbuf[ReadBufferSize];

    int bytes = usb_interrupt_read(devh, 0x81, tbuf, ReadBufferSize, 1000);
    memcpy(buffer, tbuf, bytes);
    // usleep(82*1000);flushDeviceMemory
    return bytes;
}

void openUSBDevice() {
    // open USB device

    // Test if device has been locked by another client
    // read file ./wws.lock
    // if locked => Error : Device has been locked by another client
    // else lock, go on, unlock
    // done by bash script

    _open_readw();
    _init_wread();

}

void _send_usb_msg(char* bytes ) {
    int ret = usb_control_msg(devh, USB_TYPE_CLASS + USB_RECIP_INTERFACE, 9, 0x200, 0, bytes, 8, 1000);
    if (ret != 8) {
        printf("Error: usb_control_msg read %d characters, expecting 8, aborting...\n", ret);
        exit(1);
    }
    //printf("**DEBUG: usb_control_msg return status = %d\n", ret);
    // usleep(28*1000);
}

int readBytesFromUSB(char* buffer, long location) {

    unsigned char addr1 = (unsigned char) ((location >> 8) & 0xFF);
    unsigned char addr2 = (unsigned char) (location & 0xFF);

    char buffersize = ReadBufferSize & 0xFF;
    char bytes[] = {'\xa1', addr1, addr2, buffersize, '\xa1', addr1, addr2, buffersize};

    // set up to read 32 bytes starting at device memory 'location'
    _send_usb_msg(bytes);

    // read the bytes into the buffer
    return _read_usb_msg(buffer);
}





