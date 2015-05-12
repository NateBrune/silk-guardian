#!/bin/sh
# 
# THE CLEANER SCRIPT
# Part of the secure_data_deletion toolkit by van Hauser / THC
# Run at your own risk. Tested on Linux only.
#
# Run this to wipe your system as most as possible with automatic stuff.
# You should run this in the STOP runlevels 2 and 3.
#
# ------------------------------------------------------------------------
#
# Please configure the following variables:
#
# 
# WIPE_MODE: 1-3
#            1: highly secure mode (38 wipes)
#            2: insecure mode (2 wipes)
#            3: highly insecure mode (1 wipe)
WIPE_MODE=3
#
#
# WIPE_FAST: yes/no
#            yes: dont use a secure random number generator, less secure
#            no:  use a secure random number generator, secure
WIPE_FAST=yes
#
#
# WIPE_VERBOSE: yes/no
#               yes: write verbose messages
#               no:  only write if error/warnings occur
WIPE_VERBOSE=yes
#
#
# WIPE_DIRECTORIES: directories you want to wipe completely all files and
#                   subdirectories from. Usually /tmp, /usr/tmp and /var/tmp.
WIPE_DIRECTORIES="/tmp /usr/tmp /var/tmp"
#
# WIPE_USER_FILES: files you want to wipe from user directories. Usually
#                  .*history*, .netscape/cache/*, .netscape/history*,
#                  .netscape/cookies, tmp/*, *~ and core
WIPE_USER_FILES=".*history* .netscape/cache/ .netscape/history* \
 .netscape/cookies .lynx_cookies tmp/* *~ .gqview_thmb/ dead.letter core"

# ------------------------------------------------------------------------
#
# Preparation Phase
#
test "$WIPE_MODE" -gt 0 -a "$WIPE_MODE" -lt 4 || {
    echo "WIPE_MODE must be a value between 1 and 3."
    exit 1
}
test "$WIPE_FAST" = yes -o "$WIPE_FAST" = "no" || {
    echo "WIPE_FAST must be either yes or no."
    exit 1
}
test "$WIPE_VERBOSE" = yes -o "$WIPE_VERBOSE" = "no" || {
    echo "WIPE_VERBOSE must be either yes or no."
    exit 1
}
MODE=""
test "$WIPE_MODE" -eq 2 && MODE="-l"
test "$WIPE_MODE" -eq 3 && MODE="-ll"
FAST=""
test "$WIPE_FAST" = yes && FAST="-f"
VERBOSE=""
test "$WIPE_VERBOSE" = yes && VERBOSE="-v"

# ------------------------------------------------------------------------
#
# Starting the wiping process
#

# Wipe directories
test -z "$VERBOSE" || echo "STARTING THE CLEANER."
test -z "$VERBOSE" || echo "CLEANER: Wiping directory contents"
for i in $WIPE_DIRECTORIES; do
    test -z "$i" -o "$i" = "." -o "$i" = ".." -o "$i" = "/" || { 
        test -z "$VERBOSE" || echo "         $i"
        cd "$i" && srm $MODE $FAST -d -r -- .* *
    }
done

# Wipe files
test -z "$VERBOSE" || echo "CLEANER: Wiping user files"
awk -F: '{ print $1 " " $6 }' /etc/passwd |
while read user homedir; do
    test "$homedir" = "" -o "$homedir" = "." -o "$homedir" = ".." || {
        cd "$homedir" && {
            test -z "$VERBOSE" || echo "         $user"
            for j in $WIPE_USER_FILES; do
                test -z "$j" || {
                    test -L "$j" || {
                        test -e "$j" && srm $MODE $FAST -d -r -- $j
#                        test -e "$j" && "i would wipe: $j"
                    }
                }
            done
        }
    }
done

# Wipe free space and inodes
test -z "$VERBOSE" || echo "CLEANER: Wiping free space and inodes on filesystems"
for i in `mount|grep -E '^/dev/.* type ext'|awk '{print$3}'`; do
    test -z "$VERBOSE" || echo "         $i"
    test -z "$i" || sfill $MODE $FAST "$i"
done

# Now the swap space:
test -z "$VERBOSE" || echo "CLEANER: Wiping swap space"
#ACTIVE=`swapoff -s|grep ^/dev/|awk '{print$1}'`
swapoff -a
for i in `(grep -w swap /etc/fstab|awk '{print$1}';echo "$ACTIVE";)|sort -u`; do
    test -z "$VERBOSE" || echo "         $i"
    test -z "$i" || sswap $MODE $FAST "$i"
done

# Finally the memory:
test -z "$VERBOSE" || echo "CLEANER: Wiping the memory"
smem $MODE $FAST
#

swapon -a

# FINNISHED!
test -z "$VERBOSE" || echo "THE CLEANER FINNISHED."
