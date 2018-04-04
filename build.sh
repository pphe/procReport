#!/bin/bash
# Quick build script.

echo "Building module."
make clean
make all

echo "Loading module."
sudo insmod procReport.ko
sleep 1 # give module a chance to generate proc data
cat /proc/proc_report > output.txt
vim output.txt

echo "Unloading module."
sudo rmmod procReport.ko

echo
echo "Last 10 lines of /var/log/syslog:"

if [ -f '/var/log/syslog' ]; then
	sudo tail -n 10 /var/log/syslog # my machine
else
	sudo tail -n 10 /var/log/messages # centos
fi
echo "Done."

