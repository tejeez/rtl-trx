rtl-trx: rtl-trx.c
	gcc rtl-trx.c -o rtl-trx -Wall -Wextra -O3 -lrtlsdr -lpthread

clean:
	rm rtl-trx

