#!/bin/sh

# The 'fontedit' program that this script uses, will save the new font to
# current directory with a suitable name (needs to be correct as server
# maps font attributes to name).
#
# (w) 1997 by Eero Tamminen

function newfonts()
{
	verbose=false
	for i in $*
	do
		case $i in
			-v)		verbose=true; continue;;
			*[0-9]bi.*)	options="-s bi";;
			*[0-9]i.*)	options="-s i";;
			*[0-9]b.*)	options="-s b";;
			*)		options="";;
		esac
		if [ $verbose = true ]
		then
			echo "converting $i..."
			fontedit $i -v 1 $options
		else
			fontedit $i $options
		fi
	done
	return 0
}

export newfonts

# usage message
cat << EOF
usage: newfonts [-v] <fonts>

will show the new font attributes (if '-v' used) and then write the
converted font into current directory with a correct name.
EOF
