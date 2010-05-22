/*
 *  header.h
 *
 *
 *  Created by jim on 10/02/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

//! V0.1

#ifndef _HEADER_H
#define _HEADER_H

void listHeader();

unsigned int getUnsignedInt(char * ptr);
int getSignedInt(char * ptr);

void* getValueOfField(char* fieldname);
unsigned int getLocationOfCurrent();
unsigned int getRecordsStored();
unsigned int getInterval();
const char* getDateTime();

// put a pointer to a date string here to use instead of system date (normall NULL)
extern char * usedate;

#endif
