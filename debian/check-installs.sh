#!/bin/sh
#
# do some checks on the *.install files content:
# - everything installed only once / to one package
# - all usr/share/man/man1/ manual pages listed in them
# - all usr/bin/ binaries listed in them
# latter two are because they need to be specified
# individually as they go to multiple packages.

src=check.all.tmp
dst=check.install.tmp

show_find_cmd ()
{
	paths=""
	for path in '*bin/*' '*man1/*' '*games/*' '*man6/*'; do
		if [ -z "$paths" ]; then
			paths="find tmp/ -path '$path'"
		else
			paths="$paths -o -path '$path'"
		fi
	done
	echo "$paths"
}
# show find command
#show_find_cmd

echo
echo "Install dirs/patterns:"
fgrep -e '*' -e '/$' *.install | sed 's/^/- /' | tr : '\t'

find tmp/ -path '*bin/*' -o -path '*man1/*' -o -path '*games/*' -o -path '*man6/*' | sed 's%^tmp/%%' | sort > $src
fgrep -h -v '*' *.install | sort > $dst

echo "Install overlaps:"
uniq -c $dst | grep -v " 1 "

echo
echo "Missing /usr/bin/ binaries + their manpages:"
diff -u $src $dst | grep '^-[^-]' | sed 's/^-/- /'

rm $src $dst
