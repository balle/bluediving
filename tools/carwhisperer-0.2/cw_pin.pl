#!/usr/bin/perl
# Special PIN helper that returns preset passkeys depending on the respective
# Bluetooth device address. This little script was done to be used as a
# replacement bluepin helper when using the 'carwhisperer' program that tries 
# to connect the SCO channels on a given Bluetooth device.
#
# Scripted in July 2005 by Martin Herfurt <martin@trifinite.org>
#

# this is the BDADDR of the device for which a passkey is required
$bdaddr = $ARGV[1];

undef $pin;

# match the address with known ones or return the 'standard pin'
# it's also possible to just specify the first part of the address for
# setting a default passkey for a certain manufacturer

SWITCH: for ($bdaddr) {
	/00:02:EE/ 	&& do { $pin="5475"; last;}; # Nokia
	/00:0E:9F/ 	&& do { $pin="1234"; last;}; # Audi UHV
	/00:80:37/ 	&& do { $pin="8761"; last;}; # O'Neill
	/00:0A:94/ 	&& do { $pin="1234"; last;}; # Cellink
	/00:0C:84/ 	&& do { $pin="1234"; last;}; # Eazix
	$pin="0000"; # 0000 is the default passkey in many cases
	}

# provide the preset PIN to the device that asks
print "PIN:$pin\n";
