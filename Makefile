CFLAGS = `pkg-config --cflags --libs pangocairo`
LIBS   = `pkg-config --libs pangocairo x11 xrandr`

all: deskfetch

deskfetch: main.o
	cc main.o -o $@ $(LIBS) $(CFLAGS)

clean:
	rm -f *.o deskfetch

# install: all
# 	mkdir -p ${DESTDIR}${PREFIX}/bin
# 	install -m 755 ${PROG} ${DESTDIR}${PREFIX}/bin/${PROG}
# 	mkdir -p ${DESTDIR}${MANPREFIX}/man1
# 	install -m 644 ${PROG}.1 ${DESTDIR}${MANPREFIX}/man1/${PROG}.1

# uninstall:
# 	rm -f ${DESTDIR}${PREFIX}/bin/${PROG}
# 	rm -f ${DESTDIR}${MANPREFIX}/man1/${PROG}.1
