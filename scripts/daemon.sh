#!/bin/sh
set -e

PIDFILE="/tmp/tigerlash.pid"
PROGDIR="$PWD"

case "$1" in
  start)
  	echo -n "Starting tigerlash: "
  	LD_LIBRARY_PATH=$PROGDIR start-stop-daemon --start --pidfile $PIDFILE --exec $PROGDIR/tigerlash -- -c $PROGDIR/tigerlash-d.json --daemonize --pidfile $PIDFILE
  	echo "done"
  	;;
  stop)
  	echo -n "Stopping tigerlash: "
  	start-stop-daemon --stop --pidfile $PIDFILE
  	echo "done"
  	;;
  restart)
  	$0 stop
    $0 start
    echo "done"
	  ;;
  status)
    start-stop-daemon --status --pidfile $PIDFILE
    echo $?
    ;;
  *)
  	echo "Usage: tigerlash { start | stop | restart | status }" >&2
  	exit 1
  	;;
esac

exit 0
