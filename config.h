/* Files silk-guardian will remove upon detecting change in usb state. */
static char *removeFiles[] = {
	"/home/user/privatekey",
	"/private/ssnumber.pdf",
};

/* How many times to shred file. The more iterations the longer it takes. */
static char *shredIterations = "3";
