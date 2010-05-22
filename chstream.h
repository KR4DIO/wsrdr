// V0.1

#ifndef _CHSTREAM_H
#define	_CHSTREAM_H

#ifdef	__cplusplus
extern "C" {
#endif

void uopen();
int uerror();
void uflush();
void useek(int location);
void urewind();
char ugetc();
int uread(char* buffer, int size);

#ifdef	__cplusplus
}
#endif

#endif	/* _CHSTREAM_H */
