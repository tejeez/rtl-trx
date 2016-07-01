all: beacon rtl-trx

rtl-trx: rtl-trx.c baudot.h
	gcc rtl-trx.c -o rtl-trx -Wall -Wextra -O3 -lrtlsdr -lpthread -lliquid

beacon: beacon.c baudot.h
	gcc beacon.c -o beacon -Wall -Wextra -O3 -lrtlsdr

clean:
	rm rtl-trx beacon

