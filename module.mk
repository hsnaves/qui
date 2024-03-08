OBJS := dev/console.o dev/devio.o main.o vm/quivm.o

dev/console.o: dev/console.c vm/quivm.h dev/console.h
dev/devio.o: dev/devio.c vm/quivm.h dev/devio.h dev/console.h
main.o: main.c vm/quivm.h dev/devio.h dev/console.h
vm/quivm.o: vm/quivm.c vm/quivm.h
