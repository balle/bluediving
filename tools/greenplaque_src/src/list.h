/*
   Written 2003 by Ollie Whitehouse <ollie@atstake.com>
   Modified 2005 by KF <kf@digitalmunition.com>
*/

struct {
   char *name;
   char *addr;
   char *desc;
} manf[] = {
//   {"3com",             "00:0B:AC", "3Com Europe Ltd."},
   {"Ericsson",         "00:01:EC", "Ericsson Group pre Sony-Ericsson"},
   {"SE",               "00:80:37", "Ericsson Group Sony-Ericsson"},
   {"SE2",              "00:0A:D9", "Sony Ericsson Mobile Communications Ab"},
//   {"murata",           "00:60:57", "Murata Manufacturing Co., Ltd. Nokia"},
   {"Nokia",            "00:02:EE", "Nokia Danmark A/s"},
   {"tdk",              "00:80:98", "TDK Corporation"},
   {"dlink",            "00:80:C8", "D-link Systems, Inc. CSR Chipset"},
   {"digianswer",       "00:50:CD", "Digianswer A/s"},
   {"Tecom",            "00:03:C9", "Tecom Co., Ltd."},
   {"apple",		"00:03:93", "Apple Computer, Inc."},
   {"siwave",		"00:03:3A", "Silicon Wave, Inc."},
   {"csr",		"00:02:5B", "Cambridge Silicon Radio"},
   {"widcomm",		"00:03:61", "Widcomm, Inc."},
   {"redm",		"00:0A:1E", "Red-M Communications Limited"},
//   {"billion",		"00:10:60", "Billionton Systems, Inc."},
   {"Nokia2",		"00:E0:03", "Nokia Wireless Business Communications"},
   {"alpsipaq",		"00:02:C7", "Alps Electric Co., Ltd. Ipaq 38xx"},
   {"intelbt",		"00:D0:B7", "Intel Corporation Bluetooth"},
//   {"3com2",		"00:04:76", "3 Com Corporation Bluetooth"},
   {"cmt",		"00:30:8E", "Cross Match Technologies, Inc. Axis"},
   {"windigo",		"00:08:1B", "Windigo Systems"},
   {"taiyo",		"00:03:7A", "Taiyo Yuden Co., Ltd."},
   {"abocom",		"00:E0:98", "AboCom Systems, Inc. Palladio USB CSR Chipset"},
   {"anicom",		"00:40:05", "Ani Communications Inc."},
// KF
   {"actiontec",	"00:20:E0", "Actiontec BTMxxx"},
   {"ambicom",		"00:10:7A", "AmbiCom BT2000C-USB"},
   {"belkin",		"00:0A:3A", "Belkin F8T003-USB"},
   {"jabra",		"00:07:A4", "Jabra BT110 Bluetooth headset"},
   {"ipaq2215",		"00:04:3E", "HP Ipaq"},
   {"siemensag",	"00:01:E3", "Siemens AG/S55 Phone"},
   {"smc",		"00:11:B1", "SMC-BT10"},
   {"sprint-lg",	"08:00:28", "Sprint LG PM325 Phone"},
   {"zoom",		"00:10:60", "Zoom Class 1 device"},
   {"apple-powerbook",	"00:0D:93", "Apple PowerBook G4"},
   {"3comDeskjet955c",	"00:04:76", "HP Deskjet 955c"},
   {"Blackberry",	"00:0F:86", "RIM Blackberry 7100t 7290"},
   {"kodakkiosk",	"00:0B:AC", "Kodak PictureMaker Kiosk"},
   {"vuln-ericsson",	"00:0E:07", "Sony Ericsson T616 - vuln"},
   {"nokia",		"00:60:57", "Nokia 3650"},
   {"nokia6820",	"00:20:EE", "Nokia 6820 - ctech chipset?"},
   {"landrover",	"00:08:2F", "LandRover LR3 / RangeRover HSE"},
   {"nokia6600",	"00:0E:6D", "Nokia 6600"},
   {"motov505",		"00:0A:28", "motorola v505"}, // JT's phone.. im commin for ya JT! =]
   {0, 0}
};


