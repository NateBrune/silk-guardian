/* Secure Delete Library - by van Hauser / [THC], vh@thc.org
 *
 * Secure Delete Library provides the following public functions:
 *
 * void sdel_init(int secure_random)
 *      Initializiation function for sdel_overwrite. It needs to be called
 *      once at program start, not for each file to be overwritten.
 *      Options:
 *          secure_random - if != 0 defines that the secure random number
 *                          generator RANDOM_DEVICE should be used
 *
 * void sdel_finnish()
 *      Clean-up function, if sdel_init() was called in a program. It needs
 *      only to be called at the end of the program.
 *
 * int  sdel_overwrite(int mode, int fd, long start, unsigned long bufsize,
 *                     unsigned long length, int zero)
 *      This is the heart of sdel-lib. It overwrites the target file
 *      descriptor securely to make life hard even for the NSA.
 *      Read the next paragraph for the techniques.
 *      Options:
 *          mode = 0 - once overwrite with random data
 *                 1 - once overwrite with 0xff, then once with random data
 *                 2 - overwrite 38 times with special values
 *          fd       - filedescriptor of the target to overwrite
 *          start    - where to start overwriting. 0 is from the beginning
 *                     this is needed for wiping swap spaces etc.
 *          bufsize  - size of the buffer to use for overwriting, depends
 *                     on the filesystem
 *          length   - amount of data to write (file size), 0 means until
 *                     an error occurs
 *          zero     - last wipe is zero bytes, not random
 *      returns 0 on success, -1 on errors
 *
 * int  sdel_unlink(char *filename, int directory, int truncate, int slow)
 *      First truncates the file (if it is not a directory), then renames it
 *      and finally rmdir/unlinks the target.
 *      Options:
 *          filename  - filename/directory to unlink/rmdir
 *          directory - if != 0, it is a directory
 *          truncate  - if != 0, it truncates the file
 *          slow      - is either O_SYNC (see open(2)) or 0
 *      returns 0 on success, -1 on errors
 *
 * Compiles clean on OpenBSD, Linux, Solaris, AIX and I guess all others.
 *
 */ 

/*
 * For security reasons full 32kb blocks are written so that the whole block
 * on which the file(s) live are overwritten. (change #define #BLOCKSIZE)
 * Standard mode is a real security wipe for 38 times, flushing
 * the caches after every write. The wipe technique was proposed by Peter
 * Gutmann at Usenix '96 and includes 10 random overwrites plus 28 special
 * defined characters. Take a look at the paper of him, it's really worth
 * your time. 
 *
 * Read the manual for limitations.
 */

#include "sdel-lib.h"

//
// STARTING FUNCTIONS
//

void __sdel_fill_buf(char pattern[3], unsigned long bufsize, char *buf) {
    int loop;
    int where;
    
    for (loop = 0; loop < (bufsize / 3); loop++) {
        where = loop * 3;
	*buf++ = pattern[0];
	*buf++ = pattern[1];
	*buf++ = pattern[2];
    }
}

void __sdel_random_buf(unsigned long bufsize, char *buf) {
    int loop;
    
    if (devrandom == NULL)
        for (loop = 0; loop < bufsize; loop++)
            *buf++ = (unsigned char) (256.0*rand()/(RAND_MAX+1.0));
    else
        fread(buf, bufsize, 1, devrandom);
}

void __sdel_random_filename(char *filename) {
    int i;
    for (i = strlen(filename) - 1;
         (filename[i] != DIR_SEPERATOR) && (i >= 0);
         i--)
        if (filename[i] != '.') /* keep dots in the filename */
            filename[i] = 97+(int) ((int) ((256.0 * rand()) / (RAND_MAX + 1.0)) % 26);
}

void sdel_init(int secure_random) {

    (void) setvbuf(stdout, NULL, _IONBF, 0);
    (void) setvbuf(stderr, NULL, _IONBF, 0);

    if (BLOCKSIZE<16384)
        fprintf(stderr, "Programming Warning: in-compiled blocksize is <16k !\n");
    if (BLOCKSIZE % 3 > 0)
        fprintf(stderr, "Programming Error: in-compiled blocksize is not a multiple of 3!\n");

    srand( (getpid()+getuid()+getgid()) ^ time(0) );
    devrandom = NULL;
#ifdef RANDOM_DEVICE
    if (secure_random) {
        if ((devrandom = fopen(RANDOM_DEVICE, "r")) != NULL)
            if (verbose)
                printf("Using %s for random input.\n", RANDOM_DEVICE);
    }
#endif

    __internal_sdel_init = 1;
}

void sdel_finnish() {
    if (devrandom != NULL) {
        fclose(devrandom);
        devrandom = NULL;
    }
    if (! __internal_sdel_init) {
        fprintf(stderr, "Programming Error: sdel-lib was not initialized before calling sdel_finnish().\n");
        return;
    }
    __internal_sdel_init = 0;
}

/*
 * secure_overwrite function parameters:
 * mode = 0 : once overwrite with random data
 *        1 : once overwrite with 0xff, then once with random data
 *        2 : overwrite 38 times with special values
 * fd       : filedescriptor of the target to overwrite
 * start    : where to start overwriting. 0 is from the beginning
 * bufsize  : size of the buffer to use for overwriting, depends on the filesystem
 * length   : amount of data to write (file size), 0 means until an error occurs
 *
 * returns 0 on success, -1 on errors
 */
int sdel_overwrite(int mode, int fd, long start, unsigned long bufsize, unsigned long length, int zero) {
    unsigned long writes;
    unsigned long counter;
    int turn;
    int last = 0;
    char buf[65535];
    FILE *f;

    if (! __internal_sdel_init)
        fprintf(stderr, "Programming Error: sdel-lib was not initialized before sdel_overwrite().\n");

    if ((f = fdopen(fd, "r+b")) == NULL)
        return -1;

/* calculate the number of writes */
    if (length > 0)
        writes = (1 + (length / bufsize));
    else
        writes = 0;

/* do the first overwrite */
    if (start == 0)
        rewind(f);
    else
        if (fseek(f, start, SEEK_SET) != 0)
            return -1;
    if (mode != 0 || zero) {
        if (mode == 0)
            __sdel_fill_buf(std_array_00, bufsize, buf);
        else
            __sdel_fill_buf(std_array_ff, bufsize, buf);
        if (writes > 0)
            for (counter=1; counter<=writes; counter++)
                fwrite(&buf, 1, bufsize, f); // dont care for errors
        else
            do {} while(fwrite(&buf, 1, bufsize, f) == bufsize);
        if (verbose)
            printf("*");
        fflush(f);
        if (fsync(fd) < 0)
            FLUSH;
        if (mode == 0)
            return 0;
    }

/* do the rest of the overwriting stuff */
    for (turn = 0; turn <= 36; turn++) {
        if (start == 0)
            rewind(f);
        else
            if (fseek(f, start, SEEK_SET) != 0)
                return -1;
        if ((mode < 2) && (turn > 0))
            break;
        if ((turn >= 5) && (turn <= 31)) {
            __sdel_fill_buf(write_modes[turn-5], bufsize, buf);
            if (writes > 0)
                for (counter = 1; counter <= writes; counter++)
                    fwrite(&buf, 1, bufsize, f); // dont care for errors
            else
                do {} while(fwrite(&buf, 1, bufsize, f) == bufsize);
        } else {
            if (zero && ((mode == 2 && turn == 36) || mode == 1)) {
                last = 1;
                __sdel_fill_buf(std_array_00, bufsize, buf);
            }
            if (writes > 0) {
	        for (counter = 1; counter <= writes; counter++) {
	            if (! last)
                        __sdel_random_buf(bufsize, buf);
	            fwrite(&buf, 1, bufsize, f); // dont care for errors
	        }
	    } else {
	        do {
	            if (! last)
                        __sdel_random_buf(bufsize, buf);
	        } while (fwrite(&buf, 1, bufsize, f) == bufsize); // dont care for errors
	    }
        }
        fflush(f);
        if (fsync(fd) < 0)
            FLUSH;
        if (verbose)
            printf("*");
    }

    (void) fclose(f);
/* Hard Flush -> Force cached data to be written to disk */
    FLUSH;

    return 0;
}

/*
 * secure_unlink function parameters:
 * filename   : the file or directory to remove
 * directory  : defines if the filename poses a directory
 * truncate   : truncate file
 * slow       : do things slowly, to prevent caching
 *
 * returns 0 on success, -1 on errors.
 */
int sdel_unlink(char *filename, int directory, int truncate, int slow) {
   int fd;
   int turn = 0;
   int result;
   char newname[strlen(filename) + 1];
   struct stat filestat;

/* open + truncating the file, so an attacker doesn't know the diskblocks */
   if (! directory && truncate)
       if ((fd = open(filename, O_WRONLY | O_TRUNC | slow)) >= 0)
           close(fd);

/* Generate random unique name, renaming and deleting of the file */
    strcpy(newname, filename); // not a buffer overflow as it has got the exact length

    do {
        __sdel_random_filename(newname);
        if ((result = lstat(newname, &filestat)) >= 0)
            turn++;
    } while ((result >= 0) && (turn <= 100));

    if (turn <= 100) {
       result = rename(filename, newname);
       if (result != 0) {
          fprintf(stderr, "Warning: Couldn't rename %s - ", filename);
          perror("");
          strcpy(newname, filename);
       }
    } else {
       fprintf(stderr,"Warning: Couldn't find a free filename for %s!\n",filename);
       strcpy(newname, filename);
    }

    if (directory) {
        result = rmdir(newname);
        if (result) {
            printf("Warning: Unable to remove directory %s - ", filename);
            perror("");
	    (void) rename(newname, filename);
	} else
	    if (verbose)
	        printf("Removed directory %s ...", filename);
    } else {
        result = unlink(newname);
        if (result) {
            printf("Warning: Unable to unlink file %s - ", filename);
            perror("");
            (void) rename(newname, filename);
        } else
            if (verbose)
                printf(" Removed file %s ...", filename);
    }

    if (result != 0)
        return -1;

    return 0;
}

void sdel_wipe_inodes(char *loc, char **array) {
    char *template = malloc(strlen(loc) + 16);
    int i = 0;
    int fail = 0;
    int fd;

    if (verbose)
        printf("Wiping inodes ...");

    array = malloc(MAXINODEWIPE * sizeof(template));
    strcpy(template, loc);
    if (loc[strlen(loc) - 1] != '/')
        strcat(template, "/");
    strcat(template, "xxxxxxxx.xxx");
       
    while(i < MAXINODEWIPE && fail < 5) {
        __sdel_random_filename(template);
        if (open(template, O_CREAT | O_EXCL | O_WRONLY, 0600) < 0)
            fail++;
        else {
            array[i] = malloc(strlen(template));
            strcpy(array[i], template);
            i++;
        }
    }
    FLUSH;
       
    if (fail < 5) {
        fprintf(stderr, "Warning: could not wipe all inodes!\n");
    }
       
    array[i] = NULL;
    fd = 0;
    while(fd < i) {
        unlink(array[fd]);
        free(array[fd]);
        fd++;
    }
    free(array);
    array = NULL;
    FLUSH;
    if (verbose)
        printf(" Done ... ");
}
