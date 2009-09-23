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

echo "mfcr2 show version: " >> $1

asterisk -rx 'mfcr2 show version' >> $1

echo -n "\nmfcr2 show channels:\n" >> $1

asterisk -rx 'mfcr2 show channels' > $1.channels.txt

cat $1.channels.txt >> $1

echo -n "\n$dahdi channels status:\n" >> $1

asterisk -rx 'dahdi show chan'

for chan in `tail +2 $1.channels.txt | awk '{print $1}'`
do
	echo "dahdi show channel $chan" >> $1
done

rm $1.channels.txt

set +x

