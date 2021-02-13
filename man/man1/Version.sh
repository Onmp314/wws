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
		print ".TH " \$2 " 1 " "\"Version 1, Release 4\"" " " \\
			"\"W Window System\"" " " "\"W PROGRAMS\""
	} else {
		print
	}
}
EOF

# all W manual pages having more than one (redirection) line
files=`ls -l *.1 | awk '{ if($5 > 80) print $9 }'`

# update the headers
for i in $files
do
	echo $i
	awk -f $awk -- $i > $tmp
	mv $tmp $i
done

rm $tmp
rm $awk
unset files
