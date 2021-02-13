#!/bin/sh
#
# replace strings recursively in given files, requires 'find' and 'sed'.
#
# usage: replace '<file-pattern>' '<search-pattern>' <replace string>
#
# IMPORTANT:  remember to quote patterns!!!
#
# like this:
# 	../configs/replace.sh '*.c' 'foo[a-z]*' bar
# or:
#	configs/replace.sh Makefile ADDCFLAGS BASECFLAGS
#
# (w) 1998 by Eero Tamminen

if [ -z $1 ]; then
	echo "usage:	replace '<file-pattern>' '<text-pattern>' <replace-string>"
	echo
	echo "file pattern is given to 'find' and text pattern to 'sed'."
	echo "an example:"
	echo "	replace '*.c' 'foo[a-z]*' bar"
	exit 0
fi

echo "Replacing \"$2\" with \"$3\"..."

# temporary file name
tmp=replace.tmp

# all the files matching pattern
files=`find -name "$1" -print`

function fail
{
	echo "...FAILED, check your replace patterns!"
	rm $tmp
	exit 1
}

for i in $files
do
	# skip executables
	if [ -x $i ]; then

		echo $i "is executable, skipping..."
	else
		echo $i
		sed s/"$2"/"$3"/ $i > $tmp  || fail
		mv $tmp $i
	fi
done
unset files tmp i
