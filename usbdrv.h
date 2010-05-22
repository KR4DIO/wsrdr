/*
 * File:   usbdrv.h
 * Author: jim
 *
 * Created on 22 January 2010, 22:54
 */

// V0.1

#ifndef _USBDRV_H
#define	_USBDRV_H

#ifdef	__cplusplus
extern "C" {
#endif

struct usb_device *find_device(int vendor, int product);
void _close_readw();
void _open_readw();
void _init_wread();
void _send_usb_msg(char* bytes);
int _read_usb_msg(char *buffer);

void openUSBDevice();
int readBytesFromUSB(char* buffer, long location);


#ifdef	__cplusplus
}
#endif

#endif	/* _USBDRV_H */

