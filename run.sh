#!/bin/bash

COMMAND=image-server

USAGE="Usage: `basename $0` [--clean]"

#
cd `dirname $0`
CDIR=`pwd`
BUILDDIR=/tmp/`basename $CDIR`

##
do_command()
{
    echo $1
    $1
}


##
## killer
##
server_kill()
{
    PS=`ps x | grep "/tmp/[Ii]mage-server" | awk '{print $1}'`
    
    if test "$PS" = ""; then
	echo "No image-server process"
    else
	echo "kill image-server: $PS"
	kill $PS
    fi
}

##
## 
##
case "$@" in
    *Kabukicho_2022-05-23_1800.mp4*)
	FLAGS_scale="-scale 1.1"
	FLAGS_mx="-mx -880"
	FLAGS_my="-my -320"
        ;;

    *1920x1080_shinyokohama2.mp4*)
	FLAGS_scale="-scale 0.85"
	FLAGS_mx="-mx -100"
	FLAGS_my="-my -60"
        ;;

    *QueensSquare2.mp4*)
	FLAGS_scale="-scale 0.9"
	FLAGS_mx="-mx -100"
	FLAGS_my="-my -60"
        ;;

    *QueensSquare4.mp4*)
	FLAGS_scale="-scale 1.0"
	FLAGS_mx="-mx -120"
	FLAGS_my="-my -230"
        ;;

    *machida2f.mp4*)
	FLAGS_scale="-scale 0.8"
	FLAGS_mx="-mx -120"
	FLAGS_my="-my -300"
        ;;

    *set?_cam_?.mp4*)
	FLAGS_width="-width 720"
	FLAGS_height="-height 540"
        ;;

    *)
	;;
esac

##
## analize options
##

while [ $# -gt 0 ]
do
    case $1 in
	--clean)
	    sh $CDIR/00make.sh --clean
	    ;;

        -d)
	    if [ "${2::1}" != "-" ]; then DEVICE="-d $2"; fi; shift;;

	-debug | --debug)
	    FLAGS_debug="--debug"
	    ;;

	-h | --help)
	    echo $USAGE
	    exit 0;;

        -i)
	    if [ "${2::1}" != "-" ]; then FLAGS_i="-i $2"; fi; shift;;

        -r | --rotate)
	    if [ "${2::1}" != "-" ]; then FLAGS_rotate="-rotate $2"; fi; shift;;

        -s | --scale)
	    if [ "${2::1}" != "-" ]; then FLAGS_scale="-scale $2"; fi; shift;;

        -mx)
	    FLAGS_mx="-mx $2"; shift;;

        -my)
	    FLAGS_my="-my $2"; shift;;

        -loop | --loop)
	    FLAGS_loop="-loop"; shift;;

	--kill)
	    server_kill
	    exit 0
	    ;;

        -port | --port)
	    if [ "${2::1}" != "-" ]; then FLAGS_port="-port $2"; fi; shift;;

	-*)
	    echo "unknown: $1"
	    # exit 0
	    ;;

	*)
	    echo "unknown: $1"
	    ;;
    esac
    shift
done

##
## Run
##
COMMAND=$BUILDDIR/$COMMAND

sh $CDIR/00make.sh $FLAGS_debug || exit -1

do_command "$COMMAND $FLAGS_i $FLAGS_port $FLAGS_loop $FLAGS_rotate $FLAGS_scale $FLAGS_mx $FLAGS_my $FLAGS_width $FLAGS_height"

exit 0

# end
