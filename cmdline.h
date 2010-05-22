/*
 * File:   cmdline.h
 * Author: jim
 *
 * Created on 22 January 2010, 16:55
 */

// V0.1

#ifndef _CMDLINE_H
#define	_CMDLINE_H

#ifdef	__cplusplus
extern "C" {
#endif
    #define true    (1==1)
    #define false   (1==0)

    struct OPTIONS {
        unsigned int showHelp               : 1;    // -h
        unsigned int dumpHeader             : 1;    // -H
        unsigned int dumpMemory             : 1;    // -m
        unsigned int printRecords           : 1;    // -r
        unsigned int writeMemoryToFile      : 1;    // -w "filename"
        unsigned int printSpecification     : 1;    // [-r...] -p "spec"
        unsigned int printRecordsSince      : 1;    // [-r...] -s "utc-date"
        unsigned int inputFromFile          : 1;    // -F "filename"
        unsigned int verbose                : 1;    // -v
		unsigned int fieldseparator			: 1;	// -S "field separator string"
        unsigned int untilFirstRecord       : 1;    // internal flag
    };

    void read_arguments(int argc, char **argv);

    extern struct OPTIONS options;
    extern char * cmdFilename;

    extern char * recordPrintSpecification;
    extern unsigned int memoryDumpStart;
    extern unsigned int memoryDumpEnd;
    extern unsigned int startRecordNumber;
    extern unsigned int endRecordNumber;
    extern const char* dateSince;
	extern const char* fieldseparator;

#ifdef	__cplusplus
}
#endif

#endif	/* _CMDLINE_H */

