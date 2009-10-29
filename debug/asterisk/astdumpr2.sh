#!/bin/bash

if [ "x$1" = "x" ]
then
	echo "First argument must be a file path to dump the Asterisk R2 status"
	exit 1
fi

if [ -f /usr/lib/asterisk/modules/chan_dahdi.so ]
then
	dahdi="dahdi"
else
	dahdi="zap"		
fi

set -x

echo "== OpenR2 debugging information for Asterisk ==" > $1

echo "OpenR2 reported version: " >> $1
r2test -v >> $1

echo -e "\nmfcr2 show version: " >> $1

asterisk -rx 'mfcr2 show version' >> $1

for t in 1 2
do
	echo "" >> $1
	echo "== Data Collection $t ==" >> $1
	echo "" >> $1

	echo -e "\nmfcr2 show channels:\n" >> $1
	
	asterisk -rx 'mfcr2 show channels' > $1.channels.txt
	
	cat $1.channels.txt >> $1
	
	echo -e "\n$dahdi channels status:\n" >> $1
	
	for chan in `tail --lines=+2 $1.channels.txt | awk '{print $1}'`
	do
		asterisk -rx "dahdi show channel $chan" >> $1
	done
	
	rm $1.channels.txt
	sleep 2s
done

set +x

