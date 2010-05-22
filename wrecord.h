/*
 * File:   wrecord.h
 * Author: jim
 *
 * Created on 23 January 2010, 04:19
 */

// V0.11
// Change: JW01 - rreadl parameter location now a long (was int)

#ifndef _WRECORD_H
#define	_WRECORD_H

#ifdef	__cplusplus
extern "C" {
#endif

    struct weatherRecord {
        unsigned int	memPos;
        unsigned int	interval;
        unsigned int	humIn;
        double		tempIn;
        unsigned int	humOut;
        double		tempOut;
        double		press;
        double		windSpeed;
        double		gustSpeed;
        unsigned int	windDir;
        unsigned int	rainCounter;
        unsigned int	errorCode;

        unsigned char	rawdata[16];
    };

    typedef struct weatherRecord* weatherRecordPtr;


////////////////////////////////////////////////////////////////////////////
//
//   R E C O R D    R O U T I N E S

	// read record at given memloc
	weatherRecordPtr rreadl(weatherRecordPtr record, long location);

	// read record at given index
	weatherRecordPtr rread(weatherRecordPtr record, int index);

	// print record as row with ,s
	void rprint(weatherRecordPtr wRec);

	// print a record according to the given spec with the given separator between fields
	void rprints(weatherRecordPtr recptr, const char* recordPrintSpecification, char* separator);
	
	void rprintv(weatherRecordPtr wRec, const char* recordPrintSpecification, char* separator, int headings);

    // hexdump of the given record
    void rhexdump(weatherRecordPtr wRec);

	// get rain counter diff from previous
	int rainMeterDifference(weatherRecordPtr this);


#ifdef	__cplusplus
}
#endif

#endif	/* _WRECORD_H */
