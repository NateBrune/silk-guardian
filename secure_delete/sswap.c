/* Secure SWAP cleaner - by van Hauser / [THC], vh@thc.org
 *
 * Note that this program is beta. It was tested with linux, but I can't
 * tell for other platforms. Read the statement at #define SWAP_PAGESIZE
 * on how to use this program on other unix machines.
 *
 * of course: you have to turn of the swapspace before using this program !
 *
 * Secure SWAP overwrites all data on your swap device.
 * Standard mode is a real security wipe for 38 times, flushing
 * the caches after every write. The wipe technique was proposed by Peter
 * Gutmann at Usenix '96 and includes 10 random overwrites plus 28 special
 * defined characters. Take a look at the paper of him, it's really worth
 * your time.
 * The option -l overwrites two times the data. (0xff + random)
 * The option -ll overwrites the data once. (random)
 *
 * Read the manual for limitations.
 * Compiles clean on OpenBSD, Linux, Solaris and AIX
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <strings.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "sdel.h"

/* SWAP_PAGESIZE is an important variable. You have to set this
 * to your header length of your swapdevice. For Linux this is 4096,
 * I don't know for the other OSs. To be sure, set this to 0 and
 * recreate your swapspace afterwards (for linux: mkswap /dev/swapdevice)
 */
#define SWAP_PAGESIZE	4096
#ifdef BLOCKSIZE
 #undef BLOCKSIZE
#endif
#define BLOCKSIZE	65535

int fd;
int slow = O_SYNC;
int zero = 0;

void help() {
    printf("sswap %s (c) 1997-2003 by %s <%s>\n\n", VERSION, AUTHOR, EMAIL);
    printf("Syntax: %s [-flvz] [-j start] /dev/of_swap_device\n\n", prg);
    printf("Options:\n");
    printf("\t-f  fast (and insecure mode): no /dev/urandom, no synchronize mode.\n");
    printf("\t-j  jump over the first number of bytes when wiping. (default: %d)\n", SWAP_PAGESIZE);
    printf("\t-l  lessens the security (use twice for total insecure mode).\n");
    printf("\t-v  is verbose mode.\n");
    printf("\t-z  last wipe writes zeros instead of random data.\n");
    printf("\nsswap does a secure overwrite of the swap space.\n");
    printf("Default is secure mode (38 writes).\n");
    printf("Updates can be found at %s\n", WEB);
    printf("\nNOTE: You must disable the swapspace before using this program!\007\n");
    exit(1);
}

void cleanup() {
    fprintf(stderr,"\nTerminated by signal. Clean exit.\n");
    close(fd);
    sync();
    exit(1);
}

int main (int argc, char *argv[]) {
    int secure = 2;
    int result;
    int mode;
    unsigned long start = SWAP_PAGESIZE;
    struct stat stats;
    char *filename;
    
    prg = argv[0];
    if (argc == 1 || strncmp(argv[1], "-h", 2) == 0 || strncmp(argv[1], "--h", 3) == 0)
        help();

    while (1) {
        result = getopt(argc, argv, "FfJ:j:LlSsVvZz");
        if (result<0) break;
        switch (result) {
            case 'F' :
            case 'f' : slow = 0;
                       break;
            case 'J' :
            case 'j' : start = atol(optarg);
                       if (start < 0 || start > 65535) {
                           fprintf(stderr, "Error: The -j option must be set between 0 and 65535!\n");
                           exit(-1);
                       }
                       break;
            case 'L' :
            case 'l' : if (secure) secure--;
            	       break;
            case 'S' :
            case 's' : secure++;
                       break;
            case 'V' :
            case 'v' : verbose++;
                       break;
            case 'Z' :
            case 'z' : zero++;
                       break;
            default :  help();
        }
    }

    if ((optind+1) != argc)
        help();

    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);
    signal(SIGHUP, cleanup);

    filename = argv[optind];
    mode = secure;

    if ((fd = open (filename, O_RDWR | O_LARGEFILE | slow)) < 0) {
        fprintf(stderr, "Error: Can't open %s for writing.", filename);
        perror("");
        exit(1);
    }

    fstat(fd, &stats);
    if (!S_ISBLK(stats.st_mode)) {
        fprintf(stderr, "Error: Target is not a block device - %s\n", filename);
        exit(1);
    }

    if (verbose) {
        char type[15] = "random";
        if (zero) strcpy(type, "zero");
        switch (mode) {
           case 0 : printf("Wipe mode is insecure (one pass [%s])\n",type);
                    break;
           case 1 : printf("Wipe mode is insecure (two passes [0xff/%s])\n",type);
                    break;
           default: printf("Wipe mode is secure (38 special passes)\n");
        }
 	printf("Writing to device %s: ", filename);
    }

    sdel_init(slow);

    if (sdel_overwrite(mode, fd, start, BLOCKSIZE, 0, zero) == 0)
        if (verbose)
    	    printf(" done\n");

    sdel_finnish();

    /* thats all */
    exit(0);
}
