/* Secure MEMORY cleaner - by van Hauser / [THC], vh@thc.org
 *
 * Note that this program is beta. It was tested with linux, solaris and
 * openbsd but I can't tell for other platforms. 
 *
 * Secure MEMORY overwrites all data in your memory it gets.
 * The any -l option this does a real security wipe for 38 times, flushing
 * the caches after every write. The wipe technique was proposed by Peter
 * Gutmann at Usenix '96 and includes 10 random overwrites plus 29 special
 * defined characters. Take a look at the paper of him, it's really worth
 * your time.
 * If run with one -l option, it wipes the memory twice, first with null
 * bytes, then with random values.
 * If run with two -l options, it wipes the memory only once with null bytes.
 *
 * Note that it is *very* slow. You might run it with "-llf"
 *
 * Read the manual for limitations.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
                     
#include "sdel.h"

#ifdef BLOCKSIZE
#undef BLOCKSIZE
#endif
#define BLOCKSIZE 65536

char buf[BLOCKSIZE+2];
int slow = 1;

extern FILE *devrandom;

void help() {
    printf("smem %s (c) 1997-2003 by %s <%s>\n\n", VERSION, AUTHOR, EMAIL);
    printf("Syntax: %s [-flv]\n\n", prg);
    printf("Options:\n");
    printf("\t-f  fast (and insecure mode): no /dev/urandom.\n");
    printf("\t-l  lessens the security (use twice for total insecure mode).\n");
    printf("\t-v  is verbose mode.\n");
    printf("\nsmem does a secure overwrite of the memory (RAM), because memory contents can\n");
    printf("be recovered even after a shutdown! Default is secure mode (38 writes).\n");
    printf("You can find updates at %s\n", WEB);
    exit(1);
}

int smash_it(int mode) {
    unsigned char write_modes[27][3] = {
        {"\x55\x55\x55"}, {"\xaa\xaa\xaa"}, {"\x92\x49\x24"}, {"\x49\x24\x92"},
        {"\x24\x92\x49"}, {"\x00\x00\x00"}, {"\x11\x11\x11"}, {"\x22\x22\x22"},
        {"\x33\x33\x33"}, {"\x44\x44\x44"}, {"\x55\x55\x55"}, {"\x66\x66\x66"},
        {"\x77\x77\x77"}, {"\x88\x88\x88"}, {"\x99\x99\x99"}, {"\xaa\xaa\xaa"},
        {"\xbb\xbb\xbb"}, {"\xcc\xcc\xcc"}, {"\xdd\xdd\xdd"}, {"\xee\xee\xee"},
        {"\xff\xff\xff"}, {"\x92\x49\x24"}, {"\x49\x24\x92"}, {"\x24\x92\x49"},
        {"\x6d\xb6\xdb"}, {"\xb6\xdb\x6d"}, {"\xdb\x6d\xb6"}
    };

    int turn;
    unsigned int counter = 0;
    unsigned char buffers[27][BLOCKSIZE+2];
    char *ptr;
    struct rlimit rlim;
    
    if (verbose) {
        switch (mode) {
           case 0 : printf("Wipe mode is insecure (one pass with 0x00)\n");
                    break;
           case 1 : printf("Wipe mode is insecure (two passes [0x00/random])\n");                    break;
           default: printf("Wipe mode is secure (38 special passes)\n");
        }
    }

    if (slow && mode)
        if ((devrandom = fopen(RANDOM_DEVICE, "r")) != NULL)
            if (verbose)
                printf("Using %s for random input.\n", RANDOM_DEVICE);

/* We set a new ulimit, so we can grab all memory ... */
#ifdef RLIM_INFINITY
    rlim.rlim_cur = RLIM_INFINITY;
    rlim.rlim_max = RLIM_INFINITY;
 #ifdef RLIMIT_DATA
    if (setrlimit(RLIMIT_DATA, &rlim) != 0)
        fprintf(stderr, "Warning: Could not reset ulimit for data.\n");
 #endif
 #ifdef RLIMIT_STACK
    if (setrlimit(RLIMIT_STACK, &rlim) != 0)
        fprintf(stderr, "Warning: Could not reset ulimit for stack.\n");
 #endif
 #ifdef RLIMIT_RSS
    if (setrlimit(RLIMIT_RSS, &rlim) != 0)
	fprintf(stderr, "Warning: Could not reset ulimit for rss.\n");
 #endif
 #ifdef RLIMIT_MEMLOCK
    if (setrlimit(RLIMIT_MEMLOCK, &rlim) != 0)
	fprintf(stderr, "Warning: Could not reset ulimit for mem locked.\n");
 #endif
 #ifndef RLIMIT_DATA
  #ifndef RLIMIT_STACK
   #ifndef RLIMIT_RSS
    #ifndef RLIMIT_MEMLOCK
        fprintf(stderr, "Warning: Not compiled with support for resetting ulimits for memory\n");
    #endif
   #endif
  #endif
 #endif
#else
        fprintf(stderr, "Warning: Not compiled with support for resetting ulimits for memory\n");
#endif

    if (mode > 1) {
        for (turn=0; turn<27; turn++) {
            __sdel_fill_buf(write_modes[turn], BLOCKSIZE + 2, buf);
            memcpy(buffers[turn], buf, BLOCKSIZE);
        }
    }

    alarm(600); /* needed to prevent mem caching */

    while ( (ptr = calloc(4096, 16)) != NULL) {
        if (mode > 0) {
            for (turn=0; turn<=36; turn++) {
                if ((mode == 1) && (turn > 0)) break;
                if ((turn>=5) && (turn<=31)) {
                    memcpy(ptr, buffers[turn-5], BLOCKSIZE);
                } else {
                    __sdel_random_buf(BLOCKSIZE + 2, buf);
                    memcpy(ptr, buf, BLOCKSIZE);
       	        }
      	    }
        }
        if (verbose && (counter > 8)) { /* every 512kb */
            printf("*");
            counter = 0;
        } else counter++;
    }

    if (devrandom)
        fclose(devrandom);
    if (verbose)
	printf(" done\n");
    return 0;
}

void cleanup() {
    fprintf(stderr,"Terminated by signal. Clean exit.\n");
    if (devrandom)
            fclose(devrandom);
    fflush(stdout);
    fflush(stderr);
    exit(1);
}

int main (int argc, char *argv[]) {
    int secure = 2;
    int result;
    
    prg = argv[0];
    if (argc == 2)
        if ( (strncmp(argv[1],"-h", 2) == 0) || (strcmp(argv[1],"-?") == 0) )
            help();

    while (1) {
        result = getopt(argc, argv, "FfLlSsVvZz");
        if (result<0) break;
        switch (result) {
            case 'F' :
            case 'f' : slow = 0;
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
            case 'Z':
            case 'z': break;
            default :  help();
        }
    }

    if (optind < argc)
        help();

    printf("Starting Wiping the memory, press Control-C to abort earlier. Help: \"%s -h\"\n", prg);

    (void) setvbuf(stdout, NULL, _IONBF, 0);
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);
    signal(SIGHUP, cleanup);
    signal(SIGALRM, cleanup);

    smash_it(secure);

    /* thats all */
    exit(0);
}
