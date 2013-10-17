/*
 *! wrecord.c
 *!
 *! Provides a 'record' interface to the weather station. Each record represents a
 *! set of data readings. Records are numbered with 0 being current up to the oldest
 *! stored record (getRecordsStored() in the header).
 *!
 *! Each record is held in raw form (16 bytes) and as a number of fields (see
 *! struct weatherRecord in wrecord.h)
 *!
 *! At the moment the assumption is that the device holds records in a circular buffer.
 *!
 *! V0.11
 *!
 *! Change JW01 Added brackets to address calculation in dataaddress. Also
 *!             chnaged location parameter in rreal() to long (was int)
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

#include "config.h"
#include "header.h"
#include "wrecord.h"
#include "dfile.h"

#define todouble(v)	((double) v / 10)

const char * directions[16] = { "N", "NNE", "NE", "NEE", "E", "SEE", "SE", "SSE", "S", "SSW", "SW", "SWW", "W", "NWW", "NW", "NNW" };


//! Turn a record index into a device physical memory location. Checks for
//! invalid index.
//
int dataaddress(int index) {
    long memoryAddress = getLocationOfCurrent();
    long offset = index * RecordSize;

    // check valid index
    if (index >= getRecordsStored()) {
        return -1;
    }

    if (memoryAddress - offset >= BaseAddress) {
        return (memoryAddress - offset);
    }

    // need to wrap backwards
    //
    // + 0x1000
    // .
    // ~                       <--+ 0x10000 - (offset - memoryAddress - 0x0100)
    // .                          |
    // + memoryAddress            |
    // |                          |
    // |                          |
    // ~                          |
    // + 0x0100                +  |
    // .                       |--+
    // .                       |
    // + memorAddress - offset +

    memoryAddress = DeviceMemorySize - (offset - (memoryAddress - 0x0100));
    //**JW V0.11 Changed this forgot to put brackets around (m - 0x100)!!
    // I had changed the address variables to long as I initially thought that
    // might be the issue. I've left them as long as it doesn't harm.

    return memoryAddress;

}

//! Read a record at the given device memory location
//
weatherRecordPtr rreadl(weatherRecordPtr record, long location) {
    char buffer[RecordSize];

    //printf("DEBUG: rreadl(record, %04x)\n", location);

    // get the raw data
    dread(buffer, location, RecordSize);
    memcpy(&record->rawdata, buffer, RecordSize);

    // load up logical record
    record->memPos	= location;
    record->interval	= record->rawdata[0] & 0xFF;
    record->humIn	= record->rawdata[1] & 0xFF;
    record->tempIn	= todouble(getSignedInt((char *)(record->rawdata + 0x02)));
    record->humOut	= record->rawdata[4] & 0xFF;
    record->tempOut	= todouble(getSignedInt((char *)(record->rawdata + 0x05)));
    record->press	= todouble(getUnsignedInt((char *)(record->rawdata + 0x07)));
    record->windSpeed	= todouble((record->rawdata[9] & 0xFF));	// metres/sec
    record->gustSpeed	= todouble(getUnsignedInt((char *)(record->rawdata + 0x0A)));
    record->windDir	= record->rawdata[12] & 0xFF;
    record->rainCounter	= todouble(getUnsignedInt((char *)(record->rawdata + 0x0D)));
    record->errorCode	= record->rawdata[15] & 0xFF;

    return record;
}

//! Read a record at the given index, checks for invalid index (-1 or too big
//! which is done in dataaddress()).
//
weatherRecordPtr rread(weatherRecordPtr record, int index) {
    long location = dataaddress(index);
    //printf("DEBUG: rread() index=%d, address=%04x, current = %04x\n", index, location, getLocationOfCurrent());

    if (location == -1) {
        printf("ERROR: invalid location for record index %d in rread, aborting...\n", index);
        exit(0);
    }

    return rreadl(record, location);
}

//! Print a record using field specifier - see help for details.
//
void rprints(weatherRecordPtr recptr, const char* recordPrintSpecification, char* separator) {
    const char * sp = recordPrintSpecification;

    if (sp == NULL) {
        printf("ERROR: null print specification\n");
        return;
    }

    while(*sp != '\0') {
        switch(*sp++) {
            case 'a':   printf("%04x", recptr->memPos);             break;
            case 'h':   printf("%d",   recptr->humOut);             break;
            case 'H':   printf("%d",   recptr->humIn);              break;
            case 't':   printf("%.1f", recptr->tempOut);            break;
            case 'T':   printf("%.1f", recptr->tempIn);             break;
            case 'r':   printf("%d",   recptr->rainCounter);        break;
            case 'R':   printf("%d", rainMeterDifference(recptr));  break;
            case 'p':   printf("%.1f", recptr->press);              break;
            case 'w':   printf("%.1f", recptr->windSpeed);          break;
            case 'g':   printf("%.1f", recptr->gustSpeed);          break;
            case 'd':   printf("'%s'", directions[recptr->windDir]);  break;
            case 'D':   printf("%d", recptr->windDir);              break;
            case 'i':   printf("%d",   recptr->interval);           break;
            case 'u':   printf("%s",   getDateTime());              break;
            case 'U':   printf("'%s'", getDateTime());              break;
            case 'e':	printf("%02x", recptr->errorCode);          break;
        }
        if (*sp != '\0') {
            printf("%s", separator);
        }
    }
    printf("\n");
}

//! Print a given record as a formatted row. Column headings are optional.
//
void rprintv(weatherRecordPtr wRec, const char* recordPrintSpecification, char* separator, int headings) {
    const char * sp = recordPrintSpecification;

    if (sp == NULL) {
        printf("ERROR: null print specification\n");
        return;
    }

    if (headings > 0) {
        // print headings appropriate to the spec
        while(*sp != '\0') {
            switch(*sp++) {
                case 'a':   printf("loc.");                break;
                case 'i':   printf("int");                 break;
                case 'H':   printf("hI.");                 break;
                case 'h':   printf("hO.");                 break;
                case 'T':   printf("iTemp");               break;
                case 't':   printf("oTemp");               break;
                case 'p':   printf("Pres..");              break;
                case 'w':   printf("wSpd.");               break;
                case 'g':   printf("gSpd.");               break;
                case 'd':   printf("dir");                 break;
                case 'D':   printf("dir");                 break;
                case 'r':   printf("rn.");                 break;
                case 'e':   printf("err");                 break;
                case 'U':
                case 'u':   printf("UTC date        ");    break;
                case 'R':   printf("rdif");                break;
            }
            if (*sp != '\0') {
                printf("%s", separator);
            }
        }
        printf("\n");
    }

    sp = recordPrintSpecification;
    while(*sp != '\0') {
        // print fields according to spec
        switch(*sp++) {
            case 'a':   printf("%04x", (unsigned int)wRec->memPos); break;
            case 'i':   printf("%3d",  wRec->interval);             break;
            case 'H':   printf("%3d",  wRec->humIn);                break;
            case 'h':   printf("%3d",  wRec->humOut);               break;
            case 'T':   printf("%5.1f",wRec->tempIn);               break;
            case 't':   printf("%5.1f",wRec->tempOut);              break;
            case 'p':   printf("%6.1f",wRec->press);                break;
            case 'w':   printf("%5.1f",wRec->windSpeed);            break;
            case 'g':   printf("%5.1f",wRec->gustSpeed);            break;
            case 'd':   printf("%3d",  wRec->windDir);              break;
            case 'D':   printf("%d",   wRec->windDir);              break;
            case 'r':   printf("%3d",  wRec->rainCounter);          break;
            case 'e':   printf("%3d",  wRec->errorCode);            break;
            case 'u':   printf("%15s", getDateTime());              break;
            case 'U':   printf("'%15s'",getDateTime());             break;

            case 'R':   printf("%4d%s", rainMeterDifference(wRec), separator);    break;       // total width = 59
        }
        if (*sp != '\0') {
            printf("%s", separator);
        }
    }
    printf("\n");
}


//! print a given weather record
void rprint(weatherRecordPtr wRec) {
    static const char* specification = "aiHhTtpwgdre";
    rprints(wRec, specification, ", ");
}

//! Hex dump of a record.
//
void rhexdump(weatherRecordPtr wRec) {
    printf("%04x |", (unsigned int)wRec->memPos);
    for(int i = 0; i < RecordSize; i++) {
        printf(" %02x", wRec->rawdata[i]);
    }
    printf("\n");
}

//! Calculate the change in the rain meter from one data reading to the next.
//! (Used when R is present in the print spec)
//
int rainMeterDifference(weatherRecordPtr this) {
    struct weatherRecord previous;

    // get address of previous record
    int address = this->memPos - RecordSize;
    if (address < 0x0100) {
        address = 0x1000 - RecordSize;
    }

    // get the record
    rreadl(&previous, address);

    return (this->rainCounter - previous.rainCounter);
}

