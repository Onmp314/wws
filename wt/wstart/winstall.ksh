# *UNTESTED* installation script for wstart
# needs `%' and `#' substitutions so plain `sh' won't do

echo "run 'wstart_install' to install WStart after running this script"
wstart_install()
{
	# if you really want to test the script, remove this safety feature
	# and mail me how it worked :-)	-- t150315@cc.tut.fi, Eero Tamminen
	echo "Untested, so beware... (quitting here)"
	return 0

	echo "Compiling WStart..."
	make wstart
	if [ ! -x wstart ]
	then
		echo "...failed."
		return -1
	fi
	path=`which wserver`
	path=${path%/*}
	start=~/wstart-dir

	mv wstart $path/
	echo "...and moved to $path/."

	echo
	echo "Creating WStart root directory..."
	mkdir $start
	mkdir $start/start
	if [ ! -d $start/start ]
	then
		echo "...failed."
		unset start
		unset path
		return -1
	fi
	echo "...done."

	echo "Linking excutables from W server path to WStart directory..."
	for file in $path/*
	do
		if [ -x $file ]
		then
			ln -s $file $start/start/${file##*/}
		fi
	done
	echo "...done."

	echo "Adding WStart to W configuration files..."
	cat "wstart $start" >> ~/.wrc
	cat "menuitem=WStart,wstart $start" >> ~/.wconfig
	echo "...all done."


	# xinit style W startup script
	#
	# save startup script
	#ppid='$!'
	#wpid='$WPID'
	#cat > $path/winit.sh << EOF
	#	wserver --forcemono &
	#	WPID=$ppid
	#	wstart $start
	#	kill $wpid
	#EOF
	#
	#chmod 755 $path/winit.sh

	unset start
	unset path
	unset file
	return 0
}

