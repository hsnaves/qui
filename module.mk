OBJS := dev/console.o dev/devio.o dev/display.o dev/storage.o main.o \
 vm/quivm.o

dev/console.o: dev/console.c vm/quivm.h dev/console.h
dev/devio.o: dev/devio.c vm/quivm.h dev/devio.h dev/console.h dev/storage.h \
 dev/display.h
dev/display.o: dev/display.c vm/quivm.h dev/display.h
dev/storage.o: dev/storage.c vm/quivm.h dev/storage.h
main.o: main.c vm/quivm.h dev/devio.h dev/console.h dev/storage.h \
 dev/display.h
vm/quivm.o: vm/quivm.c vm/quivm.h
