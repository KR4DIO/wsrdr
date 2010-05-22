//!
//! chstream
//! Presents a read-only character stream interface for the weather station.
//!
//! Data is read in 'blocks', the size of which is controlled by the ReadBufferSize entry
//! in config.h. As accessing the device is quite expensive, the interface creates a simple
//! cache of data read. The uflush() and uFoce() methods can be used to make reads physical,
//! uflush() affecting the whole cache and uforce() being used on parts of it.
//!
//! V0.1
//!
//! Copyright (C) J. Whurr 2010
//!

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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "chstream.h"
#include "usbdrv.h"


static char cache[DeviceMemorySize];
static char validflag[DeviceMemorySize / ReadBufferSize];

static int lastreadaddress = 0xFFFF;
static int devaddress = 0;
static char streambuffer[ReadBufferSize];
static int error = 0;

//! Opens the usb device and sets up the internal cache
//
void uopen() {
    openUSBDevice();
    uflush();
}

//! Closes the usb device
//
void uclose() {
    _close_readw();
}

//! Returns the internal error code (0 = no error)
//
int uerror() {
    return error;
}

//! Sets the stream so that location is the next address read
//
void useek(int location) {
    devaddress = location;
    error = 0;
}

//! Flushes the cache - all reads are physical for the first reference
//
void uflush() {
    for(int i = 0; i < DeviceMemorySize / ReadBufferSize; i++) {
        validflag[i] = false;
    }
}

//! Does a useek and forces a physical read of the location
//
void uforce(int location) {
    useek(location);
    validflag[ReadAddress(location)] = false;
}

//! Seeks to the start of device memory (equivalent tto useek(0))
//
void urewind() {
    useek(0);
}

//! Returns the next byte of data from the stream
//
char ugetc() {
    error = 0;

    //printf("DEBUG: ugetch @ %04x\n", devaddress);

    // wasn't in stream buffer so work out what we need
    int bufferoffset = devaddress % ReadBufferSize;
    lastreadaddress = devaddress - bufferoffset;

    //printf("DEBUG: bufferoffset = %d, lastreadaddrss = %04x\n", bufferoffset, lastreadaddress);
    //printf("DEBUG: sizeof(validflag) = %d\n", sizeof(validflag));

    // see if its in the cache of device memory
    if (validflag[lastreadaddress / ReadBufferSize] == true) {
        //printf("DEBUG: address %04x in cache\n", devaddress);
            return (char) cache[devaddress++];
    }

    //printf("DEBUG: address %04x not in cache, reading buffer @ %04x\n", devaddress, lastreadaddress);

    // isn't in cache so we have to get it
    int bytesread = readBytesFromUSB(streambuffer, lastreadaddress);
    if (bytesread < bufferoffset) {
        error = EOF;
        return -1;
    }

    // copy it to cache
    memcpy(cache + lastreadaddress, streambuffer, ReadBufferSize);
    validflag[lastreadaddress / ReadBufferSize] = true;

    // update stream location and return the data
    return cache[devaddress++];
}

//! Reads the given number of bytes into the specified memory buffer
//
int uread(char* buffer, int size) {
    //printf("DEBUG: uread(buffer, %d) @ %04x\n", size, devaddress);
    int bytesread = 0;
    for(int i = 0; i < size; i++) {
        buffer[i] = ugetc();
        if (error == EOF) {
            printf("ERROR: ugetc() reported EOF at location %04x\n", devaddress);
            return bytesread;
        }
        bytesread++;
    }
    return bytesread;
}

