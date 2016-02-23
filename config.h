/* Files silk-guardian will remove upon detecting change in usb state. */
const char *removeFiles[] = {
	"/home/user/privatekey",
	"/private/ssnumber.pdf",
};

/* How many times to shred file. The more iterations the longer it takes. */
const char *shredIterations = "3";
