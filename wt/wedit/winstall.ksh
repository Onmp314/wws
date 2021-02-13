# W text editor installer
# Editor binary to same directory as wserver,
# configuration and manual to default places.

binary=wedit
config=kurzels

wpath=`which wserver`
wpath=${wpath%/*}

make $binary

mv -v $binary $wpath/$binary
cp -v $binary.1 /usr/man/cat1/$binary.1
cp -v $config ~/.$config
