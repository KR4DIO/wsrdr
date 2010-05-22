/**
 *! dfile.c
 *! Provides a file-like interface to the underlying data irrespective of
 *! the actual storage. Layered on top of the chstream.h interface.
 *!
 *! V0.11
 *!
 *! Change: JW01 changed location parameter of dread() to long (was int)
 *!
 *! Copyright (C) J. Whurr 2010
 */

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

#include "chstream.h"

//#define _DEBUG

typedef enum HANDLE {
    NONE = 0,
    DEVICE,
    CFILE
} HANDLE;

HANDLE handle;
FILE* cfile = NULL;


#ifdef _DEBUG
void dump(char* data, int location, int size) {
    for(int i = 0; i < size; i++) {
            if (i % 16 == 0) {
                    printf("\n%04x |", location + i);
            }
            printf(" %02x", data[i] & 0xFF);
    }
    printf("\n");
}
#define debug(d, l, s)	dump(d, l, s)
#else
    #define debug(d, l, s)	;
#endif

//! Open the file that holds the data, either the actual device (:usb:) or a file
//! holding a copy of weatherstation memory.
//!
int dopen(char* filename) {
    if (strcmp(":usb:", filename) == 0) {
        uopen();			//openUSBDevice();
        handle = DEVICE;
        return 1;
    }

    cfile = fopen(filename, "rb");
    handle = CFILE;
    return cfile >= 0;
}

//! Read a block of the file into the specified buffer.
//
int dread(char* buffer, long location, int size) {
    switch(handle) {
        case NONE:
            printf("Error: no device has been opened, use dopen()\n");
            exit(1);

        case DEVICE:
            useek(location);
            int bytes = uread(buffer, size);
            debug(buffer, location, size);
            return bytes;

        case CFILE:
            fseek(cfile, location, SEEK_SET);
            int read = fread(buffer, 1, size, cfile);
            debug(buffer, location, size);
            return read;
    }
    return -1;
}

//! Pass-through to the relevant flush routine. For the physical device this
//! causes the underlying chstream cache to be flushed.
//
void dflush() {
    switch(handle) {
        case NONE:
            printf("Error: no device has been opened, use dopen()\n");
            exit(1);

        case DEVICE:
            uflush();
            break;

        case CFILE:
            fflush(cfile);
    }
}

//! Close the device/file
//
void dclose() {
    if (handle == CFILE) {
        fclose(cfile);
    }
    handle = NONE;
}
