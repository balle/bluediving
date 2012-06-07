# Bluetooth Stack Smasher (BSS)
# Pierre BETOUIN <pierre.betouin@security-labs.org>
# Modification Ollie Whitehouse <ol at uncon dot org>

CC=gcc
MAKE=make
CFLAGS=-Wall

BINDIR=/usr/local/bin
ETCDIR=/usr/local/etc

BSS_OBJ=bss
BSS_SRC=bss.c
L2P_SRC=l2ping.c
REP_SRC=replace.c
BSS_TMP=bss.o
L2P_TMP=l2ping.o
REP_TMP=replace.o
BSS_FLAGS=
BSS_LIBS=-lbluetooth

all: $(BSS_OBJ)

$(BSS_OBJ): $(BSS_SRC) $(BSS_INC)
	$(CC) -c $(BSS_SRC) 
	$(CC) -c $(L2P_SRC)
	$(CC) -c $(REP_SRC)
	$(CC) $(BSS_TMP) $(L2P_TMP) $(REP_TMP) -o $(BSS_OBJ) $(CFLAGS) $(BSS_FLAGS) $(BSS_LIBS)

install:
	strip $(BSS_OBJ)
	cp $(BSS_OBJ) $(BINDIR)

clean:
	rm -f $(BSS_OBJ) $(BSS_TMP) $(L2P_TMP) $(REP_TMP)
	replay_packet/clean.sh
