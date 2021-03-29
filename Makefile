all: deskfetch

deskfetch: main.o
	gcc main.o -o deskfetch -lX11 -lcairo

clean:
	rm *.o deskfetch

# install: all
# 	mkdir -p ${DESTDIR}${PREFIX}/bin
# 	install -m 755 ${PROG} ${DESTDIR}${PREFIX}/bin/${PROG}
# 	mkdir -p ${DESTDIR}${MANPREFIX}/man1
# 	install -m 644 ${PROG}.1 ${DESTDIR}${MANPREFIX}/man1/${PROG}.1

# uninstall:
# 	rm -f ${DESTDIR}${PREFIX}/bin/${PROG}
# 	rm -f ${DESTDIR}${MANPREFIX}/man1/${PROG}.1
