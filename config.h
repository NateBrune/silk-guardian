/* Files silk-guardian will remove upon detecting change in usb state. */
static char *remove_files[] = {
	//"/home/user/privatekey",
	//"/private/ssnumber.pdf",
	NULL,	/* Must be NULL terminated */
};

char *sdmem_argv[] = {
    "/usr/bin/sdmem",
    "-f", "-ll",
};

/* How many times to shred file. The more iterations the longer it takes. */
static char *shredIterations = "3";

/* List of all USB devices you want whitelisted (i.e. ignored) */
static const struct usb_device_id whitelist_table[] = {
	{ USB_DEVICE(0x0000, 0x0000) },
};


/* comment this if normal shutdown isn't fast enough */
/* Slower than kernel_power_off */
#define USE_ORDERLY_SHUTDOWN

/* Uncomment to wipe ram upon shutdown with sdmem */
/* #define WIPE_RAM */
