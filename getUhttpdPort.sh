#!/bin/sh

result=`uci get uhttpd.main.listen_http`
for section in $result
do
	r=`echo $section | grep '0.0.0.0'`
	if [ -n "$r" ]; then
		r=`echo $r | awk -F ':' '{print $2}'`
		echo $r
	fi
done
