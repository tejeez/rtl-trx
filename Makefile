rtl-trx: rtl-trx.c baudot.h
	gcc rtl-trx.c -o rtl-trx -Wall -Wextra -O3 -lrtlsdr -lpthread -lliquid

clean:
	rm rtl-trx

