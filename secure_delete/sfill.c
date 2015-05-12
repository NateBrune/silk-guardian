/* Secure FILL - by van Hauser / [THC], vh@thc.org
 *
 * Secure FILL overwrites all available free diskspace by creating a file,
 * wiping all free diskspace it gets and finally deleting the file.
 *
 * Standard mode is a real security wipe for 38 times, flushing
 * the caches after every write. The wipe technique was proposed by Peter
 * Gutmann at Usenix '96 and includes 10 random overwrites plus 28 special
 * defined characters. Take a look at the paper of him, it's really worth
 * your time.
 * The option -l overwrites two times the data. (0xff + random)
 * The option -ll overwrites the data once. (random)
 * New with v2.3: wipes all inodes in the defined directory
 *
 * Read the manual for limitations.
 * Compiles clean on OpenBSD, Linux, Solaris and AIX
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "sdel.h"

#ifdef BLOCKSIZE
#undef BLOCKSIZE
#endif
#define BLOCKSIZE	65535
#define MAXINODEWIPE	4194304		/* 22 bits */

char **array = NULL;
int slow = O_SYNC;
int debug = 1;
int zero = 0;
int inode_only = 0;
int fd = -1;
char *filename = NULL;
FILE *f;

void help() {
    printf("sfill %s (c) 1997-2003 by %s <%s>\n\n", VERSION, AUTHOR, EMAIL);
    printf("Syntax: %s [-fiIlvz] directory\n\n", prg);
    printf("Options:\n");
    printf("\t-f  fast (and insecure mode): no /dev/urandom, no synchronize mode.\n");
    printf("\t-i  wipe only inodes in the directory specified\n");
    printf("\t-I  just wipe space, not inodes\n");
    printf("\t-l  lessens the security (use twice for total insecure mode).\n");
    printf("\t-v  is verbose mode.\n");
    printf("\t-z  last wipe writes zeros, not random data.\n");
    printf("\nsfill does a secure overwrite of the free space on the partition the specified\ndirectory resides and all free inodes of the directory specified.\n");
    printf("Default is secure mode (38 writes).\n");
    printf("You can find updates at %s\n", WEB);
    exit(1);
}

void cleanup(int signo) {
    fprintf(stderr,"\nTerminated by signal. Clean exit.\n");
    if (fd > 0) {
        fsync(fd);
        (void) close(fd);
        sync();
        if (filename != NULL && unlink(filename) != 0)
            fprintf(stderr, "Error: Could not remove temporary file %s!\n", filename);
    }
    if (array != NULL) {
	int i = 0;
	while(array[i] != NULL) {
	    unlink(array[i]);
	    i++;
	}
    }
    exit(1);
}

int main (int argc, char *argv[]) {
    int result;
    int secure = 2; /* standard is secure mode */
    int loop;
    int turn;
    int counter;
    struct stat filestat;
    struct rlimit rlim;
    char type[15] = "random";

    if (getuid() != 0)
        fprintf(stderr,"Warning: you are not root. You might not be able to wipe the whole filesystem.\n");

    prg = argv[0];
    if (argc == 1 || strncmp(argv[1], "-h", 2) == 0 || strncmp(argv[1], "--h", 3) == 0)
        help();

    while (1) {
        result = getopt(argc, argv, "fFiIlLsSvVzZ");
        if (result < 0)
            break;
        switch (result) {
            case 'f' :
            case 'F' : slow = 0;
            	       break;
            case 'i' : inode_only = 1;
                       break;
            case 'I' : inode_only = -1;
                       break;
            case 'l' :
            case 'L' : if (secure) secure--;
            	       break;
	    case 's' :
            case 'S' : secure++;
                       break;
	    case 'v' :
            case 'V' : verbose++;
                       break;
            case 'Z' :
            case 'z' : zero++;
                       break;
            default :  help();        
        }
    }
    loop = optind;
    if (loop >= argc)
	help();
    if (zero) strcpy(type, "zero");

    do {
       char newname[strlen(argv[loop]) + 16];
       strcpy(newname, argv[loop]); // can not overflow
       if (opendir(newname) == NULL) {  /* no need for ensuring close */
           fprintf(stderr, "Error: %s is not a directory\n", newname);
       } else {

            /* Generate random unique name for tempfile */
	    srand(getpid()+getuid());

            if (newname[strlen(newname)-1] != DIR_SEPERATOR)
               strcat(newname, "/");

	    turn = 0; result = 0;
            strcat(newname, "oooooooo.ooo");
            result = lstat(newname, &filestat);

            while ((result >= 0) && (turn <= 250)) {
                for (counter = strlen(newname)-1;
                     (newname[counter] != DIR_SEPERATOR);
                     counter--)
                    if (newname[counter] != '.') 
                       newname[counter] = 97+(int) (27.0 * rand() / (RAND_MAX + 1.0));
		if ((result = lstat(newname, &filestat)) >= 0)
 	 	    turn++;
            };
            if (result >= 0)
                fprintf(stderr,"Error: couldn't find a free filename in %s\n",argv[loop]);
            else {
                signal(SIGINT, cleanup);
                signal(SIGTERM, cleanup);
                signal(SIGHUP, cleanup);

		sdel_init(slow);

    	       if (verbose && inode_only < 1) {
        	   switch (secure) {
           	       case 0 : printf("Wipe mode is insecure (one pass [%s])\n",type);
                                break;
           	       case 1 : printf("Wipe mode is insecure (two passes [0xff/%s])\n",type);
                                break;
           	       default: printf("Wipe mode is secure (38 special passes)\n");
           	   }
		   printf("Wiping now ...\n");
               }

#ifdef RLIM_INFINITY
 #ifdef RLIMIT_FSIZE
               rlim.rlim_cur = RLIM_INFINITY;
               rlim.rlim_max = RLIM_INFINITY;
               if (setrlimit(RLIMIT_FSIZE, &rlim) != 0)
                   fprintf(stderr, "Warning: Could not reset ulimit for filesize.\n");
 #else
                   fprintf(stderr, "Warning: not compiled with support for resetting ulimit for filesize.\n");
 #endif
#else
                   fprintf(stderr, "Warning: not compiled with support for resetting ulimit for filesize.\n");
#endif                    

                result = 9;
                if (inode_only < 1) {
                    /* create the file */
                    if (verbose) printf("Creating %s ... ", newname);
                    if ((fd = open(newname, O_RDWR | O_EXCL | O_CREAT | O_LARGEFILE | slow, 0600 )) < 0)
                        result = 1;
                    else {
                        filename = newname;
			result = sdel_overwrite(secure, fd, 0, BLOCKSIZE, 0, zero);
                        /* Hard Flush -> Force cached data to be written to disk - the defines above! */
                        FLUSH;
                        if ((fd = open(newname, O_WRONLY | O_TRUNC)) >= 0)
                            close(fd);
                    }
                }

               if ((result == 0 && inode_only == 0) || inode_only > 0) {
                   if (result == 0 && inode_only == 0)
                       printf(" ");
                   sdel_wipe_inodes(argv[loop], array);
                   result = 0;
               }

               switch (result) {
                   case 0 : if (verbose) printf(" Finished\n");
                            break;
                   case 1 : fprintf(stderr, "Error: No write permission for %s. ", argv[loop]);
			    perror("");
                            break;
                   case 9:  break;
                   default: fprintf(stderr, "Unknown error\n");
               }
               if (unlink(newname) != 0)
                   fprintf(stderr, "Error: Could not remove temporary file %s!\n", newname);
               filename = NULL;
            }
       }
       loop++;
    } while (loop < argc);

    sdel_finnish();
    
    exit(0);
}
