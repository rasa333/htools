CC             = gcc
CFLAGS         = -O3 -Wunused -I/var/storage/postgres/include -fgnu89-inline -fcommon -Wstringop-overflow=0
LDFLAGS        = -L. # -static
LIBS           = -lm -ltermcap -lz
objects	       = array.o dir.o file.o funcs.o input.o postgres.o signal.o tcap.o base64.o broker.o lock.o

%.o : %.c $(wildcard *.h)
	$(CC) $(CFLAGS) -c $<

all test	: libhtools.a test.o
	$(CC) $(LDFLAGS) -o test test.o -lhtools $(LIBS)

libhtools.a   : $(objects)
	rm -f libhtools.a
	ar rc libhtools.a $(objects)

$(objects) : $(wildcard *.h)

clean:
	rm -f *.o test
