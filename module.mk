OBJS := dev/console.o dev/devio.o dev/display.o dev/network.o dev/rtclock.o \
 dev/storage.o main.o vm/quivm.o

dev/console.o: dev/console.c vm/quivm.h dev/console.h
dev/devio.o: dev/devio.c vm/quivm.h dev/devio.h dev/console.h dev/storage.h \
 dev/network.h dev/rtclock.h dev/display.h
dev/display.o: dev/display.c vm/quivm.h dev/display.h
dev/network.o: dev/network.c vm/quivm.h dev/network.h
dev/rtclock.o: dev/rtclock.c vm/quivm.h dev/rtclock.h
dev/storage.o: dev/storage.c vm/quivm.h dev/storage.h
main.o: main.c vm/quivm.h dev/devio.h dev/console.h dev/storage.h \
 dev/network.h dev/rtclock.h dev/display.h
vm/quivm.o: vm/quivm.c vm/quivm.h
