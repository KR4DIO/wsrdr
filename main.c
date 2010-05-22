//! main.c
//! wsrdr main module: reads command line, opens device (or file)
//! and executes commands
//!
//! wsrdr V0.2
//! change to remove reading records before start when no date is required.
//!
//! This programme builds upon ideas and (some) code from the wwsr project
//! which was originally developed by:
//!
//!     Michael Pendec      2007
//!     Svend Skafte        2008
//!     Quentin Decembry    2009
//!
//! Copyright (C) J. Whurr 2010
//!
//! V0.11
//!
//! Change JW01 Changed code in list record so that it  doesn't read every
//!             record up to the start of the range when the date is not
//!             going to be displayed.

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

#include <time.h>

#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "dfile.h"
#include "header.h"
#include "wrecord.h"
#include "cmdline.h"

static void dump_options();
static void printHelp();

//! print a hexdump of memory on stdout. (default width is 16)
//
void dumpmemory(int start, int end, int width) {
    char data[end - start];

    dread(data, start, (end - start));
	for(int i = start; i < end; i++) {
		if (i % width == 0) {
			printf("\n%04x |", start + i);
		}
		printf(" %02x", data[i] & 0xFF);
	}
	printf("\n");
}

//! Copy the weather-station device memory to a file.
//
void copymem(char* filename) {
    int size = getLocationOfCurrent() + RecordSize;
    char buffer[size];
    FILE* opfile = fopen(filename, "wb");
    int read = dread(buffer, 0, size);
    fwrite(buffer, read, 1, opfile);
    fclose(opfile);
}


//! Print the header records (that are programmed) to stdout.
//
void printHeader() {
    printf("List header...\n");
    listHeader();       // see header.c
}

//! Convert a string date (UTCformat) to a time_t
//
time_t cvtStr2Time_t(char* date) {
	// for some reason these have to be static or it f's up!!
	static const char* fmt = "%Y-%m-%d %H:%M";
	static struct tm tmp;

	// convert to tm first using library function strptime()
	strptime(date, fmt, &tmp);
        tmp.tm_isdst = -1;            // unsure about daylight saving!

	// return time_t using library function mktime()
	time_t t = mktime(&tmp);
	//printf("DEBUG: date(%s) -> %d-%02d-%02d %02d:%02d -> %d\n", date, tmp.tm_year, tmp.tm_mon, tmp.tm_mday, tmp.tm_hour, tmp.tm_min, t);
	return t;
}


//! Convert a time-t time to a string UTC representation
//
void cvtTime2Str(char* string, int maxlen,  time_t* timeptr) {
    struct tm* tmtime = localtime(timeptr);
    strftime(string, maxlen, "%Y-%m-%d %H:%M", tmtime);
}

#define true (1==1)
#define false (1==0)

//!**JW01**
//! Introduced to allow listRecords to determine if it needs to track the date
//! so as to avoid (potentially) reading lots of records unecessarily.
//!
//! Retuins true if the date is going to be printed as part of the output.
//
int daterequired() {
    char* ptr = recordPrintSpecification;
    while(*ptr != '\0') {
        if (*ptr == 'u' || *ptr == 'U')
            return true;
        ptr++;
    }
    return false;
}

//! List a range of records.
//! Checks that the end is not greater than the number of records stored
//
void listRecords(int start, int end) {
    struct weatherRecord record;

    //printf("DEBUG: -r %d:%d\n", start, end);

    if (end < start) {
        end = start;
    }

    int numrecs = getRecordsStored();

    //printf("DEBUG: (main.c) there are %d records stored\n", numrecs);

    if (end >= numrecs) {
        printf("invalid end record number %d\n", end);
        return;
    }

    // set up for storing the time of readings
    // convert date/time given to time_t

    // get current date and time using getDateTime() (see header.h)
    char* devtimestr = (char*) getDateTime();
    //printf("DEBUG: device date = %s\n", devtimestr);
    time_t devtime = cvtStr2Time_t(devtimestr);

    // prepare, get a record pointer and a record counter and temp time
    weatherRecordPtr p = NULL;
    int recordidx = 0;
    time_t tmptime = devtime;

    if (daterequired()) {
    // read up to the starting record, adjusting time
        while(recordidx < start) {
            // calculate time of the first stored record using rread() (see wrecord.h)
            p = rread(&record, recordidx++);
            tmptime -= (record.interval * 60);
        }
    }
    else {
        recordidx = start;
    }

    // now recordidx is pointing at the first record to be listed
    // and tmptime is holding the time of that record

    // get date string storage
    char datestr[17];

    // see if headings are to be printed, for options see cmdline.h
    int headings = (options.verbose == 1) ? 1 : 0;

    // loop through the records to be listed
    while(recordidx <= end) {
        // read the record (see wrecord.h)
        p = rread(&record, recordidx++);
        // convert the date to a string
        cvtTime2Str(datestr, 17, &tmptime);
        // tell getDateTime() to use our date/time (usedate is external, see header.h)
        usedate = datestr;

        // print the record using functions specified in wrecord.h
        if (options.verbose == 0) {
            rprints(&record, recordPrintSpecification, fieldseparator);
        }
        else {
            rprintv(&record, recordPrintSpecification, fieldseparator, headings);
        }

        // calculate date/time of next saved record
        tmptime -= (record.interval * 60);

        // reset getDateTime() to use device time
        usedate = NULL;
        // no more headings for this list
        headings = 0;
    }

    /*
    int headings = (options.verbose == 1) ? 1 : 0;
    for(int i = end; i >= 0 && i >= start; i--) {
        rread(&record, i);
        if (options.verbose == 0) {
            rprints(&record, recordPrintSpecification, fieldseparator);
        }
        else {
            rprintv(&record, recordPrintSpecification, fieldseparator, headings);
        }
        headings = 0;
    }
    */
}


//! List a range of SAVED records.
//! Checks that the time specified is not later than device time
//
void listRecordsSince(const char* since) {
    struct weatherRecord record;
    char datestr[17];

    // convert date/time given to time_t
    time_t since_t = cvtStr2Time_t(since);

    // get current date and time - getDateTime() found in header.h
    char* devtimestr = (char*) getDateTime();

    //printf("DEBUG: device date = %s\n", devtimestr);
    
    time_t devtime = cvtStr2Time_t(devtimestr);

    // check for since date in the future
    if (since_t > devtime) {
        printf("Date %s is in the future, date now is %s\n", since, getDateTime());
        exit(1);
    }

    // maximum number of records, getRecordsStored() in header.h
    int guard = getRecordsStored();

    //printf("DEBUG: (main.c) there are %d records stored\n", guard);

    int headings = (options.verbose == 1) ? 1 : 0;

    // Record 0 is the current reading, it is continually overwritten until the
    // polling period is reached, at which time a new current record is started
    // with a 0 second delay.
    int recordidx = 0;

    // skip the 0 record as it is the current value and should be read using
    // -r 0
    // (rread() found in wrecord.h)
    weatherRecordPtr p = rread(&record, recordidx++);

    // calculate time of the first saved record
    time_t tmptime = devtime - (record.interval * 60);

    // starting with the first saved record (recordidx == 1)
    // while the record time is greater than the time since (and there are records!)
    // output the result
    while((tmptime > since_t) && (recordidx < guard)) {
        // read the record
        p = rread(&record, recordidx++);
        // calculate its date/time
        tmptime -= (record.interval * 60);
        // if it is later than since then print it
        if (tmptime > since_t) {
            cvtTime2Str(datestr, 17, &tmptime);
            // tell getDateTime() to use our date/time (usedate is external, see header.h)
            usedate = datestr;
            if (options.verbose == 0) {
                rprints(&record, recordPrintSpecification, fieldseparator);
            }
            else {
                rprintv(&record, recordPrintSpecification, fieldseparator, headings);
            }
            // reset getDateTime() to use device time
            usedate = NULL;
            // no more headings for this list
            headings = 0;
        }
    }
}

//! Converts a tm structured time to a seconds since type time.
//
void cvtTime_t2Tm(struct tm* tm, const time_t* tv) {
    /* convert it to a struct tm */
    struct tm* rettm;
    rettm = localtime_r(tv, tm);
    if(rettm == NULL) {
        printf("Bad return value from localtime_r\n");
        exit(1);
    }
}

#if 0 // debug code
void print_t(time_t* t) {
    char buffer[32];
    struct tm tm1;

    cvtTime_t2Tm(&tm1, t);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &tm1);
    printf("time_t = %s\n", buffer);

}
#endif


//! Set up device and execute command(s).
//
int main(int argc, char** argv) {
    // read in program arguments
    read_arguments(argc, argv);

#ifdef _DEBUG
    // DEBUG: list the options
    dump_options();
#endif // _DEBUG

    // help over-rides all. For options variable see cmdline.h
    if (options.showHelp) {
        printHelp();
        exit(0);
    }

    // open the device or its imposter (file), see dfile.h
    if (options.inputFromFile == 1) {
        dopen(cmdFilename);
    }
    else {
        dopen(":usb:");
    }

    // dump header can be executed with other commands
    if (options.dumpHeader == 1) {
        printHeader();
    }

    // now dispatch for processing
    if (options.dumpMemory == 1) {
        // hexdump device memory
        dumpmemory(memoryDumpStart, memoryDumpEnd, DumpWidth);
    }
    else if (options.writeMemoryToFile == 1) {
        // copy device memory to a file
        copymem(cmdFilename);
    }
    else if (options.printRecordsSince == 1) {
        // list records since given date & time
        listRecordsSince(dateSince);
    }
    else if (options.printRecords == 1) {
        // list specified records (0 = current, n = oldest)
        if (options.untilFirstRecord == 1) {
            endRecordNumber = getRecordsStored();
        }
        listRecords(startRecordNumber, endRecordNumber);
    }

    dclose();
}


static void printHelp() {
    printf("\nWeather Station Reader v0.11\n");
    printf("Reads and reports data from the WH1081 Fine Offset usb weather station\n");
    printf("(C) 2010 Jim Whurr\n\n");
    printf("[Based on the original wwsr utility V0.1, (C) Michael Pendec 2007, and \n");
    printf("ideas from V0.3 (C) Svend Skafte - 2008, (C) Quentin Decembry - 2008]\n\n");
    printf("options:\n");
    printf(" -h             help information\n");
    printf(" -H             list header fields\n");
    printf(" -F filename    read data from the specified file as if it were the device\n");
    printf(" -w filename    write device memory to the specified file\n");
    printf(" -v             verbose, causes headings to be listed\n");
    printf(" -m start:end   dump device memory from start to end (specified in hex)\n");
    printf(" -r start:end   hex dump of records (0 = current, 1 = last saved, etc)\n");
    printf("\nsub-options of -r\n");
    printf("\t-s \"date\"  list records (r > 0) saved since the specified utc formatted date\n");
    printf("\t-S \"string\" use the specified string as a separator between fields\n");
    printf("\t-p \"spec\"  Print using the specification string, see below\n");
    printf("\t\ta.... the device memory address (in hex)\n");
    printf("\t\th.... humidity outside\n");
    printf("\t\tH.... humidity inside\n");
    printf("\t\tt.... temperature outside\n");
    printf("\t\tT.... temperature inside\n");
    printf("\t\tr.... rain meter reading\n");
    printf("\t\tR.... rain meter difference from previous\n");
    printf("\t\tp.... pressure\n");
    printf("\t\tw.... wind speed\n");
    printf("\t\tg.... gust speed\n");
    printf("\t\td.... wind direction\n");
    printf("\t\ti.... record interval (time since previous save in mins)\n");
    printf("\t\tu.... date/time of the data as utc\n");
    printf("\t\tU.... as u but with the value enclosed in ''s\n");
    printf("\n\nfor example:\n");
    printf("\t$ ./wsrdr -r 1:20 -p \"uhtpwd\"\n\n");
}

#ifdef _DEBUG
static void dump_options() {
    printf("options.dumpMemory           = %d\n", options.dumpMemory);
    printf("options.dumpHeader           = %d\n", options.dumpHeader);
    printf("options.printRecords         = %d\n", options.printRecords);
    printf("options.printSpecification   = %d\n", options.printSpecification);
    printf("options.printRecordsSince    = %d\n", options.printRecordsSince);
    printf("options.untilFirstRecord     = %d\n", options.untilFirstRecord);
    printf("options.inputFromFile        = %d\n", options.inputFromFile);
    printf("options.writeMemoryToFile    = %d\n", options.writeMemoryToFile);
    printf("options.showHelp             = %d\n", options.showHelp);
    printf("options.verbose              = %d\n", options.verbose);

    printf("\nmemory dump %04x:%04x\n", memoryDumpStart, memoryDumpEnd);
    printf("record print range %d:%d\n", startRecordNumber, endRecordNumber);
    printf("print specification %s\n", recordPrintSpecification);
    if (options.printRecordsSince == 1) {
        printf("Date to get records from %s\n", dateSince);
    }
}

#endif // _DEBUG

