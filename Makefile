
CFLAGS=-Wall -g

OBJECTS=main.o unix.o tracing.o

toy-logging: bigobj.o
	$(CC) -pthread -o $@ $<

bigobj.o: $(OBJECTS)
	$(LD) -r -Ttrace.lds -o $@ $+

%.o: %.c tracing.h sys.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) *~ toy-logging
