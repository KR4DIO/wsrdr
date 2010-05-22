/*
 *!  header.c
 *!  Read, translate and display device memory.
 *!
 *! V0.1
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

#include "config.h"
#include "header.h"
#include "dfile.h"


//////////////////////////////////////////////////////////////////////////////////////////////
//
//   M A C R O S
//

#define	L_INTERVAL	(0x010)				// storage interval
#define	L_RECORDS	(0x01B)				// number of records dstored on device
#define L_CURRENT	(0x01E)				// memory address of current record
#define	L_DATETIME	(0x02B)				// address of device date & time


//////////////////////////////////////////////////////////////////////////////////////////////
//
//   D A T A    D E F I N I T I O N
//

enum FIELDTYPES {
	CHAR,
	INT,
	UINT,
	ULINT,
	DATE
};

struct HEADERFIELD {
	char*	name;
	char*	description;
	int		location;
	int		type;
};

struct HEADERFIELD fields[] = {
//	 name			description								    loc				type
	{"interval",	"storage interval................%5d\n",    L_INTERVAL,     CHAR},
	{"records",	"number of records stored........%5d\n",    L_RECORDS,      UINT},
	{"current",	"current record location.... ....%04x\n",   L_CURRENT,      UINT},
	{"rpressure",	"current relative pressure.......%5d\n",    0x020,          UINT},
	{"apressure",	"current absolute pressure.......%5d\n",    0x022,          UINT},
	{"datetime",	"current date & time.............%s\n",     L_DATETIME,     DATE},
	{"ihumiditymax","inside maximum humidity.........%5d\n",    0x062,          CHAR},
	{"ihumiditymin","inside minimum humidity.........%5d\n",    0x063,          CHAR},
	{"ohumiditymax","outside maximum humidity........%5d\n",    0x064,          CHAR},
	{"ohumiditymin","outside minimum humidity........%5d\n",    0x065,          CHAR},
	{"itempmax",    "inside maximum temperature......%5d\n",    0x066,          INT},
	{"itempmin",    "inside minimum temperature......%5d\n",    0x068,          INT},
	{"otempmax",    "outside maximum temperature.....%5d\n",    0x06A,          INT},
	{"otempmin",    "outside minimum temperature.....%5d\n",    0x06C,          INT},
	{"windchillmax","maximum wind chill..............%5d\n",    0x06E,          INT},
	{"windchillmin","minimum wind chill..............%5d\n",    0x070,          INT},
	{"dewpointmax", "maximum dew point...............%5d\n",    0x072,          INT},
	{"dewpointmin", "minimum dew point...............%5d\n",    0x074,          INT},
	{"abspressmax", "maximum absolute pressure.......%5d\n",    0x076,          UINT},
	{"abspressmin", "minimum absolute pressure.......%5d\n",    0x078,          UINT},
	{"relpressmax", "maximum relative pressure.......%5d\n",    0x07A,          UINT},
	{"relpressmin", "minimum relative pressure.......%5d\n",    0x07C,          UINT},
	{"windspeedmax","maximum wind speed..............%5d\n",    0x07E,          UINT},
	{"gustspeedmax","maximum gust speed..............%5d\n",    0x080,          UINT},
	{"rainhoumax",  "maximum rain in an hour.........%5d\n",    0x082,          UINT},
	{"raindaymax",  "maximum rain in a day...........%5d\n",    0x084,          UINT},
	{"rainweekmax", "maximum rain in a week..........%5d\n",    0x086,          UINT},
	{"rainmonthmax","maximum rain in a month.........%5d\n",    0x088,          UINT},
	{"raintotalmax","maximum total rain..............%5d\n",    0x08A,          ULINT}, // this is a long
	{"ihummaxdate", "date inside maximum humidity....%s\n",     0x08D,          DATE},
	{"ihummindate", "date inside minimum humidity....%s\n",     0x092,          DATE},
	{"ohummaxdate", "date outside maximum humidity...%s\n",     0x097,          DATE},
	{"ohummindate", "date outside minimum humidity...%s\n",     0x09C,          DATE},
	{"itempmaxdate","date inside maximum temp........%s\n",     0x0A1,          DATE},
	{"itempmindate","date inside minimum temp........%s\n",     0x0A6,          DATE},
	{"otempmaxdate","date outside maximum temp.......%s\n",     0x0AB,          DATE},
	{"otempmindate","date outside minimum temp.......%s\n",     0x0B0,          DATE},
	{"wcmaxdate",   "date maximum wind chill.........%s\n",     0x0B5,          DATE},
	{"wcmindate",   "date minimum wind chill.........%s\n",     0x0BA,          DATE},
	{"dpmaxdate",   "date maximum dew point..........%s\n",     0x0BF,          DATE},
	{"dpmindate",   "date minimum dew point..........%s\n",     0x0C4,          DATE},
	{"apmaxdate",   "date maximum absolute pressure..%s\n",     0x0C9,          DATE},
	{"apmindate",   "date minimum absolute pressure..%s\n",     0x0CE,          DATE},
	{"rpmaxdate",   "date maximum relative pressure..%s\n",     0x0D3,          DATE},
	{"rpmindate",   "date minimum relative pressure..%s\n",     0x0D8,          DATE},
	{"wsmaxdate",   "date maximum wind speed.........%s\n",     0x0DD,          DATE},
	{"gsmindate",   "date minimum gust speed.........%s\n",     0x0E2,          DATE},
	{"rhmaxdate",   "date maximum rain in an hour....%s\n",     0x0E7,          DATE},
	{"rdmaxdate",   "date maximum rain in a day......%s\n",     0x0EC,          DATE},
	{"rwmaxdate",   "date maximum rain in a week.....%s\n",     0x0F1,          DATE},
	{"rmmaxdate",   "date maximum rain in a month....%s\n",     0x0F6,          DATE},
	{"rtmaxdate",   "date maximum rain total.........%s\n",     0x0FB,          DATE},

};

// special location for storing date to use other than the system date
char * usedate = NULL;


//////////////////////////////////////////////////////////////////////////////////////////////
//
//   U T I L I T Y    R O U T I N E S
//


// return device 2 byte integer as unsigned int
unsigned int getUnsignedInt(char * ptr) {
    return (unsigned char)ptr[0] + (((unsigned int)(unsigned char)ptr[1]) << 8);
}

// return device 2 byte integer as signed int
signed int getSignedInt(char * ptr) {
    unsigned int sign = ptr[1] & 0x80;
    unsigned int val = (unsigned char)ptr[0] + (((unsigned int)(unsigned char)ptr[1]) << 8);
    return (sign) ? -(val^0x8000) : val;
}

unsigned long getUnsignedLong(char* ptr) {
    long val = (unsigned char) ptr[0] + ((unsigned char) ptr[1] * 256) + ((unsigned char) ptr[2] * 256 * 256);
    return val;
}

static char * strDate(char * ptrDate) {
    static char stringDate[17];

    sprintf(stringDate, "20%02x-%02x-%02x %02x:%02x", ptrDate[0], ptrDate[1], ptrDate[2], ptrDate[3], ptrDate[4]);
    return stringDate;
}

static char * strTime(char * ptrTime) {
    static char stringTime[6];

    sprintf(stringTime, "%02x:%02x", ptrTime[0], ptrTime[1]);
    return stringTime;
}



//////////////////////////////////////////////////////////////////////////////////////////////
//
//   F I E L D   L E V E L    R O U T I N E S
//


void* getFieldValue(int location, int type) {
    char buffer[8];
    long retval = 0;
    switch(type) {

        case INT:
            dread(buffer, location, 2);
            retval = (long) getSignedInt(buffer);
            break;

        case UINT:
            dread(buffer, location, 2);
            retval = (long) getUnsignedInt(buffer);
            break;

        case ULINT:
            dread(buffer, location, 3);
            retval = getUnsignedLong(buffer);
            break;

        case DATE:
            dread(buffer, location, 5);
            //printf("DEBUG: getFieldValue DATE[%04x] - %d %d %d %d %d\n", location, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);
            retval = (long) strDate(buffer);
            break;

        case CHAR:
        default:
            dread(buffer, location, 1);
            retval = (long) buffer[0];
            break;
    }
    return (void*) retval;
}


struct HEADERFIELD* getFieldData(char* fieldname) {
    int rows = sizeof(fields) / sizeof(struct HEADERFIELD);
    for(int i = 0; i < rows; i++) {
        if (strcmp(fields[i].name, fieldname) == 0) {
            return &fields[i];
        }
    }
    return NULL;
}


void* getValueOfField(char* fieldname) {
    struct HEADERFIELD* data = getFieldData(fieldname);
    if (data != NULL) {
        return getFieldValue(data->location, data->type);
    }
    return (void*) -1;
}


unsigned int getLocationOfCurrent() {
    char buffer[2];

    dread(buffer, L_CURRENT, 2);
    return getUnsignedInt(buffer);
}


unsigned int getRecordsStored() {
    char buffer[2];

    dread(buffer, L_RECORDS, 2);
    return getUnsignedInt(buffer);
}


unsigned int getInterval() {
    char buffer[2];

    dread(buffer, L_INTERVAL, 2);
    return getUnsignedInt(buffer);
}


const char* getDateTime() {
    if (usedate == NULL)
	return (char*) ((long) getValueOfField("datetime"));
    else
        return usedate;
}



//////////////////////////////////////////////////////////////////////////////////////////////
//
//   P R I N T    R O U T I N E
//

void listHeader() {
    int rows = sizeof(fields) / sizeof(struct HEADERFIELD);
    //int rows = 6;
    for(int i = 0; i < rows; i++) {
        //printf("DEBUG: header field[%d].name = %s, location = %04x, type = %d\n", i, fields[i].name, fields[i].location, fields[i].type);
        printf(fields[i].description, getFieldValue(fields[i].location, fields[i].type));
    }
}
