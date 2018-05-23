#!/usr/bin/env bash

pidfile="/var/run/chronyd.pid"

if [ -f "$pidfile" ]
then
	rm $pidfile
fi

exec /usr/local/sbin/chronyd -x -d -f /var/lib/chrony/chrony.conf