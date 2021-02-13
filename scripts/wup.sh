#!/bin/sh
#
# Updates W version but preserves the current configuration and makefiles.
# Requires GNU tar and gzip and parent directory to be writable as that's
# where this script archives the old configuration :-).
#
# This is intended mainly for systems where configuration files in configs/
# directory don't provide enough flexibility for your system (like older
# MiNT setups) and you have rolled up your own Makefiles.
#
# (w) 1998 by Eero Tamminen

if [ ! -f .config ]; then
	echo "You should run this from the W base directory!"
	exit 0
fi

# location of the archive for new W version
wtar=/a/w.tgz

# backup current configuration files (+ this script) and extract new W version
tar -zcvf ../makesw.tgz .config scripts/wup.sh **/Makefile **/**/Makefile **/**/**/Makefile
tar -zxvf $wtar -C../

# backup new configuration files, you can remove these after you have
# recompiled everything and it works (rm `find -name "*.orig" -print`).
for i in .config scripts/wup.sh **/Makefile **/**/Makefile **/**/**/Makefile
do
	mv -i $i $i.orig
	# restore: mv -i $i.orig ${i%.orig}
done

# restore old configuration files
tar -zxvf ../makesw.tgz
unset wtar
unset i

# recompile W
#make clean
#make
# test, fix errors, remake and install
