CC=gcc
OPT=-O2 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE
#OPT=-Wall -D_DEBUG_ -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE
INSTALL_DIR=/usr/local/bin
MAN_DIR=/usr/local/man
DOC_DIR=/usr/share/doc/secure_delete
OPT_MOD=-D__KERNEL__ -DMODULE -fomit-frame-pointer -fno-strict-aliasing -pipe -mpreferred-stack-boundary=2
#LD_MOD=-r

all: sdel-lib.o srm sfill sswap smem sdel-mod.o
	@echo
	@echo "A Puritan is someone who is deathly afraid that someone, somewhere, is"
	@echo "having fun."
	@echo
	@echo "I hope YOU have fun!"
	@echo

sdel-mod.o: sdel-mod.c
	$(CC) $(OPT) $(OPT_MOD) $(LD_MOD) -I/lib/modules/`uname -r`/build/include -c sdel-mod.c

sdel-lib.o: sdel-lib.c
	$(CC) ${OPT} -c sdel-lib.c

srm: srm.c
	$(CC) ${OPT} -o srm srm.c sdel-lib.o
	-strip srm
sfill: sfill.c
	$(CC) ${OPT} -o sfill sfill.c sdel-lib.o
	-strip sfill
sswap: sswap.c
	$(CC) ${OPT} -o sswap sswap.c sdel-lib.o
	-strip sswap
smem: smem.c
	$(CC) ${OPT} -o smem smem.c sdel-lib.o
	-strip smem

clean:
	rm -f sfill srm sswap smem sdel sdel-lib.o sdel-mod.o core *~

install: all
	mkdir -p -m 755 ${INSTALL_DIR} 2> /dev/null
	rm -f sdel && ln -s srm sdel
	cp -f sdel srm sfill sswap smem the_cleaner.sh ${INSTALL_DIR}
	chmod 711 ${INSTALL_DIR}/srm ${INSTALL_DIR}/sfill ${INSTALL_DIR}/sswap ${INSTALL_DIR}/smem ${INSTALL_DIR}/the_cleaner.sh
	mkdir -p -m 755 ${MAN_DIR}/man1 2> /dev/null
	cp -f srm.1 sfill.1 sswap.1 smem.1 ${MAN_DIR}/man1
	chmod 644 ${MAN_DIR}/man1/srm.1 ${MAN_DIR}/man1/sfill.1 ${MAN_DIR}/man1/sswap.1 ${MAN_DIR}/man1/smem.1
	mkdir -p -m 755 ${DOC_DIR} 2> /dev/null
	cp -f CHANGES FILES README secure_delete.doc usenix6-gutmann.doc ${DOC_DIR}
	-test -e sdel-mod.o && cp -f sdel-mod.o /lib/modules/`uname -r`/kernel/drivers/char
#	@-test '!' -e sdel-mod.o -a `uname -s` = 'Linux' && echo "type \"make sdel-mod install\" to compile and install the Linux loadable kernel module for secure delete"
	@echo
	@echo "If men could get pregnant, abortion would be a sacrament."
	@echo
