// V0.11
// Change: JW01 - dread parameter location now a long (was int)

#ifndef _DFILE_H
#define	_DFILE_H

#ifdef	__cplusplus
extern "C" {
#endif

int dopen(char* filename);
int dread(char* buffer, long location, int size);
void dclose();

#ifdef	__cplusplus
}
#endif

#endif
