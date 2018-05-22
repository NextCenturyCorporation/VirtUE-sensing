#!/usr/bin/env bash

pidfile="/var/run/chronyd.pid"

if [ -f "$pidfile" ]
then
	rm $pidfile
fi

exec /usr/sbin/chronyd -d -f /var/lib/chrony/chrony.conf