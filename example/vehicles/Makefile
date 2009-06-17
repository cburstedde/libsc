
include $(HOME)/nosave/build/libsc/local/etc/Makefile.sc.mk

CC = $(SC_CC)
CFLAGS = -Wall -g -O0 -rdynamic -I. $(SC_CPPFLAGS) $(SC_CFLAGS)
LFLAGS = -Wall -g -O0 -rdynamic $(SC_CFLAGS) $(SC_LDFLAGS)
LIBS = $(SC_LIBS)

TARGETS = scobj2

all: $(TARGETS)

.c.o:
	$(CC) -c $(CFLAGS) $<

scobj2: sc_object_system.o sc_object.o \
        vehicle.o car.o tunedcar.o boat.o scobj_main.o
	$(CC) $(LFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f *.o *~ $(TARGETS)

.PHONY: all clean
