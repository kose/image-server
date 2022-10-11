#!/bin/bash

COMMAND=image-server

#
cd `dirname $0`
CDIR=`pwd`
BUILDDIR=/tmp/`basename $CDIR`

##
## analize options
##
BUILD=Release

while [ $# -gt 0 ]
do
    case $1 in
	--clean | --distclean)
	    echo "rm -rf $BUILDDIR"
	    rm -rf $BUILDDIR
	    exit 0;;

	--debug)
	    BUILD=Debug
	    ;;
	
	*)
	    echo "unknown: $1"
	    ;;
    esac
    shift
done


mkdir -p $BUILDDIR
cd $BUILDDIR

cmake $CDIR/cpp -DEXE=$COMMAND -DCMAKE_BUILD_TYPE=$BUILD

make -j 4 || exit -1

exit 0

# end
