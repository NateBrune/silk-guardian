/* Secure RM - by van Hauser / [THC], vh@thc.org
 *
 * Secure ReMove first overwrites then renames and finally deletes the target
 * file(s) specified via parameters.
 * For security reasons full 32kb blocks are written so that the whole block
 * on which the file(s) live are overwritten. (change #define #BLOCKSIZE)
 * The option -l overwrites two times the data.
 * The option -ll overwrites the data once.
 * Standard mode is a real security wipe for 38 times, flushing
 * the caches after every write. The wipe technique was proposed by Peter
 * Gutmann at Usenix '96 and includes 10 random overwrites plus 28 special
 * defined characters. Take a look at the paper of him, it's really worth
 * your time. 
 *
 * Advice : set "alias rm 'srm -v'"
 *
 * Read the manual for limitations.
 * Compiles clean on OpenBSD, Linux, Solaris, AIX and I guess all others.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "sdel.h"
   
int slow = O_SYNC;
int recursive = 0;
int zero = 0;
unsigned long bufsize = BLOCKSIZE;
int fd;

void help () {
    printf("srm %s (c) 1997-2003 by %s <%s>\n\n", VERSION, AUTHOR, EMAIL);
    printf("Syntax: %s [-dflrvz] file1 file2 etc.\n\n", prg);
    printf("Options:\n");
    printf("\t-d  ignore the two dot special files \".\" and \"..\".\n");
    printf("\t-f  fast (and insecure mode): no /dev/urandom, no synchronize mode.\n");
    printf("\t-l  lessens the security (use twice for total insecure mode).\n");
    printf("\t-r  recursive mode, deletes all subdirectories.\n");
    printf("\t-v  is verbose mode.\n");
    printf("\t-z  last wipe writes zeros instead of random data.\n");
    printf("\nsrm does a secure overwrite/rename/delete of the target file(s).\n");
    printf("Default is secure mode (38 writes).\n");
    printf("You can find updates at %s\n", WEB);
    exit(1);
}

int smash_it(char *filename, int mode)  {
    struct stat filestat;
    struct stat controlstat;
    int i_am_a_directory = 0;

/* get the file stats */
    if (lstat(filename, &filestat))
            return 1;

    if (S_ISREG(filestat.st_mode) && filestat.st_nlink > 1) {
        fprintf(stderr, "Error: File %s - file is hardlinked %d time(s), skipping!\n", filename, filestat.st_nlink - 1);
        return -1;
    }

/* if the blocksize on the filesystem is bigger than the on compiled with, enlarge! */
    if (filestat.st_blksize > bufsize) {
	if (filestat.st_blksize > 65532) {
	    bufsize = 65535;
	} else {
	    bufsize = (((filestat.st_blksize / 3) + 1) * 3);
	}
    }

/* handle the recursive mode */
    if (recursive)
        if (S_ISDIR(filestat.st_mode)) {
	    DIR *dir;
	    struct dirent *dir_entry;
	    struct stat cwd_stat;
	    char current_dir[4097];
	    int res;
	    int chdir_success = 1;
 
            if (verbose) printf("DIRECTORY (going recursive now)\n");
            getcwd(current_dir, 4096);
            current_dir[4096] = '\0';
            
            /* a won race will chmod a file to 0700 if the user is owner/root
               I'll think about a secure solution to this, however, I think
               there isn't one - anyone with an idea? */
            if (chdir(filename)) {
                (void) chmod(filename, 0700); /* ignore permission errors */
                if (chdir(filename)) {
                    fprintf(stderr,"Can't chdir() to %s, hence I can't wipe it.\n", filename);
                    chdir_success = 0;
                }
            }

	    if (chdir_success) {
	        lstat(".", &controlstat);
	        lstat("..", &cwd_stat);
	        if ( (filestat.st_dev != controlstat.st_dev) || (filestat.st_ino != controlstat.st_ino) ) {
	            fprintf(stderr, "Race found! (directory %s became a link)\n", filename);
	        } else {
  	            if ((dir = opendir (".")) != NULL) {
	                (void) chmod(".", 0700); /* ignore permission errors */
	                dir = opendir (".");
	            }
		    if (dir != NULL) {
		        while ((dir_entry = readdir(dir)) != NULL)
			    if (strcmp(dir_entry->d_name, ".") && strcmp(dir_entry->d_name, "..")) {
			        if (verbose) printf("Wiping %s ", dir_entry->d_name);
		  	        if ( (res = smash_it(dir_entry->d_name, mode)) > 0) {
		  	            if (res == 3)
		  	                fprintf(stderr,"File %s was raced, hence I won't wipe it.\n", dir_entry->d_name);
		  	            else {
			                fprintf(stderr,"Couldn't delete %s. ", dir_entry->d_name);
			                perror("");
			            }
			        } else
			            if (verbose) printf(" Done\n");
			    }
		        closedir(dir);
		    }
		}
		if(chdir(current_dir) != 0) {
		    fprintf(stderr, "Error: Can't chdir to %s (aborting) - ", current_dir);
		    perror("");
		    exit(1);
		}
/*
		lstat(current_dir, &controlstat);
		if ( (cwd_stat.st_dev != controlstat.st_dev) || (cwd_stat.st_ino != controlstat.st_ino) ) {
                    fprintf(stderr, "Race found! (directory %s was exchanged or its your working directory)\n", current_dir);
                    exit(1);
                }
*/
		i_am_a_directory = 1;
	    }
        }
/* end of recursive function */


    if (S_ISREG(filestat.st_mode)) {

       /* open the file for writing in sync. mode */
        if ((fd = open(filename, O_RDWR | O_LARGEFILE | slow)) < 0) {
            /* here again this has a race problem ... hmmm */
            /* make it writable for us if possible */
            (void) chmod(filename, 0600);   /* ignore errors */
            if ((fd = open(filename, O_RDWR | O_LARGEFILE | slow)) < 0)
    	        return 1;
        }

        fstat(fd, &controlstat);
        if ((filestat.st_dev != controlstat.st_dev) || (filestat.st_ino != controlstat.st_ino) || (! S_ISREG(controlstat.st_mode))) {
            close(fd);
            return 3;
        }

        if (sdel_overwrite(mode, fd, 0, bufsize, filestat.st_size > 0 ? filestat.st_size : 1, zero) == 0)
            return sdel_unlink(filename, 0, 1, slow);
    } /* end IS_REG() */
    else {
        if (S_ISDIR(filestat.st_mode)) {
            if (i_am_a_directory == 0) {
                fprintf(stderr,"Warning: %s is a directory. I will not remove it, because the -r option is missing!\n", filename);
                return 0;
            } else
                return sdel_unlink(filename, 1, 0, slow);
        } else
            if (! S_ISDIR(filestat.st_mode)) {
	        fprintf(stderr,"Warning: %s is not a regular file, rename/unlink only!", filename);
	        if (! verbose)
	            printf("\n");
	        return sdel_unlink(filename, 0, 0, slow);
            }
    }

    return 99; // not reached
}

void cleanup(int signo) {
    fprintf(stderr,"Terminated by signal. Clean exit.\n");
    if (fd >= 0)
    	close(fd);
    FLUSH;
    exit(1);
}

int main (int argc, char *argv[]) {
    int errors = 0;
    int dot = 0;
    int result;
    int secure = 2; /* Standard is now SECURE mode (38 overwrites) [since v2.0] */
    int loop;
    struct rlimit rlim;

    prg = argv[0];
    if (argc < 2 || strncmp(argv[1], "-h", 2) == 0|| strncmp(argv[1], "--h", 3) == 0)
        help();

    while (1) {
        result = getopt(argc, argv, "DdFfLlRrSsVvZz");
        if (result < 0) break;
        switch (result) {
            case 'd' :
            case 'D' : dot = 1;
                       break;
            case 'F' :
            case 'f' : slow = 0;
            	       break;
	    case 'L' :
	    case 'l' : if (secure) secure--;
	    	       break;
	    case 'R' :
	    case 'r' : recursive++;
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
    loop = optind;
    if (loop == argc)
        help();

    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);
    signal(SIGHUP, cleanup);

    sdel_init(slow);

    if (verbose) {
        char type[15] = "random";
        if (zero) strcpy(type, "zero");
        switch (secure) {
           case 0 : printf("Wipe mode is insecure (one pass [%s])\n",type);
		    break;
	   case 1 : printf("Wipe mode is insecure (two passes [0xff/%s])\n",type);
	   	    break;
	   default: printf("Wipe mode is secure (38 special passes)\n");
	}
    }

#ifdef RLIM_INFINITY
 #ifdef RLIMIT_FSIZE
    rlim.rlim_cur = RLIM_INFINITY;
    rlim.rlim_max = RLIM_INFINITY;
    if (setrlimit(RLIMIT_FSIZE, &rlim) != 0)
    	fprintf(stderr, "Warning: Could not reset ulimit for filesize.\n");
 #else
    fprintf(stderr, "Warning: Not compiled with support for resetting ulimit filesize.\n");
 #endif
#endif

    while (loop < argc) {
        char rmfile[strlen(argv[loop]) + 1];
        strcpy(rmfile, argv[loop]);
        loop++;
        if (strcmp("/", rmfile) == 0) {
            fprintf(stderr,"Warning: Do you really want to remove the ROOT directory??\n");
            fprintf(stderr,"I'm giving you 5 seconds to abort ... press Control-C\n");
	    sleep(6);
            fprintf(stderr,"Doing my evil work now, don't whimp later, you had been informed!\n");
        }
        if (dot)
            if ((strcmp(".", rmfile) == 0) || (strcmp("..", rmfile) == 0))
                continue;
        if (verbose)
            printf("Wiping %s ", rmfile);
        result = (int) smash_it(rmfile, secure);
        switch (result) {
            case 0 : if (verbose) printf(" Done\n");
                     break;
            case 1 : fprintf(stderr, "Error: File %s - ", rmfile);
                     perror("");
                     break;
            case -1: break;
            case 3 : fprintf(stderr, "File %s was raced, hence I won't wipe it!\n", rmfile);
                     break;
            default: fprintf(stderr, "Unknown error\n");
        }
        if (result)
            errors++;
    }

    sdel_finnish();

    if (errors)
        return 1;
    else
        return 0;
}
