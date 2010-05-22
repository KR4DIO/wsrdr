//! cmdline.c
//! read command line arguments and set the corresponding programme flags (struct options)
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
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <ctype.h>
#include <unistd.h>

#include "config.h"
#include "cmdline.h"

unsigned int memoryDumpStart;
unsigned int memoryDumpEnd;
unsigned int startRecordNumber;
unsigned int endRecordNumber;
const char* dateSince;
const char* fieldseparator = ", ";

struct OPTIONS options;

char * recordPrintSpecification = "ahHtTrpwg";
char * cmdFilename;

static int parseMemoryLocations(char*);
static int parseRecordRange(char* string);
static char * validatePrintSpecification(char*);

static int noRecordRange = 0;

void read_arguments(int argc, char **argv) {
    int c;
    int done = 0;

    while ((done == 0) && ((c = getopt(argc, argv, "hHvm:p:r:s:w:F:S:")) != -1)) {	// JW01, added S
        switch (c) {
            case 'm':
                options.dumpMemory = 1;
                options.inputFromFile = 0;
                options.writeMemoryToFile = 0;
                options.printRecords = 0;
                if (optarg != NULL) {
                    if (!parseMemoryLocations(optarg)) {
                        options.showHelp = 1;
                        return;
                    }
                }
                else {
                    memoryDumpStart = 0x0000;
                    memoryDumpEnd = 0x10000;
                }
                done = 1;
                break;

            case 'F':
                options.inputFromFile = 1;
                options.writeMemoryToFile = 0;
                cmdFilename = optarg;
                break;

            case 'w':
                options.dumpMemory = 0;
                options.inputFromFile = 0;
                options.writeMemoryToFile = 1;
                options.printRecords = 0;
                cmdFilename = optarg;
                done = 1;
                break;

            case 'H':
                options.dumpHeader = 1;
                break;

            case 'r':
                options.dumpMemory = 0;
                options.inputFromFile = 0;
                options.writeMemoryToFile = 0;
                options.printRecords = 1;
                startRecordNumber = 0;                   // current record
                //printf("DEBUG: (cmdline.c) %04x\n", optarg);
                if (optarg != NULL) {
                     if (!parseRecordRange(optarg)) {
                        //options.showHelp = 1;
                        //return;
                        noRecordRange = 1;
                    }
                }
                break;

            case 'p':
                options.printRecords = 1;
                options.printSpecification = 1;
                if ((recordPrintSpecification = validatePrintSpecification(optarg)) == NULL) {
                    options.showHelp = 1;
                    return;
                }
                break;

            case 's':
                options.printRecordsSince = 1;
                noRecordRange = 0;
                dateSince = optarg;
                break;

            case 'S':
                    options.fieldseparator = 1;
                    fieldseparator = optarg;
                    break;

            case 'v':
                options.verbose = 1;
                break;

            case '?':
                if (optopt == 'r')
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint (optopt))
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf (stderr,"Unknown option character `\\x%x'.\n", optopt);
                break;


            case 'h':
            default:
                options.showHelp = 1;
        }
    }

    // if no option selected => show help
    int* ovalue = (int*) &options;
    if (*ovalue == 0) {
        options.showHelp = 1;
    }
}

static int parseRecordRange(char* string) {
    // allow the following:
    //  999
    //  :999
    //  999:999
    //  999:n

    if( !isdigit(*string) && *string != ':') {
        return false;
    }

    char * colon = strchr(string, ':');

    // only the end address is given
    if (colon == string) {
        startRecordNumber = 0;
        if (*(string+1) == 'n') {
            options.untilFirstRecord = 1;
            return true;
        }
        sscanf(string+1, "%d", &endRecordNumber);

        if (endRecordNumber > MaxRecords) {
            return false;
        }
		return true;
    }

    sscanf(string, "%d", &startRecordNumber);
    if (startRecordNumber > MaxRecords) {
        return false;
    }

    // only the start addeess is given
    if (colon == NULL) {
        endRecordNumber = 0;
        return true;
    }

    // check for end number being 'n'
    if (*(string+1) == 'n') {
        options.untilFirstRecord = 1;
        return true;
    }

   // both addresses given
    sscanf(++colon, "%d", &endRecordNumber);

    if ((startRecordNumber > endRecordNumber) || (endRecordNumber > MaxRecords)) {
        return false;
    }

    return true;
}

static int parseMemoryLocations(char* string) {
    // allow the following:
    //  0
    //  0x9999
    //  :0x9999
    //  0x9999:0x9999

    if( !isxdigit(*string) && *string != ':') {
        return false;
    }

    char * colon = strchr(string, ':');

    // only the end address is given
    if (colon == string) {
        memoryDumpStart = 0;
        sscanf(string+1, "%x", &memoryDumpEnd);
        if (memoryDumpEnd == 0 || memoryDumpEnd > 0x10000) {
            return false;
        }
		return true;
    }

    sscanf(string, "%x", &memoryDumpStart);

    // only the start addeess is given
    if (colon == NULL) {
        memoryDumpEnd = 0x10000;
        return true;
    }

    // both addresses given
    sscanf(++colon, "%x", &memoryDumpEnd);

    if (memoryDumpStart > memoryDumpEnd) {
        return false;
    }

    return true;
}

static char * validatePrintSpecification(char* spec) {
    int i = 0;
    char* ptr = spec;

    for(i = 0; i < strlen(spec); i++) {
        if (strchr("ahHtTrRpwgdDuUi", *ptr++) == NULL) {
            return NULL;
        }
    }
    return spec;
}
