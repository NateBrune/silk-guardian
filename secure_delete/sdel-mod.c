/*
 * sdel-mod.c (c)2003 by van Hauser / THC <vh@thc.org> and Frank Heimann <homy@gmx.net>
 *
 */

//#define _DEBUG_

#include <linux/config.h>
#include <linux/version.h>

#if defined(MODVERSIONS)
#include <linux/modversions.h>
#endif
#include <linux/kernel.h>

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <sys/syscall.h>
#include <errno.h>
#include <asm/uaccess.h>
#include <asm/smplock.h>

#define LOG(x,y) printk( KERN_DEBUG "sdel-mod: " x,y )

#if !defined SEEK_SET
 #define SEEK_SET 0
#endif

extern void *sys_call_table[];

int (*unlink_orig)(const char *filename);
int (*lstat_orig)( const char *file_name, struct stat *buf );
int (*fstat_orig)(int filedes, struct stat *buf);
int (*rename_orig)(const char *oldpath, const char *newpath);
int (*open_orig)(const char *filename, int flags);
int (*close_orig)(int fd);
ssize_t (*read_orig)(int fd, void *buf, size_t count);
ssize_t (*write_orig)(int fd, const void *buf, size_t count);
int (*sync_orig)(void);
int (*fsync_orig)(int fd);
off_t (*lseek_orig)(int fildes, off_t offset, int whence);
int (*setrlimit_orig)(int resource, const struct rlimit *rlim);
int (*brk_orig)(void *end_data_segment);
char* (*getcwd_orig)(char *buf, size_t size);

void cleanup_module(void);

unsigned char write_modes[27][3] = {
        {"\x55\x55\x55"}, {"\xaa\xaa\xaa"}, {"\x92\x49\x24"}, {"\x49\x24\x92"},
        {"\x24\x92\x49"}, {"\x00\x00\x00"}, {"\x11\x11\x11"}, {"\x22\x22\x22"},
        {"\x33\x33\x33"}, {"\x44\x44\x44"}, {"\x55\x55\x55"}, {"\x66\x66\x66"},
        {"\x77\x77\x77"}, {"\x88\x88\x88"}, {"\x99\x99\x99"}, {"\xaa\xaa\xaa"},
        {"\xbb\xbb\xbb"}, {"\xcc\xcc\xcc"}, {"\xdd\xdd\xdd"}, {"\xee\xee\xee"},
        {"\xff\xff\xff"}, {"\x92\x49\x24"}, {"\x49\x24\x92"}, {"\x24\x92\x49"},
        {"\x6d\xb6\xdb"}, {"\xb6\xdb\x6d"}, {"\xdb\x6d\xb6"}
    };

unsigned char std_array[3] = "\xff\xff\xff";

#define DIR_SEPERATOR	'/'
#define FLUSH		sync_orig()
#define BLOCKSIZE       32769

#ifndef O_SYNC
 #ifdef O_FSYNC
  #define O_SYNC O_FSYNC
 #else
  #define O_SYNC 0
 #endif
#endif

#define RAND_MAX        2147483647

unsigned long bufsize = BLOCKSIZE;
char buf[BLOCKSIZE];
int slow = O_SYNC;

void __sdel_random_filename(char *filename) {
    int i;
    unsigned char rand;
    for (i = strlen(filename) - 1;
         (filename[i] != '/') && (i >= 0);
         i--)
        if (filename[i] != '.') { /* keep dots in the filename */
            get_random_bytes(&rand, 1);
            filename[i] = 97 + (int) ((int) rand % 26);
        }
}

/*
 * secure_unlink function parameters:
 * filename   : the file or directory to UNLINK
 *
 * returns 0 on success, -1 on errors.
 */
static int sdel_unlink(const char *filename) {
   int turn = 0;
   int result;
   char newname[strlen(filename) + 1]; // just in kernelspace
   char *ul_newname=0; // for memory in userspace, syscalls need all userspace 
   unsigned long mmm=0; // for storing old memory pointer
   struct stat filestat;

/* Generate random unique name, renaming and deleting of the file */
    strcpy(newname, filename); // not a buffer overflow as it has got the exact length

    do {
        __sdel_random_filename(newname);
        if ((result = lstat_orig(newname, &filestat)) >= 0)
            turn++;
    } while ((result >= 0) && (turn <= 100));

    if (turn <= 100) {
       mmm = current->mm->brk;
       if( brk_orig((void*) mmm + strlen(filename) + 1 ) < 0) {
	       LOG( "Can't allocate userspace mem %s","\n" );
	       return (-ENOMEM);
       }
       ul_newname = (void*)(mmm + 2); // set variable to new allocates userspace mem
       copy_to_user(ul_newname,newname,strlen(newname));
       result = rename_orig(filename, ul_newname);
       if (result != 0) {
          LOG("Warning: Couldn't rename %s\n", filename);
          strcpy(newname, filename);
       }
    } else {
       LOG("Warning: Couldn't find a free filename for %s\n",filename);
       strcpy(newname, filename);
    }

    result = unlink_orig(ul_newname);
    if (result) {
        LOG("Warning: Unable to unlink file %s\n", filename);
        (void) rename_orig(newname, filename);
    }
#if defined _DEBUG_
      else
        LOG("Renamed and unlinked file %s\n", filename);
#endif

    if (result != 0)
        return -1;

    if( brk_orig((void*) mmm) < 0 )
	    return (-ENOMEM);
    return 0;
}

static void fill_buf(char pattern[3])
{
	int loop;
	int where;

	for (loop = 0; loop < (bufsize / 3); loop++)
	{
		where = loop * 3;
		buf[where] = pattern[0];
		buf[where+1] = pattern[1];
		buf[where+2] = pattern[2];
	}
}

static void random_buf(char* ul_buf) {
	get_random_bytes(buf, bufsize);
	copy_to_user(ul_buf, buf, bufsize);
}

static int smash_it(const char *ul_filename, const char*kl_filename, struct stat kl_filestat, int mode) {
	unsigned long writes;
	unsigned long counter;
	unsigned long filesize;
	struct stat kl_controlstat;
	struct stat *tmp;
	int turn;
	int i;
	int kl_file;

	char *kl_newname=0;

	unsigned long mmm;
	char *ul_buf = NULL;

#if defined _DEBUG_
	LOG("smashing with mode %d\n", mode);
#endif

/* if the blocksize on the filesystem is bigger than the on compiled with, enlarge! */
	if (kl_filestat.st_blksize > bufsize) {
		if (kl_filestat.st_blksize > ( BLOCKSIZE - 3 ))
			bufsize = BLOCKSIZE;
		else
			bufsize = (((kl_filestat.st_blksize / 3) + 1) * 3);
	}

/* open the file for writing in sync. mode */
	if ((kl_file = open_orig(ul_filename, O_RDWR)) < 0) {
		LOG("open failed %d\n", kl_file);
		return kl_file;
	}

//	LOG("open %s\n", "ok");

	mmm = current->mm->brk;
	if(brk_orig((void*) mmm + sizeof(struct stat)) < 0) {
		LOG("brk %s\n", "failed");
		return (-ENOMEM);
	}

	tmp = (void*)(mmm +  2);
//	LOG( "brk %s\n","ok" );

	// do we need to check for races? hmmm
	if ((i = fstat_orig(kl_file, tmp)) < 0) {
		LOG("fstat failed %d\n", i);
		if (brk_orig((void*) mmm) < 0)
			return (-ENOMEM);
		return i;
	}
//	LOG( "brk %s\n","ok" );

	copy_from_user(&kl_controlstat, tmp, sizeof(struct stat));
	if (brk_orig((void*) mmm) < 0)
		return (-ENOMEM);

	if ((kl_filestat.st_dev != kl_controlstat.st_dev) || (kl_filestat.st_ino != kl_controlstat.st_ino) ||
	    (! S_ISREG(kl_controlstat.st_mode))) {
		LOG( "RACE - CONDITION %s\n"," " );
		return (-EIO);
	}

/* calculate the number of writes */
	filesize = kl_filestat.st_size;
	writes = (1 + (filesize / bufsize));

#if defined _DEBUG_
	LOG("start overwriting in mode %d\n", mode);
#endif

//	if (mode != 0) {
		fill_buf(std_array);

		mmm = current->mm->brk;
		if (brk_orig((void*) mmm + bufsize) < 0) {
			LOG("brk %s\n", "failed");
			return (-ENOMEM);
		}

		ul_buf = (void*)(mmm + 2);
//		LOG("brk %s\n", "ok");
		copy_to_user(ul_buf, buf, bufsize);

		for (counter=1; counter<=writes; counter++) {
			if ((i = write_orig(kl_file, ul_buf, bufsize)) < 0)
				LOG("write failure: %d\n", i);
		}

		if (fsync_orig(kl_file) < 0)
			FLUSH;
//	}

/* do the overwriting stuff */
    if (mode > 0) {
	for (turn=0; turn<=36; turn++) {

		if (lseek_orig(kl_file, SEEK_SET, 0) < 0)
			LOG( "lseek %s\n", "failed");

		if ((mode < 2) && (turn > 0))
			break;

		if ((turn>=5) && (turn<=31)) {
#if defined _DEBUG_
			LOG("pattern o/w%s\n", " ");
#endif
			fill_buf(write_modes[turn-5]);
			copy_to_user(ul_buf, buf, bufsize);
			for (counter=1; counter<=writes; counter++)
				write_orig(kl_file, ul_buf, bufsize);
		}
		else {
#if defined _DEBUG_
			LOG("random o/w%s\n", " ");
#endif
			for (counter=1; counter<=writes; counter++) {
				random_buf(ul_buf);
				write_orig(kl_file,ul_buf,bufsize);
			}
		}

		if (fsync_orig(kl_file) < 0)
			FLUSH;
	}
    }

	if (brk_orig((void*) mmm) < 0)
		return (-ENOMEM);

/* Hard Flush -> Force cached data to be written to disk */
	FLUSH;
/* open + truncating the file, so an attacker doesn't know the diskblocks */
	if ((kl_file = open_orig(ul_filename, O_WRONLY | O_TRUNC | slow)) >= 0)
		close_orig(kl_file);

	if (brk_orig((void*) mmm) < 0)
		return (-ENOMEM);

	kfree(kl_newname);

	return 0;
}

int wipefile(const char *ul_filename) {
	struct stat kl_filestat;
	struct stat *ul_fs;
	int ret;
	int size;
	char *kl_filename;
	unsigned long mmm;

	lock_kernel();
	MOD_INC_USE_COUNT;

	kl_filename = getname(ul_filename);
	ret = PTR_ERR(kl_filename);

	if (IS_ERR(kl_filename)) {
		LOG("getname %s\n", "failed");
		putname(kl_filename);
		return ret;
	}

//	LOG("getname %s\n", "ok");

	size = strlen(kl_filename);

	if (size <= PATH_MAX) {
		mmm = current->mm->brk;
		ret = brk_orig((void*) mmm + sizeof(struct stat));
		ul_fs = (void*)(mmm + 2);

		if ((ret = (*lstat_orig)(ul_filename, ul_fs)) < 0) {
//			LOG("lstat returned %d\n", ret);
//			LOG("on: %s\n", kl_filename);
			
			
			if (brk_orig((void*) mmm) < 0)
				return (-ENOMEM);
		} else {
			copy_from_user(&kl_filestat, ul_fs, sizeof(struct stat));
			if (brk_orig((void*) mmm ) < 0)
				return (-ENOMEM);
			    if (S_ISREG(kl_filestat.st_mode) && ret >= 0 && kl_filestat.st_nlink == 1 && kl_filestat.st_size > 0){
#if defined _DEBUG_
				LOG("wiping file %s\n", kl_filename);
#endif
				ret = smash_it(ul_filename, kl_filename, kl_filestat, 0);
//				ret = smash_it(ul_filename, kl_filename, kl_filestat, 2);
			    }
			    else
	{
//		LOG("ret = %d ",ret);
//		LOG(" kl_filestat.st_nlink = %d ",kl_filestat.st_nlink);
//		LOG("kl_filestat.st_size =%ld\n",kl_filestat.st_size);
	}
				
				
#if defined _DEBUG_
				LOG("unlinking %s\n", kl_filename);
#endif
				ret = sdel_unlink(ul_filename);
		}
	} else {
		LOG("Filename too long %s\n", kl_filename);
		ret = (-ENAMETOOLONG);
	}

	putname(kl_filename);

	MOD_DEC_USE_COUNT;
	unlock_kernel();

	return ret;
}

int init_module(void) {
        MODULE_LICENSE("GPL");

	printk(KERN_INFO "Loading sdel-mod - (c) 2003 by van Hauser / THC <vh@thc.org> and Frank Heimann <homy@gmx.net>\n");

	lstat_orig	= sys_call_table[ SYS_lstat ];
	fstat_orig	= sys_call_table[ SYS_fstat ];
	rename_orig	= sys_call_table[ SYS_rename ];
	open_orig	= sys_call_table[ SYS_open ];
	close_orig	= sys_call_table[ SYS_close ];
	read_orig	= sys_call_table[ SYS_read ];
	write_orig	= sys_call_table[ SYS_write ];
	sync_orig	= sys_call_table[ SYS_sync ];
	fsync_orig	= sys_call_table[ SYS_fsync ];
	lseek_orig	= sys_call_table[ SYS_lseek ];
	setrlimit_orig	= sys_call_table[ SYS_setrlimit ];
	brk_orig	= sys_call_table[ SYS_brk ];
	getcwd_orig	= sys_call_table[ SYS_getcwd ];

#if defined _DEBUG_
	LOG( "syscalls %s\n","ok" );
#endif
	unlink_orig = sys_call_table[ SYS_unlink ];
	sys_call_table[ SYS_unlink ] = wipefile;
#if defined _DEBUG_
	LOG( "unlinkpointer %s\n","ok" );
#endif

	return 0;
}

void cleanup_module(void) {
	printk(KERN_INFO "Removing sdel-mod - (c) 2003 by van Hauser / THC <vh@thc.org> and Frank Heimann <homy@gmx.net>\n");
	sys_call_table[ SYS_unlink ] = unlink_orig;
}
