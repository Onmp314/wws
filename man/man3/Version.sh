#!/bin/sh
#
# Update W manual pages header line (needs (GNU) awk)
#
# 1998 (w) Eero Tamminen

# temporary file names
awk=version.awk
tmp=manual.tmp

# save the awk program doing the changes
cat > $awk << EOF
{
	if (FNR == 1) {
		print ".TH " \$2 " 3 " "\"Version 1, Release 4\"" " " \\
			"\"W Window System\"" " " "\"WLIB FUNCTIONS\""
	} else {
		print
	}
}
EOF

# Wlib functions start with 'w_' or lower case letter,
# files smaller than 80 chars contain page redirection
miscfiles=`ls -l [!a-z]*.3w | awk '{ if($5 > 80) print $9 }'`
libfiles=`ls -l [a-z]*.3w | awk '{ if($5 > 80) print $9 }'`

# update the headers
for i in $libfiles
do
	echo $i
	awk -f $awk -- $i > $tmp
	mv $tmp $i
done

# edit rest of the files by hand
$EDITOR $miscfiles

rm $tmp
rm $awk
unset libfiles
unset miscfiles

