//! Device specific configuration
//! V0.1
#ifndef _CONFIG_H
#define _CONFIG_H

#define	BaseAddress	        0x00100         // address of weather station record storage
#define DeviceMemorySize	0x10000         // total size of the devices' storage

                     // the size of a physical read, also 'block' of cache
#define ReadBufferSize	    (0x20)

#define RecordSize	        16              // size of weather-station data record

#define MaxRecords          (((DeviceMemorySize - BaseAddress) / RecordSize) - 1)

#define DumpWidth           16              // width of hex dump in (16 = 16 charcters of data)
#define	false				(1==0)
#define true				(1==1)

// device memory is read into a an array of 'clusters' ReadBufferSize long,
// turn an actual address into a 'cluster' address
#define	ReadAddress(l)		(((int)l/ReadBufferSize) * ReadBufferSize)

#endif // _CONFIG_H
