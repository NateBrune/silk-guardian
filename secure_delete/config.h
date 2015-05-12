#define BLOCKSIZE	32769           /* must be mod 3 = 0, should be >= 16k */
#define RANDOM_DEVICE	"/dev/urandom"  /* must not exist */
#define DIR_SEPERATOR	'/'             /* '/' on unix, '\' on dos/win */
#define FLUSH		sync()          /* system call to flush the disk */
#define MAXINODEWIPE    4194304         /* 22 bits */
