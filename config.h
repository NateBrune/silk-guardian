/* Files silk-guardian will remove upon detecting change in usb state. */
static char *remove_files[] = {
	"/home/user/privatekey",
	"/private/ssnumber.pdf",
	NULL,	/* Must be NULL terminated */
};

/* How many times to shred file. The more iterations the longer it takes. */
static char *shredIterations = "3";

/* List of all USB devices you want whitelisted (i.e. ignored) */
static const struct usb_device_id whitelist_table[] = {
	{ USB_DEVICE(0x0000, 0x0000) },
};
