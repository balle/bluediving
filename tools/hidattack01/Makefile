
CFLAGS=-Wall
LIBS=-lbluetooth

hidattack: hidattack.o sdp.o
	$(CC) $(CFLAGS) $(LIBS) hidattack.o sdp.o -o hidattack

clean:
	rm -f *.o hidattack
