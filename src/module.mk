OBJS := dev/audio.o dev/console.o dev/devio.o dev/display.o dev/keyboard.o \
 dev/network.o dev/rtclock.o dev/storage.o dev/timer.o vm/quivm.o

dev/audio.o: dev/audio.c vm/quivm.h dev/audio.h
dev/console.o: dev/console.c vm/quivm.h dev/console.h
dev/devio.o: dev/devio.c vm/quivm.h dev/devio.h dev/console.h dev/storage.h \
 dev/network.h dev/rtclock.h dev/display.h dev/audio.h dev/keyboard.h \
 dev/timer.h
dev/display.o: dev/display.c vm/quivm.h dev/display.h
dev/keyboard.o: dev/keyboard.c vm/quivm.h dev/keyboard.h
dev/network.o: dev/network.c vm/quivm.h dev/network.h
dev/rtclock.o: dev/rtclock.c vm/quivm.h dev/rtclock.h
dev/storage.o: dev/storage.c vm/quivm.h dev/storage.h
dev/timer.o: dev/timer.c vm/quivm.h dev/timer.h
main.o: main.c vm/quivm.h dev/devio.h dev/console.h dev/storage.h \
 dev/network.h dev/rtclock.h dev/display.h dev/audio.h dev/keyboard.h \
 dev/timer.h
main-sdl.o: main-sdl.c vm/quivm.h dev/devio.h dev/console.h dev/storage.h \
 dev/network.h dev/rtclock.h dev/display.h dev/audio.h dev/keyboard.h \
 dev/timer.h
vm/quivm.o: vm/quivm.c vm/quivm.h
