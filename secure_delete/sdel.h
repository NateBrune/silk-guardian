#ifndef _SDEL_H

#include "config.h"

#define VERSION		"v3.1"
#define AUTHOR		"van Hauser / THC"
#define EMAIL		"vh@thc.org"
#define WEB		"http://www.thc.org"

#ifndef O_SYNC
 #ifdef O_FSYNC
  #define O_SYNC O_FSYNC
 #else
  #define O_SYNC 0
 #endif
#endif

#ifndef O_LARGEFILE
 #define O_LARGEFILE 0
#endif

char *prg;

extern int verbose;

extern void sdel_init(int secure_random);
extern void sdel_finnish();
extern int  sdel_overwrite(int mode, int fd, long start, unsigned long bufsize, unsigned long length, int zero);
extern int  sdel_unlink(char *filename, int directory, int truncate, int slow);
extern void sdel_wipe_inodes(char *loc, char **array);
extern void __sdel_fill_buf(char pattern[3], unsigned long bufsize, char *buf);
extern void __sdel_random_buf(unsigned long bufsize, char *buf);
extern void __sdel_random_filename(char *filename);

#define _SDEL_H
#endif
