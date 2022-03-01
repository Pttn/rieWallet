#!/bin/sh

set -e

if test -d "incs" || test -d "libs" ; then
	echo "Dependencies already built, delete the incs and libs directories if you want to rebuild."
	exit
fi

deps=rieWallet0.9beta1Deps
if test -f "${deps}.tar.gz" ; then
	echo "Dependencies already downloaded, delete the ${deps}.tar.gz file if you want to download it again."
	exit
else
	wget "https://riecoin.dev/resources/Pttn/${deps}.tar.gz"
fi

tar -xf "${deps}.tar.gz"
if test ${1:-nobuild} = "build" ; then
	cd $deps
	./Build.sh
	mv incs ..
	mv libs ..
	cd ..
	rm -rf $deps
fi
